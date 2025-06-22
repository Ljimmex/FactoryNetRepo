// BlueprintDepositManager.cpp - POPRAWIONY
// Lokalizacja: Source/FactoryNet/Private/Core/BlueprintDepositManager.cpp

#include "Core/BlueprintDepositManager.h"
#include "Core/DepositSpawnManager.h"
#include "Buildings/Base/ResourceDeposit.h"
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

    // Initialize all properties
    SpawnTrigger = ESpawnTriggerType::OnBeginPlay;
    DelayTime = 2.0f;
    DepositDensity = EDepositDensity::Normal;
    bAutoGenerateOnBeginPlay = true;
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
        
        // ✅ DODANO: Test probability generation
        UE_LOG(LogTemp, Warning, TEXT("=== TESTING PROBABILITY SYSTEM ==="));
        SpawnManager->TestProbabilityGeneration(0.3f, 100);  // Test 30% probability
        SpawnManager->TestProbabilityGeneration(0.6f, 100);  // Test 60% probability  
        SpawnManager->TestProbabilityGeneration(1.0f, 100);  // Test 100% probability
        UE_LOG(LogTemp, Warning, TEXT("=== END PROBABILITY TEST ==="));
    }

    // Apply deposit density to spawn manager
    SpawnManager->SetDepositDensity(DepositDensity);

    // ✅ DODANO: Notify Blueprint przed rozpoczęciem spawnu
    OnDepositGenerationStarted_BP();

    // Generate deposits
    SpawnManager->GenerateDepositsOnMap();
    bHasGenerated = true;

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
            if (!Deposit->IsDepleted())
            {
                ActiveCount++;
            }
            
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
            // ✅ NAPRAWIONE: Prawidłowe bindowanie eventów
            SpawnManager->OnDepositSpawned.AddDynamic(this, &ABlueprintDepositManager::OnDepositSpawned);
            SpawnManager->OnAllDepositsSpawned.AddDynamic(this, &ABlueprintDepositManager::OnAllDepositsSpawned);
            
            if (bLogSpawnProcess)
            {
                UE_LOG(LogTemp, Log, TEXT("BlueprintDepositManager: Successfully initialized SpawnManager and bound events"));
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
// BlueprintDepositManager.cpp - POPRAWIONY - Część 2

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
    SpawnRule.MinDistanceFromWater = BPRule.MinDistanceFromWater;
    SpawnRule.PreferCoastline = BPRule.bPreferCoastline;
    
    return SpawnRule;
}

