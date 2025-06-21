// ResourceDeposit.cpp
// Lokalizacja: Source/FactoryNet/Private/Buildings/Base/ResourceDeposit.cpp

#include "Buildings/Base/ResourceDeposit.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/ResourceStorageComponent.h"
#include "Data/DepositDefinition.h"
#include "Buildings/Base/TransportHub.h"
#include "Core/DataTableManager.h"
#include "Engine/Engine.h"
#include "DrawDebugHelpers.h"

AResourceDeposit::AResourceDeposit()
{
    PrimaryActorTick.bCanEverTick = true;

    // Create components
    RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
    RootComponent = RootSceneComponent;

    DepositMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DepositMesh"));
    DepositMesh->SetupAttachment(RootComponent);

    StorageComponent = CreateDefaultSubobject<UResourceStorageComponent>(TEXT("ResourceStorage"));

    // Initialize default values
    CurrentLevel = 1;
    CurrentReserves = 0;
    LastExtractionTime = 0.0f;
    ConnectedHub = nullptr;
    bAutoExtractToStorage = true;
    ExtractionTickRate = 1.0f;
    bShowDebugInfo = false;
    TimeSinceLastExtraction = 0.0f;
    bHasBeenInitialized = false;
}

void AResourceDeposit::BeginPlay()
{
    Super::BeginPlay();
    
    // If we have a deposit definition set in editor, initialize with it
    if (DepositDefinition && !bHasBeenInitialized)
    {
        InitializeWithDefinition(DepositDefinition);
    }
}

void AResourceDeposit::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bHasBeenInitialized && !IsDepleted())
    {
        TickAutoExtraction(DeltaTime);
    }

    // Debug visualization
    if (bShowDebugInfo)
    {
        DrawDebugString(GetWorld(), GetActorLocation() + FVector(0, 0, 200), 
                       FString::Printf(TEXT("Reserves: %d/%d\nLevel: %d\nExtraction: %.1f/s"), 
                                     GetAvailableResource(), CurrentReserves, CurrentLevel, GetCurrentExtractionRate()),
                       nullptr, FColor::White, 0.0f);
    }
}

void AResourceDeposit::InitializeWithDefinition(UDepositDefinition* DepositDef)
{
    if (!DepositDef)
    {
        UE_LOG(LogTemp, Error, TEXT("ResourceDeposit: Cannot initialize with null DepositDefinition"));
        return;
    }

    DepositDefinition = DepositDef;
    CurrentReserves = DepositDef->TotalReserves;
    CurrentLevel = 1;

    // Set mesh from definition
    if (DepositDef->BaseMesh.LoadSynchronous())
    {
        DepositMesh->SetStaticMesh(DepositDef->BaseMesh.LoadSynchronous());
    }

    // Initialize storage component
    if (StorageComponent)
    {
        FDepositLevel LevelData = GetCurrentLevelData();
        StorageComponent->SetMaxCapacity(LevelData.MaxStorage);
        StorageComponent->SetResourceType(DepositDef->ResourceReference);
    }

    bHasBeenInitialized = true;
    UpdateVisualMesh();

    UE_LOG(LogTemp, Log, TEXT("ResourceDeposit: Initialized %s with %d reserves"), 
           *DepositDef->DepositName.ToString(), CurrentReserves);
}

void AResourceDeposit::InitializeFromSpawn(UDepositDefinition* DepositDef, int32 InitialLevel)
{
    InitializeWithDefinition(DepositDef);
    
    if (InitialLevel > 1 && InitialLevel <= GetMaxLevel())
    {
        CurrentLevel = InitialLevel;
        UpdateVisualMesh();
    }
}

int32 AResourceDeposit::ExtractResource(int32 RequestedAmount)
{
    if (!bHasBeenInitialized || IsDepleted() || RequestedAmount <= 0)
    {
        return 0;
    }

    // Calculate actual amount that can be extracted
    int32 AvailableAmount = GetAvailableResource();
    int32 ActualAmount = FMath::Min(RequestedAmount, AvailableAmount);

    if (ActualAmount > 0)
    {
        // For non-renewable resources, reduce reserves
        if (!IsRenewable())
        {
            CurrentReserves -= ActualAmount;
            CurrentReserves = FMath::Max(0, CurrentReserves);
        }

        LastExtractionTime = GetWorld()->GetTimeSeconds();
        BroadcastExtractionEvent(ActualAmount);
        CheckForDepletion();

        UE_LOG(LogTemp, VeryVerbose, TEXT("ResourceDeposit: Extracted %d of %s"), 
               ActualAmount, *GetDepositName().ToString());
    }

    return ActualAmount;
}

bool AResourceDeposit::CanExtractResource(int32 RequestedAmount) const
{
    if (!bHasBeenInitialized || IsDepleted())
    {
        return false;
    }

    return GetAvailableResource() >= RequestedAmount;
}

