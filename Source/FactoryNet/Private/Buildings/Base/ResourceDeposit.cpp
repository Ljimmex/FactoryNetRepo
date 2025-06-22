// ResourceDeposit.cpp - POPRAWIONY Z COLLISION I FIX AVAILABLE RESOURCES
// Lokalizacja: Source/FactoryNet/Private/Buildings/Base/ResourceDeposit.cpp

#include "Buildings/Base/ResourceDeposit.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"  // ✅ DODANO
#include "Components/ResourceStorageComponent.h"
#include "Data/DepositDefinition.h"
#include "Core/DataTableManager.h"
#include "Engine/Engine.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"  // ✅ DODANO
#include "EngineUtils.h"  // ✅ DODANO: Required for TActorIterator

AResourceDeposit::AResourceDeposit()
{
    PrimaryActorTick.bCanEverTick = true;

    // Create components
    RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
    RootComponent = RootSceneComponent;

    DepositMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DepositMesh"));
    DepositMesh->SetupAttachment(RootComponent);

    // ✅ DODANO: Collision Component
    CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
    CollisionComponent->SetupAttachment(RootComponent);
    CollisionComponent->SetSphereRadius(CollisionRadius);
    CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CollisionComponent->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
    CollisionComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);

    StorageComponent = CreateDefaultSubobject<UResourceStorageComponent>(TEXT("ResourceStorage"));

    // Initialize default values
    CurrentLevel = 1;
    CurrentReserves = 0;
    LastExtractionTime = 0.0f;
    bAutoExtractToStorage = true;
    ExtractionTickRate = 1.0f;
    bShowDebugInfo = false;
    TimeSinceLastExtraction = 0.0f;
    bHasBeenInitialized = false;
    
    // ✅ DODANO: Collision defaults
    CollisionRadius = 500.0f;
    bPreventOverlapping = true;
    bShowCollisionRadius = false;
}