// ✅ KLUCZOWA POPRAWA: Event handlers z rozszerzonym loggingiem
void ABlueprintDepositManager::OnDepositSpawned(AResourceDeposit* SpawnedDeposit, FVector SpawnLocation)
{
    if (!SpawnedDeposit)
    {
        UE_LOG(LogTemp, Error, TEXT("BlueprintDepositManager: OnDepositSpawned called with null deposit"));
        return;
    }

    // ✅ DODANO: Szczegółowe informacje o spawnie
    FString DepositName = SpawnedDeposit->GetDepositName().ToString();
    FString ResourceTypeName = TEXT("Unknown");
    
    // Pobierz nazwę typu zasobu
    FDataTableRowHandle ResourceRef = SpawnedDeposit->GetResourceType();
    if (!ResourceRef.RowName.IsNone())
    {
        ResourceTypeName = ResourceRef.RowName.ToString();
    }

    // ✅ DODANO: Informacje o poziomie i zasobach
    int32 CurrentLevel = SpawnedDeposit->GetCurrentLevel();
    int32 AvailableResources = SpawnedDeposit->GetAvailableResource();
    float ExtractionRate = SpawnedDeposit->GetCurrentExtractionRate();
    bool bIsRenewable = SpawnedDeposit->IsRenewable();

    if (bLogSpawnProcess)
    {
        UE_LOG(LogTemp, Log, TEXT("BlueprintDepositManager: ✅ SPAWNED DEPOSIT"));
        UE_LOG(LogTemp, Log, TEXT("  📍 Location: %s"), *SpawnLocation.ToString());
        UE_LOG(LogTemp, Log, TEXT("  🏭 Name: %s"), *DepositName);
        UE_LOG(LogTemp, Log, TEXT("  ⛏️  Resource: %s"), *ResourceTypeName);
        UE_LOG(LogTemp, Log, TEXT("  📊 Level: %d"), CurrentLevel);
        UE_LOG(LogTemp, Log, TEXT("  💎 Available: %d"), AvailableResources);
        UE_LOG(LogTemp, Log, TEXT("  ⚡ Rate: %.2f/s"), ExtractionRate);
        UE_LOG(LogTemp, Log, TEXT("  ♻️  Renewable: %s"), bIsRenewable ? TEXT("Yes") : TEXT("No"));
        UE_LOG(LogTemp, Log, TEXT("  ═══════════════════════════════════"));
    }

    // ✅ NAPRAWIONE: Używamy getter function zamiast bezpośredniego dostępu
    UResourceStorageComponent* Storage = SpawnedDeposit->GetStorageComponent();
    if (Storage)
    {
        int32 MaxStorage = SpawnedDeposit->GetMaxStorage();
        int32 CurrentStored = SpawnedDeposit->GetCurrentStoredAmount();
        float StoragePercentage = SpawnedDeposit->GetStoragePercentage();
        
        if (bLogSpawnProcess)
        {
            UE_LOG(LogTemp, Log, TEXT("  📦 Storage: %d/%d (%.1f%%)"), 
                   CurrentStored, MaxStorage, StoragePercentage * 100.0f);
        }
    }
    else if (bLogSpawnProcess)
    {
        UE_LOG(LogTemp, Warning, TEXT("  ⚠️ No Storage Component"));
    }

    // ✅ DODANO: Debug visualization
    if (bShowSpawnArea && GetWorld())
    {
        // Draw spawn indicator
        DrawDebugSphere(GetWorld(), SpawnLocation, 200.0f, 12, FColor::Green, false, DebugDisplayTime, 0, 8.0f);
        
        // Draw deposit name
        DrawDebugString(GetWorld(), SpawnLocation + FVector(0, 0, 250), 
                       FString::Printf(TEXT("%s\n%s"), *DepositName, *ResourceTypeName), 
                       nullptr, FColor::White, DebugDisplayTime);
                       
        // Draw extraction rate indicator
        DrawDebugString(GetWorld(), SpawnLocation + FVector(0, 0, 150), 
                       FString::Printf(TEXT("Rate: %.1f/s"), ExtractionRate), 
                       nullptr, FColor::Yellow, DebugDisplayTime);
    }

    // Notify Blueprint - przekaż wszystkie informacje
    OnDepositSpawned_BP(SpawnedDeposit, SpawnLocation);
}

