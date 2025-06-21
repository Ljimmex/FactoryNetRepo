// BlueprintDepositManager.cpp
// Lokalizacja: Source/FactoryNet/Private/Core/BlueprintDepositManager.cpp

#include "Core/BlueprintDepositManager.h"
#include "Core/DepositSpawnManager.h"
#include "Buildings/Base/ResourceDeposit.h"
// #include "Buildings/Base/TransportHub.h" // USUNIĘTE - jeszcze nie istnieje
#include "Components/BillboardComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ABlueprintDepositManager::ABlueprintDepositManager()
{
    PrimaryActorTick.bCanEverTick = false;

    // Create components
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
    BillboardComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
    BillboardComponent->SetupAttachment(RootComponent);

    SpawnAreaBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnAreaBounds"));
    SpawnAreaBounds->SetupAttachment(RootComponent);
    SpawnAreaBounds->SetBoxExtent(FVector(5000.0f, 5000.0f, 2500.0f));
    SpawnAreaBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // ✅ NAPRAWIONE: Dodano inicjalizację wszystkich właściwości
    SpawnTrigger = ESpawnTriggerType::OnBeginPlay;
    DelayTime = 2.0f;
    DepositDensity = EDepositDensity::Normal;
    bAutoGenerateOnBeginPlay = true;  // ✅ DODANO
    bUseDefaultSpawnRules = true;
    bLogSpawnProcess = true;
    bShowSpawnArea = true;
    bUseCustomBounds = false;
    DebugDisplayTime = 10.0f;

    // Initialize custom bounds
    CustomSpawnCenter = FVector::ZeroVector;
    CustomSpawnSize = FVector(10000.0f, 10000.0f, 5000.0f);

    // Initialize runtime data
    SpawnManager = nullptr;
    bHasGenerated = false;
}

void ABlueprintDepositManager::BeginPlay()
{
    Super::BeginPlay();

    if (bLogSpawnProcess)
    {
        UE_LOG(LogTemp, Log, TEXT("BlueprintDepositManager: BeginPlay started"));
    }

    // Initialize spawn manager
    InitializeSpawnManager();

    // Validate configuration
    if (!ValidateSpawnConfiguration())
    {
        UE_LOG(LogTemp, Error, TEXT("BlueprintDepositManager: Configuration validation failed"));
        return;
    }

    // Set spawn area from bounds
    SetSpawnAreaFromBounds();

    // Setup spawn rules
    SetupSpawnRules();

    if (bLogSpawnProcess)
    {
        LogConfigurationSummary();
    }

    // ✅ NAPRAWIONE: Używa nowej właściwości bAutoGenerateOnBeginPlay
    // Handle spawn trigger
    if (bAutoGenerateOnBeginPlay || SpawnTrigger == ESpawnTriggerType::OnBeginPlay)
    {
        GenerateDeposits();
    }
    else if (SpawnTrigger == ESpawnTriggerType::Delayed)
    {
        if (DelayTime > 0.0f)
        {
            GetWorldTimerManager().SetTimer(DelayedSpawnTimerHandle, this, 
                &ABlueprintDepositManager::GenerateDeposits, DelayTime, false);

            if (bLogSpawnProcess)
            {
                UE_LOG(LogTemp, Log, TEXT("BlueprintDepositManager: Delayed spawn scheduled for %.2f seconds"), DelayTime);
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("BlueprintDepositManager: Invalid DelayTime for delayed spawn"));
        }
    }
    else if (bLogSpawnProcess)
    {
        UE_LOG(LogTemp, Log, TEXT("BlueprintDepositManager: Manual spawn trigger - call GenerateDeposits() to spawn"));
    }
}

void ABlueprintDepositManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Clear any pending timers
    if (DelayedSpawnTimerHandle.IsValid())
    {
        GetWorldTimerManager().ClearTimer(DelayedSpawnTimerHandle);
    }

    // Unbind events
    if (SpawnManager)
    {
        SpawnManager->OnDepositSpawned.RemoveDynamic(this, &ABlueprintDepositManager::OnDepositSpawned);
        SpawnManager->OnAllDepositsSpawned.RemoveDynamic(this, &ABlueprintDepositManager::OnAllDepositsSpawned);
    }

    Super::EndPlay(EndPlayReason);
}