float AResourceDeposit::GetCurrentExtractionRate() const
{
    if (!DepositDefinition || !bHasBeenInitialized)
    {
        return 0.0f;
    }

    FDepositLevel LevelData = GetCurrentLevelData();
    return LevelData.ExtractionRate;
}

int32 AResourceDeposit::GetAvailableResource() const
{
    if (!bHasBeenInitialized || !StorageComponent)
    {
        return 0;
    }

    // For renewable resources, check storage
    if (IsRenewable())
    {
        return StorageComponent->GetCurrentAmount(GetResourceType());
    }
    else
    {
        // For non-renewable, return remaining reserves
        return CurrentReserves;
    }
}

int32 AResourceDeposit::GetMaxStorage() const
{
    if (!bHasBeenInitialized)
    {
        return 0;
    }

    FDepositLevel LevelData = GetCurrentLevelData();
    return LevelData.MaxStorage;
}

bool AResourceDeposit::UpgradeToLevel(int32 TargetLevel)
{
    if (!CanUpgradeToLevel(TargetLevel))
    {
        return false;
    }

    CurrentLevel = TargetLevel;
    UpdateVisualMesh();
    
    // Update storage capacity
    if (StorageComponent)
    {
        FDepositLevel LevelData = GetCurrentLevelData();
        StorageComponent->SetMaxCapacity(LevelData.MaxStorage);
    }

    OnDepositLevelChanged.Broadcast(this, CurrentLevel);
    OnDepositLevelChanged_BP(CurrentLevel);

    UE_LOG(LogTemp, Log, TEXT("ResourceDeposit: Upgraded %s to level %d"), 
           *GetDepositName().ToString(), CurrentLevel);

    return true;
}

bool AResourceDeposit::CanUpgradeToLevel(int32 TargetLevel) const
{
    if (!bHasBeenInitialized || !DepositDefinition)
    {
        return false;
    }

    return TargetLevel > CurrentLevel && 
           TargetLevel <= DepositDefinition->MaxLevel && 
           TargetLevel <= DepositDefinition->DepositLevels.Num();
}

float AResourceDeposit::GetUpgradeCost(int32 TargetLevel) const
{
    if (!CanUpgradeToLevel(TargetLevel))
    {
        return -1.0f;
    }

    if (TargetLevel <= DepositDefinition->DepositLevels.Num())
    {
        return DepositDefinition->DepositLevels[TargetLevel - 1].UpgradeCost;
    }

    return -1.0f;
}

int32 AResourceDeposit::GetMaxLevel() const
{
    if (!DepositDefinition)
    {
        return 1;
    }

    return DepositDefinition->MaxLevel;
}

void AResourceDeposit::ConnectToHub(ATransportHub* Hub)
{
    if (!Hub)
    {
        UE_LOG(LogTemp, Warning, TEXT("ResourceDeposit: Attempted to connect to null hub"));
        return;
    }

    if (ConnectedHub && ConnectedHub != Hub)
    {
        DisconnectFromHub();
    }

    ConnectedHub = Hub;
    UE_LOG(LogTemp, Log, TEXT("ResourceDeposit: Connected %s to hub"), 
           *GetDepositName().ToString());
}

void AResourceDeposit::DisconnectFromHub()
{
    if (ConnectedHub)
    {
        UE_LOG(LogTemp, Log, TEXT("ResourceDeposit: Disconnected %s from hub"), 
               *GetDepositName().ToString());
        ConnectedHub = nullptr;
    }
}

bool AResourceDeposit::RequiresHub() const
{
    if (!DepositDefinition)
    {
        return false;
    }

    return DepositDefinition->RequiresHub;
}

FDataTableRowHandle AResourceDeposit::GetResourceType() const
{
    if (!DepositDefinition)
    {
        return FDataTableRowHandle();
    }

    return DepositDefinition->ResourceReference;
}

FText AResourceDeposit::GetDepositName() const
{
    if (!DepositDefinition)
    {
        return FText::FromString("Unknown Deposit");
    }

    return DepositDefinition->DepositName;
}

bool AResourceDeposit::IsRenewable() const
{
    if (!DepositDefinition)
    {
        return false;
    }

    return DepositDefinition->IsRenewable;
}

bool AResourceDeposit::IsDepleted() const
{
    if (!bHasBeenInitialized)
    {
        return false;
    }

    if (IsRenewable())
    {
        return false; // Renewable resources never deplete
    }

    return CurrentReserves <= 0;
}