void ABlueprintDepositManager::OnAllDepositsSpawned(const TArray<FSpawnedDepositInfo>& SpawnedDeposits)
{
    if (bLogSpawnProcess)
    {
        UE_LOG(LogTemp, Log, TEXT("═══════════════════════════════════"));
        UE_LOG(LogTemp, Log, TEXT("BlueprintDepositManager: 🎉 ALL DEPOSITS SPAWNED"));
        UE_LOG(LogTemp, Log, TEXT("  📊 Total Count: %d"), SpawnedDeposits.Num());
        
        // ✅ DODANO: Szczegółowe statystyki
        TMap<UDepositDefinition*, int32> TypeCounts;
        TMap<ETerrainType, int32> TerrainCounts;
        int32 RenewableCount = 0;
        int32 NonRenewableCount = 0;
        
        for (const FSpawnedDepositInfo& Info : SpawnedDeposits)
        {
            if (Info.DepositDefinition)
            {
                TypeCounts.FindOrAdd(Info.DepositDefinition, 0)++;
            }
            
            TerrainCounts.FindOrAdd(Info.TerrainType, 0)++;
            
            if (Info.SpawnedActor && Info.SpawnedActor->IsRenewable())
            {
                RenewableCount++;
            }
            else
            {
                NonRenewableCount++;
            }
        }
        
        UE_LOG(LogTemp, Log, TEXT("  ♻️  Renewable: %d | 💎 Non-renewable: %d"), RenewableCount, NonRenewableCount);
        
        // Statystyki per typ
        UE_LOG(LogTemp, Log, TEXT("  📋 BREAKDOWN BY TYPE:"));
        for (const auto& Pair : TypeCounts)
        {
            FString TypeName = Pair.Key ? Pair.Key->DepositName.ToString() : TEXT("Unknown");
            UE_LOG(LogTemp, Log, TEXT("    • %s: %d"), *TypeName, Pair.Value);
        }
        
        // Statystyki per teren
        UE_LOG(LogTemp, Log, TEXT("  🌍 BREAKDOWN BY TERRAIN:"));
        for (const auto& Pair : TerrainCounts)
        {
            FString TerrainName = UEnum::GetValueAsString(Pair.Key);
            UE_LOG(LogTemp, Log, TEXT("    • %s: %d"), *TerrainName, Pair.Value);
        }
        
        UE_LOG(LogTemp, Log, TEXT("═══════════════════════════════════"));
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

    // ✅ DODANO: Pre-spawn logging
    if (bLogSpawnProcess)
    {
        UE_LOG(LogTemp, Log, TEXT("BlueprintDepositManager: 🎯 MANUAL SPAWN REQUEST"));
        UE_LOG(LogTemp, Log, TEXT("  📍 Location: %s"), *Location.ToString());
        UE_LOG(LogTemp, Log, TEXT("  🏭 Type: %s"), *DepositType->DepositName.ToString());
    }

    AResourceDeposit* SpawnedDeposit = SpawnManager->SpawnDepositAtLocation(DepositType, Location, FRotator::ZeroRotator);
    
    if (SpawnedDeposit)
    {
        if (bLogSpawnProcess)
        {
            UE_LOG(LogTemp, Log, TEXT("  ✅ SUCCESS: Manual spawn completed"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("  ❌ FAILED: Manual spawn failed"));
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
        UE_LOG(LogTemp, Log, TEXT("BlueprintDepositManager: 📐 Set spawn area from bounds"));
        UE_LOG(LogTemp, Log, TEXT("  📍 Center: %s"), *Center.ToString());
        UE_LOG(LogTemp, Log, TEXT("  📏 Size: %s"), *Size.ToString());
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
        UE_LOG(LogTemp, Log, TEXT("BlueprintDepositManager: ➕ Added custom spawn rule"));
        UE_LOG(LogTemp, Log, TEXT("  🏭 Type: %s"), *CustomRule.DepositType->DepositName.ToString());
        UE_LOG(LogTemp, Log, TEXT("  🎲 Probability: %.3f"), CustomRule.SpawnProbability);
        UE_LOG(LogTemp, Log, TEXT("  📊 Max Count: %d"), CustomRule.MaxCount);
    }
}
// BlueprintDepositManager.cpp - POPRAWIONY - Część 3 (Final)

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
        
        if (bLogSpawnProcess)
        {
            UE_LOG(LogTemp, Log, TEXT("BlueprintDepositManager: 👁️ PREVIEW MODE ACTIVATED"));
            UE_LOG(LogTemp, Log, TEXT("  📍 Center: %s"), *Center.ToString());
            UE_LOG(LogTemp, Log, TEXT("  📏 Size: %s"), *Size.ToString());
            UE_LOG(LogTemp, Log, TEXT("  ⏱️ Display Time: %.1f seconds"), PreviewTime);
        }
    }
}

void ABlueprintDepositManager::DebugSpawnedDeposits()
{
    if (!SpawnManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("BlueprintDepositManager: SpawnManager is null"));
        return;
    }

    TArray<AResourceDeposit*> AllDeposits = SpawnManager->GetAllSpawnedDeposits();
    
    UE_LOG(LogTemp, Log, TEXT("═══════════════════════════════════"));
    UE_LOG(LogTemp, Log, TEXT("BlueprintDepositManager: 🔍 DEBUG SPAWNED DEPOSITS"));
    UE_LOG(LogTemp, Log, TEXT("  📊 Total Deposits: %d"), AllDeposits.Num());
    UE_LOG(LogTemp, Log, TEXT("═══════════════════════════════════"));

    for (int32 i = 0; i < AllDeposits.Num(); i++)
    {
        AResourceDeposit* Deposit = AllDeposits[i];
        if (IsValid(Deposit))
        {
            FVector Location = Deposit->GetActorLocation();
            FString DepositName = Deposit->GetDepositName().ToString();
            FString ResourceType = Deposit->GetResourceType().RowName.ToString();
            int32 Level = Deposit->GetCurrentLevel();
            int32 Resources = Deposit->GetAvailableResource();
            float Rate = Deposit->GetCurrentExtractionRate();
            bool bRenewable = Deposit->IsRenewable();
            bool bDepleted = Deposit->IsDepleted();
            
            // ✅ DODANO: Rozszerzone informacje debug
            UE_LOG(LogTemp, Log, TEXT("  [%d] 🏭 %s"), i + 1, *DepositName);
            UE_LOG(LogTemp, Log, TEXT("      📍 Location: %s"), *Location.ToString());
            UE_LOG(LogTemp, Log, TEXT("      ⛏️ Resource: %s"), *ResourceType);
            UE_LOG(LogTemp, Log, TEXT("      📊 Level: %d"), Level);
            UE_LOG(LogTemp, Log, TEXT("      💎 Available: %d"), Resources);
            UE_LOG(LogTemp, Log, TEXT("      ⚡ Rate: %.2f/s"), Rate);
            UE_LOG(LogTemp, Log, TEXT("      ♻️ Renewable: %s"), bRenewable ? TEXT("Yes") : TEXT("No"));
            UE_LOG(LogTemp, Log, TEXT("      ⚠️ Depleted: %s"), bDepleted ? TEXT("Yes") : TEXT("No"));
            
            // Color coding based on deposit type
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
            else if (DepositName.Contains("Wheat") || DepositName.Contains("Farm"))
            {
                DepositColor = FColor::Green;
            }
            else
            {
                DepositColor = FColor::Purple;
            }
            
            // Draw debug visualization
            if (GetWorld())
            {
                float SphereSize = bDepleted ? 100.0f : 150.0f;
                DrawDebugSphere(GetWorld(), Location, SphereSize, 8, DepositColor, false, DebugDisplayTime, 0, 5.0f);
                
                // Draw deposit info
                FString InfoText = FString::Printf(TEXT("%s\nLvl:%d | %d res\n%.1f/s"), 
                    *DepositName, Level, Resources, Rate);
                DrawDebugString(GetWorld(), Location + FVector(0, 0, 200), 
                              InfoText, nullptr, DepositColor, DebugDisplayTime);
                              
                // Draw status indicators
                if (bDepleted)
                {
                    DrawDebugString(GetWorld(), Location + FVector(0, 0, 100), 
                                  TEXT("DEPLETED"), nullptr, FColor::Red, DebugDisplayTime);
                }
                else if (bRenewable)
                {
                    DrawDebugString(GetWorld(), Location + FVector(0, 0, 100), 
                                  TEXT("RENEWABLE"), nullptr, FColor::Green, DebugDisplayTime);
                }
            }
            
            UE_LOG(LogTemp, Log, TEXT("      ───────────────────────────────"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("  [%d] ❌ Invalid deposit actor"), i + 1);
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("═══════════════════════════════════"));
}

bool ABlueprintDepositManager::ValidateSpawnConfiguration() const
{
    bool bValid = true;
    
    if (bLogSpawnProcess)
    {
        UE_LOG(LogTemp, Log, TEXT("BlueprintDepositManager: 🔍 VALIDATING CONFIGURATION..."));
    }
    
    // Check if we have spawn manager
    if (!SpawnManager)
    {
        UE_LOG(LogTemp, Error, TEXT("  ❌ SpawnManager is null"));
        bValid = false;
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("  ✅ SpawnManager: OK"));
    }
    
    // Check spawn area bounds
    if (!SpawnAreaBounds)
    {
        UE_LOG(LogTemp, Error, TEXT("  ❌ SpawnAreaBounds component is null"));
        bValid = false;
    }
    else
    {
        FVector BoxExtent = SpawnAreaBounds->GetScaledBoxExtent();
        if (BoxExtent.X <= 0 || BoxExtent.Y <= 0)
        {
            UE_LOG(LogTemp, Error, TEXT("  ❌ Invalid spawn area size: %s"), *BoxExtent.ToString());
            bValid = false;
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("  ✅ Spawn Area: %s"), *BoxExtent.ToString());
        }
    }
    
    // Check custom spawn rules
    if (!bUseDefaultSpawnRules && CustomSpawnRules.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("  ⚠️ No default rules and no custom rules defined"));
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("  ✅ Spawn Rules: %d custom + %s defaults"), 
               CustomSpawnRules.Num(), bUseDefaultSpawnRules ? TEXT("enabled") : TEXT("disabled"));
    }
    
    // Validate custom rules
    int32 ValidRules = 0;
    for (int32 i = 0; i < CustomSpawnRules.Num(); i++)
    {
        const FBlueprintSpawnRule& Rule = CustomSpawnRules[i];
        bool bRuleValid = true;
        
        if (!Rule.DepositType)
        {
            UE_LOG(LogTemp, Warning, TEXT("  ⚠️ Custom rule %d has null DepositType"), i);
            bRuleValid = false;
        }
        
        if (Rule.SpawnProbability <= 0.0f || Rule.SpawnProbability > 1.0f)
        {
            UE_LOG(LogTemp, Warning, TEXT("  ⚠️ Custom rule %d has invalid probability: %.3f"), i, Rule.SpawnProbability);
            bRuleValid = false;
        }
        
        if (Rule.MaxCount <= 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("  ⚠️ Custom rule %d has invalid MaxCount: %d"), i, Rule.MaxCount);
            bRuleValid = false;
        }
        
        if (bRuleValid)
        {
            ValidRules++;
        }
    }
    
    if (CustomSpawnRules.Num() > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("  📊 Valid Rules: %d/%d"), ValidRules, CustomSpawnRules.Num());
    }
    
    // Check delay time for delayed spawn
    if (SpawnTrigger == ESpawnTriggerType::Delayed && DelayTime <= 0.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("  ⚠️ Delayed spawn with invalid DelayTime: %.2f"), DelayTime);
    }
    else if (SpawnTrigger == ESpawnTriggerType::Delayed)
    {
        UE_LOG(LogTemp, Log, TEXT("  ✅ Delay Time: %.2f seconds"), DelayTime);
    }
    
    if (bValid)
    {
        UE_LOG(LogTemp, Log, TEXT("  🎉 Configuration validation PASSED"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("  ❌ Configuration validation FAILED"));
    }
    
    return bValid;
}