void ABlueprintDepositManager::GenerateDeposits()
{
    if (!SpawnManager)
    {
        UE_LOG(LogTemp, Error, TEXT("BlueprintDepositManager: SpawnManager not initialized"));
        return;
    }

    if (bHasGenerated)
    {
        if (bLogSpawnProcess)
        {
            UE_LOG(LogTemp, Warning, TEXT("BlueprintDepositManager: Deposits already generated. Use RegenerateDeposits() to regenerate."));
        }
        return;
    }

    if (bLogSpawnProcess)
    {
        UE_LOG(LogTemp, Log, TEXT("BlueprintDepositManager: Starting deposit generation..."));
    }

    // Apply deposit density to spawn manager
    SpawnManager->SetDepositDensity(DepositDensity);

    // Generate deposits
    SpawnManager->GenerateDepositsOnMap();
    bHasGenerated = true;

    // Notify Blueprint
    OnDepositGenerationStarted_BP();

    if (bLogSpawnProcess)
    {
        UE_LOG(LogTemp, Log, TEXT("BlueprintDepositManager: Deposit generation completed"));
    }
}

void ABlueprintDepositManager::RegenerateDeposits()
{
    if (!SpawnManager)
    {
        UE_LOG(LogTemp, Error, TEXT("BlueprintDepositManager: SpawnManager not initialized"));
        return;
    }

    if (bLogSpawnProcess)
    {
        UE_LOG(LogTemp, Log, TEXT("BlueprintDepositManager: Regenerating deposits..."));
    }

    // Clear existing deposits
    SpawnManager->ClearAllSpawnedDeposits();

    // Reset flag and regenerate
    bHasGenerated = false;
    GenerateDeposits();

    // Notify Blueprint
    OnDepositsRegenerated_BP();
}

void ABlueprintDepositManager::ClearAllDeposits()
{
    if (!SpawnManager)
    {
        UE_LOG(LogTemp, Error, TEXT("BlueprintDepositManager: SpawnManager not initialized"));
        return;
    }

    if (bLogSpawnProcess)
    {
        UE_LOG(LogTemp, Log, TEXT("BlueprintDepositManager: Clearing all deposits..."));
    }

    SpawnManager->ClearAllSpawnedDeposits();
    bHasGenerated = false;

    // Notify Blueprint
    OnDepositsCleared_BP();
}

// ================================
// ✅ NAPRAWIONE: Funkcja GetDepositInfo - usunięto odwołania do nieistniejących funkcji
// Błędy które naprawiono:
// 1. "Cannot resolve symbol 'IsActive'" - zastąpiono sprawdzaniem IsValid()
// 2. "Cannot resolve symbol 'GetCurrentReserves'" - zastąpiono GetAvailableResource()
// 3. "Cannot resolve symbol 'GetDepositDefinition'" - użyto bezpośredniej referencji z DepositSpawnManager
// 4. "Cannot resolve symbol 'DepositColor'" - usunięto, nie istnieje w DepositDefinition
// ================================

FDepositInfo ABlueprintDepositManager::GetDepositInfo(UDepositDefinition* DepositType)
{
    FDepositInfo DepositInfo;

    if (!DepositType)
    {
        UE_LOG(LogTemp, Warning, TEXT("BlueprintDepositManager: DepositType is null"));
        return DepositInfo;
    }

    if (!SpawnManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("BlueprintDepositManager: SpawnManager is null"));
        return DepositInfo;
    }

    // ✅ NAPRAWIONE: Używamy publicznej funkcji GetDepositsByType zamiast bezpośredniego dostępu do prywatnych członków
    TArray<AResourceDeposit*> DepositsOfType = SpawnManager->GetDepositsByType(DepositType);

    DepositInfo.DepositType = DepositType;
    DepositInfo.TotalCount = DepositsOfType.Num();

    // Calculate total resources and active count
    int32 ActiveCount = 0;
    int32 TotalResources = 0;

    for (AResourceDeposit* Deposit : DepositsOfType)
    {
        if (IsValid(Deposit))
        {
            // ✅ NAPRAWIONE: Zamiast IsActive() używamy IsValid() i sprawdzamy czy nie jest depleted
            if (!Deposit->IsDepleted())
            {
                ActiveCount++;
            }
            
            // ✅ NAPRAWIONE: Zamiast GetCurrentReserves() używamy GetAvailableResource()
            TotalResources += Deposit->GetAvailableResource();
        }
    }

    DepositInfo.ActiveCount = ActiveCount;
    DepositInfo.TotalResources = TotalResources;

    return DepositInfo;
}