float AResourceDeposit::GetDepletionPercentage() const
{
    if (!DepositDefinition || IsRenewable())
    {
        return 0.0f;
    }

    float OriginalReserves = static_cast<float>(DepositDefinition->TotalReserves);
    float RemainingReserves = static_cast<float>(CurrentReserves);

    if (OriginalReserves <= 0.0f)
    {
        return 0.0f;
    }

    return 1.0f - (RemainingReserves / OriginalReserves);
}

void AResourceDeposit::UpdateVisualMesh()
{
    if (!DepositDefinition || !DepositMesh)
    {
        return;
    }

    UpdateMeshForLevel();
}

// === PRIVATE FUNCTIONS ===

void AResourceDeposit::TickAutoExtraction(float DeltaTime)
{
    if (!bAutoExtractToStorage || !bHasBeenInitialized || IsDepleted())
    {
        return;
    }

    TimeSinceLastExtraction += DeltaTime;

    if (TimeSinceLastExtraction >= ExtractionTickRate)
    {
        float ExtractionRate = GetCurrentExtractionRate();
        int32 ExtractAmount = FMath::RoundToInt(ExtractionRate * ExtractionTickRate);

        if (ExtractAmount > 0 && StorageComponent)
        {
            // For renewable resources, generate directly to storage
            if (IsRenewable())
            {
                StorageComponent->AddResource(GetResourceType(), ExtractAmount);
                BroadcastExtractionEvent(ExtractAmount);
            }
            else
            {
                // For non-renewable, extract from reserves to storage
                int32 ActualExtracted = ExtractResource(ExtractAmount);
                if (ActualExtracted > 0)
                {
                    StorageComponent->AddResource(GetResourceType(), ActualExtracted);
                }
            }
        }

        TimeSinceLastExtraction = 0.0f;
    }

    // Regeneration for renewable resources
    if (IsRenewable())
    {
        RegenerateResource(DeltaTime);
    }
}

void AResourceDeposit::UpdateMeshForLevel()
{
    if (!DepositDefinition || !DepositMesh)
    {
        return;
    }

    // Get level-specific mesh if available
    if (CurrentLevel <= DepositDefinition->DepositLevels.Num())
    {
        const FDepositLevel& LevelData = DepositDefinition->DepositLevels[CurrentLevel - 1];
        if (LevelData.LevelMesh.LoadSynchronous())
        {
            DepositMesh->SetStaticMesh(LevelData.LevelMesh.LoadSynchronous());
            return;
        }
    }

    // Fallback to base mesh
    if (DepositDefinition->BaseMesh.LoadSynchronous())
    {
        DepositMesh->SetStaticMesh(DepositDefinition->BaseMesh.LoadSynchronous());
    }
}

void AResourceDeposit::RegenerateResource(float DeltaTime)
{
    if (!IsRenewable() || !DepositDefinition || !StorageComponent)
    {
        return;
    }

    float RegenerationRate = DepositDefinition->RegenerationRate;
    if (RegenerationRate > 0.0f)
    {
        float RegenAmount = RegenerationRate * DeltaTime;
        int32 RegenAmountInt = FMath::FloorToInt(RegenAmount);
        
        if (RegenAmountInt > 0)
        {
            // Check if we have space in storage
            int32 CurrentAmount = StorageComponent->GetCurrentAmount(GetResourceType());
            int32 MaxCapacity = GetMaxStorage();
            
            if (CurrentAmount < MaxCapacity)
            {
                int32 SpaceAvailable = MaxCapacity - CurrentAmount;
                int32 ActualRegen = FMath::Min(RegenAmountInt, SpaceAvailable);
                
                StorageComponent->AddResource(GetResourceType(), ActualRegen);
            }
        }
    }
}

FDepositLevel AResourceDeposit::GetCurrentLevelData() const
{
    if (!DepositDefinition || CurrentLevel <= 0 || CurrentLevel > DepositDefinition->DepositLevels.Num())
    {
        // Return default level data
        FDepositLevel DefaultLevel;
        DefaultLevel.Level = 1;
        DefaultLevel.ExtractionRate = 1.0f;
        DefaultLevel.MaxStorage = 100;
        DefaultLevel.EnergyConsumption = 1.0f;
        DefaultLevel.UpgradeCost = 1000.0f;
        return DefaultLevel;
    }

    return DepositDefinition->DepositLevels[CurrentLevel - 1];
}

void AResourceDeposit::BroadcastExtractionEvent(int32 Amount)
{
    OnResourceExtracted.Broadcast(this, GetResourceType(), Amount);
    OnResourceExtracted_BP(Amount);
}

void AResourceDeposit::CheckForDepletion()
{
    if (!IsRenewable() && IsDepleted())
    {
        UE_LOG(LogTemp, Warning, TEXT("ResourceDeposit: %s has been depleted"), 
               *GetDepositName().ToString());
        
        OnDepositDepleted.Broadcast(this);
        OnDepositDepleted_BP();
    }
}