void ABlueprintDepositManager::LogConfigurationSummary() const
{
    UE_LOG(LogTemp, Log, TEXT("═══════════════════════════════════"));
    UE_LOG(LogTemp, Log, TEXT("BlueprintDepositManager: ⚙️ CONFIGURATION SUMMARY"));
    UE_LOG(LogTemp, Log, TEXT("═══════════════════════════════════"));
    
    UE_LOG(LogTemp, Log, TEXT("🚀 Spawn Trigger: %s"), 
           *UEnum::GetValueAsString(SpawnTrigger));
           
    UE_LOG(LogTemp, Log, TEXT("📊 Deposit Density: %s"), 
           *UEnum::GetValueAsString(DepositDensity));
           
    UE_LOG(LogTemp, Log, TEXT("📋 Use Default Rules: %s"), 
           bUseDefaultSpawnRules ? TEXT("✅ Yes") : TEXT("❌ No"));
           
    UE_LOG(LogTemp, Log, TEXT("🔧 Custom Rules Count: %d"), CustomSpawnRules.Num());
    
    if (SpawnTrigger == ESpawnTriggerType::Delayed)
    {
        UE_LOG(LogTemp, Log, TEXT("⏱️ Delay Time: %.2f seconds"), DelayTime);
    }
    
    UE_LOG(LogTemp, Log, TEXT("🔄 Auto Generate: %s"), 
           bAutoGenerateOnBeginPlay ? TEXT("✅ Yes") : TEXT("❌ No"));
           
    UE_LOG(LogTemp, Log, TEXT("📝 Logging: %s"), 
           bLogSpawnProcess ? TEXT("✅ Enabled") : TEXT("❌ Disabled"));
           
    UE_LOG(LogTemp, Log, TEXT("👁️ Show Area: %s"), 
           bShowSpawnArea ? TEXT("✅ Enabled") : TEXT("❌ Disabled"));
    
    if (SpawnAreaBounds)
    {
        FVector BoxExtent = SpawnAreaBounds->GetScaledBoxExtent();
        FVector SpawnSize = BoxExtent * 2.0f;
        UE_LOG(LogTemp, Log, TEXT("📐 Spawn Area Size: %.0f x %.0f x %.0f"), 
               SpawnSize.X, SpawnSize.Y, SpawnSize.Z);
    }
    
    if (bUseCustomBounds)
    {
        UE_LOG(LogTemp, Log, TEXT("🎯 Custom Bounds: %s"), *CustomSpawnCenter.ToString());
        UE_LOG(LogTemp, Log, TEXT("📏 Custom Size: %s"), *CustomSpawnSize.ToString());
    }
    
    // Custom rules summary
    if (CustomSpawnRules.Num() > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("📋 CUSTOM RULES SUMMARY:"));
        for (int32 i = 0; i < CustomSpawnRules.Num(); i++)
        {
            const FBlueprintSpawnRule& Rule = CustomSpawnRules[i];
            FString TypeName = Rule.DepositType ? Rule.DepositType->DepositName.ToString() : TEXT("NULL");
            UE_LOG(LogTemp, Log, TEXT("  [%d] %s - Prob:%.3f Max:%d Dist:%.0f"), 
                   i + 1, *TypeName, Rule.SpawnProbability, Rule.MaxCount, Rule.MinDistance);
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("═══════════════════════════════════"));
}