TArray<AResourceDeposit*> ABlueprintDepositManager::GetAllSpawnedDeposits()
{
    if (!SpawnManager)
    {
        return TArray<AResourceDeposit*>();
    }

    return SpawnManager->GetAllSpawnedDeposits();
}

TArray<AResourceDeposit*> ABlueprintDepositManager::GetDepositsByType(UDepositDefinition* DepositType)
{
    if (!SpawnManager)
    {
        return TArray<AResourceDeposit*>();
    }

    return SpawnManager->GetDepositsByType(DepositType);
}

AResourceDeposit* ABlueprintDepositManager::GetNearestDeposit(const FVector& Location, UDepositDefinition* DepositType)
{
    if (!SpawnManager)
    {
        return nullptr;
    }

    return SpawnManager->GetNearestDepositOfType(Location, DepositType);
}

// === PRIVATE FUNCTIONS ===

void ABlueprintDepositManager::InitializeSpawnManager()
{
    if (GetWorld())
    {
        SpawnManager = GetWorld()->GetSubsystem<UDepositSpawnManager>();
        
        if (SpawnManager)
        {
            // Bind to events
            SpawnManager->OnDepositSpawned.AddDynamic(this, &ABlueprintDepositManager::OnDepositSpawned);
            SpawnManager->OnAllDepositsSpawned.AddDynamic(this, &ABlueprintDepositManager::OnAllDepositsSpawned);
            
            if (bLogSpawnProcess)
            {
                UE_LOG(LogTemp, Log, TEXT("BlueprintDepositManager: Successfully initialized SpawnManager"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("BlueprintDepositManager: Failed to get DepositSpawnManager subsystem"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("BlueprintDepositManager: World is null during initialization"));
    }
}

void ABlueprintDepositManager::SetupSpawnRules()
{
    if (!SpawnManager)
    {
        return;
    }

    // Clear existing rules if not using defaults
    if (!bUseDefaultSpawnRules)
    {
        SpawnManager->ClearSpawnRules();
    }

    // Add custom rules
    int32 AddedRules = 0;
    for (const FBlueprintSpawnRule& BPRule : CustomSpawnRules)
    {
        if (BPRule.DepositType)
        {
            FDepositSpawnRule SpawnRule = ConvertBlueprintRule(BPRule);
            SpawnManager->AddSpawnRule(SpawnRule);
            AddedRules++;
        }
    }

    if (bLogSpawnProcess)
    {
        UE_LOG(LogTemp, Log, TEXT("BlueprintDepositManager: Setup %d custom spawn rules (UseDefaults: %s)"), 
               AddedRules, bUseDefaultSpawnRules ? TEXT("true") : TEXT("false"));
    }
}

// ✅ DODANO: Nową funkcję ConvertBlueprintRule z obsługą nowych właściwości
FDepositSpawnRule ABlueprintDepositManager::ConvertBlueprintRule(const FBlueprintSpawnRule& BPRule)
{
    FDepositSpawnRule SpawnRule;
    
    SpawnRule.DepositDefinition = BPRule.DepositType;
    SpawnRule.SpawnProbability = BPRule.SpawnProbability;
    SpawnRule.MaxDepositCount = BPRule.MaxCount;
    SpawnRule.MinDistanceFromOthers = BPRule.MinDistance;
    SpawnRule.PreferredTerrainTypes = BPRule.TerrainTypes;
    SpawnRule.MinElevation = BPRule.MinElevation;
    SpawnRule.MaxElevation = BPRule.MaxElevation;
    SpawnRule.MinDistanceFromWater = BPRule.MinDistanceFromWater;  // ✅ DODANO
    SpawnRule.PreferCoastline = BPRule.bPreferCoastline;           // ✅ DODANO
    
    return SpawnRule;
}

void ABlueprintDepositManager::OnDepositSpawned(AResourceDeposit* SpawnedDeposit, FVector SpawnLocation)
{
    if (SpawnedDeposit && bLogSpawnProcess)
    {
        UE_LOG(LogTemp, Log, TEXT("BlueprintDepositManager: Deposit spawned at %s"), *SpawnLocation.ToString());
    }

    // Notify Blueprint
    OnDepositSpawned_BP(SpawnedDeposit, SpawnLocation);
}

void ABlueprintDepositManager::OnAllDepositsSpawned(const TArray<FSpawnedDepositInfo>& SpawnedDeposits)
{
    if (bLogSpawnProcess)
    {
        UE_LOG(LogTemp, Log, TEXT("BlueprintDepositManager: All deposits spawned. Total: %d"), SpawnedDeposits.Num());
    }

    // Notify Blueprint
    OnAllDepositsSpawned_BP(SpawnedDeposits);
}

void ABlueprintDepositManager::UpdateSpawnAreaVisualization()
{
    if (!GetWorld() || !bShowSpawnArea)
    {
        return;
    }

    FVector Center;
    FVector Size;

    if (bUseCustomBounds)
    {
        Center = CustomSpawnCenter;
        Size = CustomSpawnSize;
    }
    else if (SpawnAreaBounds)
    {
        Center = GetActorLocation() + SpawnAreaBounds->GetRelativeLocation();
        Size = SpawnAreaBounds->GetScaledBoxExtent() * 2.0f;
    }
    else
    {
        return;
    }

    // Draw debug box
    DrawDebugBox(GetWorld(), Center, Size * 0.5f, FColor::Yellow, false, DebugDisplayTime, 0, 10.0f);
    
    // Draw center point
    DrawDebugSphere(GetWorld(), Center, 100.0f, 8, FColor::Red, false, DebugDisplayTime, 0, 5.0f);
    
    // Draw spawn area info
    DrawDebugString(GetWorld(), Center + FVector(0, 0, Size.Z * 0.6f), 
                   FString::Printf(TEXT("Spawn Area: %.0fx%.0f"), Size.X, Size.Y), 
                   nullptr, FColor::White, DebugDisplayTime);
}

AResourceDeposit* ABlueprintDepositManager::SpawnDepositAtLocation(UDepositDefinition* DepositType, const FVector& Location)
{
    if (!SpawnManager || !DepositType)
    {
        return nullptr;
    }

    AResourceDeposit* SpawnedDeposit = SpawnManager->SpawnDepositAtLocation(DepositType, Location, FRotator::ZeroRotator);
    
    if (SpawnedDeposit && bLogSpawnProcess)
    {
        FString DepositName = DepositType->DepositName.ToString();
        if (DepositName.IsEmpty())
        {
            DepositName = TEXT("Unknown");
        }
        
        if (bLogSpawnProcess)
        {
            UE_LOG(LogTemp, Log, TEXT("BlueprintDepositManager: Manually spawned %s at %s"), 
                   *DepositName, *Location.ToString());
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("BlueprintDepositManager: Failed to spawn deposit"));
    }

    return SpawnedDeposit;
}

void ABlueprintDepositManager::SetSpawnAreaFromBounds()
{
    if (!SpawnAreaBounds)
    {
        UE_LOG(LogTemp, Error, TEXT("BlueprintDepositManager: SpawnAreaBounds component is null"));
        return;
    }

    FVector Center = GetActorLocation() + SpawnAreaBounds->GetRelativeLocation();
    FVector Size = SpawnAreaBounds->GetScaledBoxExtent() * 2.0f;

    if (SpawnManager)
    {
        SpawnManager->SetSpawnArea(Center, Size);
        OnSpawnAreaChanged_BP(Center, Size);
    }

    if (bLogSpawnProcess)
    {
        UE_LOG(LogTemp, Log, TEXT("BlueprintDepositManager: Set spawn area from bounds - Center: %s, Size: %s"), 
               *Center.ToString(), *Size.ToString());
    }
}

void ABlueprintDepositManager::AddCustomSpawnRule(const FBlueprintSpawnRule& CustomRule)
{
    if (!CustomRule.DepositType)
    {
        UE_LOG(LogTemp, Warning, TEXT("BlueprintDepositManager: Cannot add spawn rule with null DepositType"));
        return;
    }

    CustomSpawnRules.Add(CustomRule);
    
    if (bLogSpawnProcess)
    {
        UE_LOG(LogTemp, Log, TEXT("BlueprintDepositManager: Added custom spawn rule for %s (Probability: %.3f, Max: %d)"), 
               *CustomRule.DepositType->DepositName.ToString(),
               CustomRule.SpawnProbability,
               CustomRule.MaxCount);
    }
}

void ABlueprintDepositManager::PreviewSpawnArea()
{
    UpdateSpawnAreaVisualization();
    
    // Draw debug visualization for preview
    if (GetWorld())
    {
        FVector Center;
        FVector Size;

        if (bUseCustomBounds)
        {
            Center = CustomSpawnCenter;
            Size = CustomSpawnSize;
        }
        else if (SpawnAreaBounds)
        {
            Center = GetActorLocation() + SpawnAreaBounds->GetRelativeLocation();
            Size = SpawnAreaBounds->GetScaledBoxExtent() * 2.0f;
        }
        else
        {
            return;
        }

        // Extended visualization for preview
        float PreviewTime = 30.0f;
        
        // Draw main area
        DrawDebugBox(GetWorld(), Center, Size * 0.5f, FColor::Cyan, false, PreviewTime, 0, 15.0f);
        
        // Draw grid
        int32 GridLines = 5;
        for (int32 i = 0; i <= GridLines; i++)
        {
            float Alpha = (float)i / GridLines;
            
            // X lines
            FVector StartX = Center + FVector(-Size.X * 0.5f, -Size.Y * 0.5f + Size.Y * Alpha, 0);
            FVector EndX = Center + FVector(Size.X * 0.5f, -Size.Y * 0.5f + Size.Y * Alpha, 0);
            DrawDebugLine(GetWorld(), StartX, EndX, FColor::Green, false, PreviewTime, 0, 2.0f);
            
            // Y lines
            FVector StartY = Center + FVector(-Size.X * 0.5f + Size.X * Alpha, -Size.Y * 0.5f, 0);
            FVector EndY = Center + FVector(-Size.X * 0.5f + Size.X * Alpha, Size.Y * 0.5f, 0);
            DrawDebugLine(GetWorld(), StartY, EndY, FColor::Green, false, PreviewTime, 0, 2.0f);
        }
    }
}

// ================================
// ✅ NAPRAWIONE: Funkcja DebugSpawnedDeposits - usunięto odwołania do nieistniejących funkcji
// Błędy które naprawiono:
// 1. "Cannot resolve symbol 'GetDepositDefinition'" - już nie używamy tej funkcji
// 2. "Cannot resolve symbol 'DepositColor'" - zastąpiono prostym kolorowaniem na podstawie nazwy
// ================================

void ABlueprintDepositManager::DebugSpawnedDeposits()
{
    if (!SpawnManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("BlueprintDepositManager: SpawnManager is null"));
        return;
    }

    TArray<AResourceDeposit*> AllDeposits = SpawnManager->GetAllSpawnedDeposits();
    
    UE_LOG(LogTemp, Log, TEXT("=== Spawned Deposits Debug ==="));
    UE_LOG(LogTemp, Log, TEXT("Total Deposits: %d"), AllDeposits.Num());

    for (int32 i = 0; i < AllDeposits.Num(); i++)
    {
        AResourceDeposit* Deposit = AllDeposits[i];
        if (IsValid(Deposit))
        {
            FVector Location = Deposit->GetActorLocation();
            FString DepositName = Deposit->GetDepositName().ToString();
            
            // ✅ NAPRAWIONE: Proste kolorowanie na podstawie nazwy zamiast nieistniejącego DepositColor
            FColor DepositColor = FColor::White;
            if (DepositName.Contains("Iron"))
            {
                DepositColor = FColor::Red;
            }
            else if (DepositName.Contains("Oil"))
            {
                DepositColor = FColor::Black;
            }
            else if (DepositName.Contains("Coal"))
            {
                DepositColor = FColor::Silver;
            }
            else if (DepositName.Contains("Gold"))
            {
                DepositColor = FColor::Yellow;
            }
            else if (DepositName.Contains("Stone"))
            {
                DepositColor = FColor::Cyan;
            }
            else
            {
                DepositColor = FColor::Green;
            }
            
            UE_LOG(LogTemp, Log, TEXT("  [%d] %s at %s"), i, *DepositName, *Location.ToString());
            
            // Draw debug visualization
            if (GetWorld())
            {
                DrawDebugSphere(GetWorld(), Location, 150.0f, 8, DepositColor, false, DebugDisplayTime, 0, 5.0f);
                
                // Draw deposit name
                DrawDebugString(GetWorld(), Location + FVector(0, 0, 200), 
                              DepositName, nullptr, DepositColor, DebugDisplayTime);
            }
        }
    }
}

bool ABlueprintDepositManager::ValidateSpawnConfiguration() const
{
    bool bValid = true;
    
    // Check if we have spawn manager
    if (!SpawnManager)
    {
        UE_LOG(LogTemp, Error, TEXT("BlueprintDepositManager: SpawnManager is null"));
        bValid = false;
    }
    
    // Check spawn area bounds
    if (!SpawnAreaBounds)
    {
        UE_LOG(LogTemp, Error, TEXT("BlueprintDepositManager: SpawnAreaBounds component is null"));
        bValid = false;
    }
    else
    {
        FVector BoxExtent = SpawnAreaBounds->GetScaledBoxExtent();
        if (BoxExtent.X <= 0 || BoxExtent.Y <= 0)
        {
            UE_LOG(LogTemp, Error, TEXT("BlueprintDepositManager: Invalid spawn area size: %s"), *BoxExtent.ToString());
            bValid = false;
        }
    }
    
    // Check custom spawn rules
    if (!bUseDefaultSpawnRules && CustomSpawnRules.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("BlueprintDepositManager: No default rules and no custom rules defined"));
    }
    
    // Validate custom rules
    for (int32 i = 0; i < CustomSpawnRules.Num(); i++)
    {
        const FBlueprintSpawnRule& Rule = CustomSpawnRules[i];
        
        if (!Rule.DepositType)
        {
            UE_LOG(LogTemp, Warning, TEXT("BlueprintDepositManager: Custom rule %d has null DepositType"), i);
        }
        
        if (Rule.SpawnProbability <= 0.0f || Rule.SpawnProbability > 1.0f)
        {
            UE_LOG(LogTemp, Warning, TEXT("BlueprintDepositManager: Custom rule %d has invalid probability: %.3f"), i, Rule.SpawnProbability);
        }
        
        if (Rule.MaxCount <= 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("BlueprintDepositManager: Custom rule %d has invalid MaxCount: %d"), i, Rule.MaxCount);
        }
    }
    
    // Check delay time for delayed spawn
    if (SpawnTrigger == ESpawnTriggerType::Delayed && DelayTime <= 0.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("BlueprintDepositManager: Delayed spawn with invalid DelayTime: %.2f"), DelayTime);
    }
    
    return bValid;
}

void ABlueprintDepositManager::LogConfigurationSummary() const
{
    UE_LOG(LogTemp, Log, TEXT("=== BlueprintDepositManager Configuration ==="));
    UE_LOG(LogTemp, Log, TEXT("Spawn Trigger: %s"), 
           *UEnum::GetValueAsString(SpawnTrigger));
    UE_LOG(LogTemp, Log, TEXT("Deposit Density: %s"), 
           *UEnum::GetValueAsString(DepositDensity));
    UE_LOG(LogTemp, Log, TEXT("Use Default Rules: %s"), 
           bUseDefaultSpawnRules ? TEXT("true") : TEXT("false"));
    UE_LOG(LogTemp, Log, TEXT("Custom Rules Count: %d"), CustomSpawnRules.Num());
    UE_LOG(LogTemp, Log, TEXT("Delay Time: %.2f seconds"), DelayTime);
    
    if (SpawnAreaBounds)
    {
        FVector BoxExtent = SpawnAreaBounds->GetScaledBoxExtent();
        UE_LOG(LogTemp, Log, TEXT("Spawn Area Size: %s"), *BoxExtent.ToString());
    }
}