void AResourceDeposit::BeginPlay()
{
    Super::BeginPlay();
    
    // Setup collision
    SetupCollision();
    
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
                       FString::Printf(TEXT("Reserves: %d/%d\nLevel: %d\nExtraction: %.1f/s\nStored: %d/%d"), 
                                     GetAvailableResource(), CurrentReserves, CurrentLevel, GetCurrentExtractionRate(),
                                     GetCurrentStoredAmount(), GetMaxStorage()),
                       nullptr, FColor::White, 0.0f);
    }
    
    // ✅ DODANO: Collision radius debug
    if (bShowCollisionRadius && GetWorld())
    {
        DrawDebugSphere(GetWorld(), GetActorLocation(), CollisionRadius, 16, FColor::Orange, false, 0.1f, 0, 2.0f);
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
        
        // ✅ POPRAWKA: Pre-fill storage dla renewable resources
        if (DepositDef->IsRenewable)
        {
            // Start with some initial resources in storage
            int32 InitialAmount = FMath::RoundToInt(LevelData.MaxStorage * 0.1f); // 10% initial fill
            StorageComponent->SetInitialResource(DepositDef->ResourceReference, InitialAmount);
            
            UE_LOG(LogTemp, Log, TEXT("ResourceDeposit: Pre-filled renewable storage with %d resources"), InitialAmount);
        }
    }

    // ✅ DODANO: Update collision size based on deposit type
    UpdateCollisionSize();

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
        UpdateCollisionSize();  // ✅ DODANO
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
        else
        {
            // For renewable resources, remove from storage
            if (StorageComponent)
            {
                ActualAmount = StorageComponent->RemoveResource(GetResourceType(), ActualAmount);
            }
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

// ✅ POPRAWKA: GetAvailableResource teraz poprawnie zwraca dane
int32 AResourceDeposit::GetAvailableResource() const
{
    if (!bHasBeenInitialized)
    {
        return 0;
    }

    // For renewable resources, check storage
    if (IsRenewable())
    {
        if (StorageComponent)
        {
            return StorageComponent->GetCurrentAmount(GetResourceType());
        }
        return 0;
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
    UpdateCollisionSize();  // ✅ DODANO
    
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
        return false; // Renewable resources never deplete completely
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

// === STORAGE ACCESS FUNCTIONS ===

int32 AResourceDeposit::GetCurrentStoredAmount() const
{
    if (!StorageComponent || !bHasBeenInitialized)
    {
        return 0;
    }

    return StorageComponent->GetCurrentAmount(GetResourceType());
}

float AResourceDeposit::GetStoragePercentage() const
{
    if (!StorageComponent || !bHasBeenInitialized)
    {
        return 0.0f;
    }

    int32 CurrentAmount = GetCurrentStoredAmount();
    int32 MaxCapacity = GetMaxStorage();
    
    if (MaxCapacity <= 0)
    {
        return 0.0f;
    }

    return (float)CurrentAmount / (float)MaxCapacity;
}

// ✅ DODANO: Collision functions
bool AResourceDeposit::IsLocationTooCloseToOthers(const FVector& TestLocation, float MinDistance) const
{
    if (!GetWorld())
    {
        return false;
    }

    // Check distance to this deposit
    float DistanceToThis = FVector::Dist(GetActorLocation(), TestLocation);
    if (DistanceToThis < MinDistance)
    {
        return true;
    }

    // Check distance to other deposits
    UClass* DepositClass = AResourceDeposit::StaticClass();
    for (TActorIterator<AResourceDeposit> ActorItr(GetWorld()); ActorItr; ++ActorItr)
    {
        AResourceDeposit* OtherDeposit = *ActorItr;
        if (OtherDeposit && OtherDeposit != this)
        {
            float Distance = FVector::Dist(OtherDeposit->GetActorLocation(), TestLocation);
            if (Distance < MinDistance)
            {
                return true;
            }
        }
    }

    return false;
}

float AResourceDeposit::GetCollisionRadius() const
{
    return CollisionRadius;
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

void AResourceDeposit::SetupCollision()
{
    if (!CollisionComponent)
    {
        return;
    }

    // Set collision radius
    CollisionComponent->SetSphereRadius(CollisionRadius);
    
    // Configure collision settings
    CollisionComponent->SetCollisionEnabled(bPreventOverlapping ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
    CollisionComponent->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
    CollisionComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("ResourceDeposit: Setup collision with radius %.1f"), CollisionRadius);
}

void AResourceDeposit::UpdateCollisionSize()
{
    if (!CollisionComponent || !DepositDefinition)
    {
        return;
    }

    // Adjust collision radius based on deposit level and type
    float BaseRadius = CollisionRadius;
    float LevelMultiplier = 1.0f + (CurrentLevel - 1) * 0.2f; // 20% increase per level
    
    // Different base sizes for different deposit types
    FString DepositName = DepositDefinition->DepositName.ToString();
    if (DepositName.Contains("Mega") || DepositName.Contains("Large"))
    {
        BaseRadius *= 1.5f;
    }
    else if (DepositName.Contains("Small") || DepositName.Contains("Mini"))
    {
        BaseRadius *= 0.7f;
    }
    
    float FinalRadius = BaseRadius * LevelMultiplier;
    CollisionComponent->SetSphereRadius(FinalRadius);
    
    UE_LOG(LogTemp, VeryVerbose, TEXT("ResourceDeposit: Updated collision radius to %.1f (Level %d)"), 
           FinalRadius, CurrentLevel);
}

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
                // Check if we have space in storage
                int32 SpaceAvailable = StorageComponent->GetAvailableSpace(GetResourceType());
                int32 ActualAmount = FMath::Min(ExtractAmount, SpaceAvailable);
                
                if (ActualAmount > 0)
                {
                    StorageComponent->AddResource(GetResourceType(), ActualAmount);
                    BroadcastExtractionEvent(ActualAmount);
                }
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

    // Regeneration for renewable resources is now handled in auto extraction above
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
    // This is now handled in TickAutoExtraction for better integration with storage
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