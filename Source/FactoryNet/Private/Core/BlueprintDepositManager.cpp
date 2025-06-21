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

    // Create root component
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

    // Create billboard component for editor visibility
    BillboardComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
    BillboardComponent->SetupAttachment(RootComponent);
    BillboardComponent->SetHiddenInGame(true);
    BillboardComponent->SetUsingAbsoluteScale(true);

    // Create spawn area bounds
    SpawnAreaBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnAreaBounds"));
    SpawnAreaBounds->SetupAttachment(RootComponent);
    SpawnAreaBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SpawnAreaBounds->SetBoxExtent(FVector(5000.0f, 5000.0f, 2500.0f));
    SpawnAreaBounds->SetHiddenInGame(false);
    SpawnAreaBounds->SetVisibility(true);
    SpawnAreaBounds->SetLineThickness(5.0f);

    // Initialize default values
    SpawnManager = nullptr;
    bHasGenerated = false;
    SpawnTrigger = ESpawnTriggerType::OnBeginPlay;
    DelayTime = 2.0f;
    DepositDensity = EDepositDensity::Normal;
    bUseDefaultSpawnRules = true;
    bAutoConnectToNearestHub = false; // ZMIENIONE: wyłączone domyślnie (brak TransportHub)
    HubSearchRadius = 5000.0f;
    bAutoCreateHubsIfNeeded = false;
    MaxSpawnAttempts = 1000;
    GridResolution = 50;
    bUseCustomBounds = false;
    CustomSpawnCenter = FVector::ZeroVector;
    CustomSpawnSize = FVector(10000.0f, 10000.0f, 5000.0f);
    bShowSpawnArea = true;
    bShowDebugInfo = false;
    bLogSpawnProcess = true;
    bDrawDebugSpheres = false;
    DebugDisplayTime = 60.0f;
}

void ABlueprintDepositManager::BeginPlay()
{
    Super::BeginPlay();

    // Initialize spawn manager
    InitializeSpawnManager();

    // Validate configuration
    if (!ValidateSpawnConfiguration())
    {
        UE_LOG(LogTemp, Error, TEXT("BlueprintDepositManager: Invalid configuration detected"));
        return;
    }

    // Log configuration summary
    if (bLogSpawnProcess)
    {
        LogConfigurationSummary();
    }

    // Handle different spawn triggers
    switch (SpawnTrigger)
    {
        case ESpawnTriggerType::OnBeginPlay:
            GenerateDeposits();
            break;

        case ESpawnTriggerType::Delayed:
            if (DelayTime > 0.0f)
            {
                GetWorldTimerManager().SetTimer(
                    FTimerHandle(), 
                    this, 
                    &ABlueprintDepositManager::DelayedGeneration, 
                    DelayTime, 
                    false
                );

                if (bLogSpawnProcess)
                {
                    UE_LOG(LogTemp, Log, TEXT("BlueprintDepositManager: Scheduled delayed generation in %.1f seconds"), DelayTime);
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("BlueprintDepositManager: DelayTime must be > 0 for delayed generation"));
            }
            break;

        case ESpawnTriggerType::Manual:
            if (bLogSpawnProcess)
            {
                UE_LOG(LogTemp, Log, TEXT("BlueprintDepositManager: Manual spawn mode - call GenerateDeposits() when ready"));
            }
            break;

        case ESpawnTriggerType::OnPlayerEnter:
            // TODO: Implement player detection logic when needed
            if (bLogSpawnProcess)
            {
                UE_LOG(LogTemp, Log, TEXT("BlueprintDepositManager: Player enter detection not implemented yet"));
            }
            break;
    }

    // Update spawn area visualization
    if (bShowSpawnArea)
    {
        UpdateSpawnAreaVisualization();
    }
}

void ABlueprintDepositManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
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

    // Setup spawn area and rules
    SetupSpawnArea();
    SetupSpawnRules();

    // Apply density multiplier to spawn manager
    SpawnManager->DepositDensity = DepositDensity;
    SpawnManager->MaxSpawnAttempts = MaxSpawnAttempts;
    SpawnManager->GridResolution = GridResolution;

    // Generate deposits
    SpawnManager->GenerateDepositsOnMap();
    bHasGenerated = true;

    // Auto-connect to hubs if enabled (skip for now - no TransportHub)
    if (bAutoConnectToNearestHub)
    {
        if (bLogSpawnProcess)
        {
            UE_LOG(LogTemp, Log, TEXT("BlueprintDepositManager: Hub connection skipped - TransportHub not implemented yet"));
        }
        // ConnectDepositsToHubs(); // ZAKOMENTOWANE - zostanie odblokowane gdy TransportHub będzie gotowy
    }

    // Draw debug spheres if enabled
    if (bDrawDebugSpheres)
    {
        DrawDebugDeposits();
    }
}

void ABlueprintDepositManager::ClearAllDeposits()
{
    if (!SpawnManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("BlueprintDepositManager: SpawnManager not available"));
        return;
    }

    if (bLogSpawnProcess)
    {
        UE_LOG(LogTemp, Log, TEXT("BlueprintDepositManager: Clearing %d deposits"), SpawnedDeposits.Num());
    }

    SpawnManager->ClearAllSpawnedDeposits();
    SpawnedDeposits.Empty();
    bHasGenerated = false;

    if (bLogSpawnProcess)
    {
        UE_LOG(LogTemp, Log, TEXT("BlueprintDepositManager: All deposits cleared"));
    }
}

void ABlueprintDepositManager::RegenerateDeposits()
{
    if (bLogSpawnProcess)
    {
        UE_LOG(LogTemp, Log, TEXT("BlueprintDepositManager: Regenerating deposits..."));
    }

    ClearAllDeposits();
    GenerateDeposits();
}

AResourceDeposit* ABlueprintDepositManager::SpawnDepositAt(UDepositDefinition* DepositType, FVector Location)
{
    if (!SpawnManager)
    {
        UE_LOG(LogTemp, Error, TEXT("BlueprintDepositManager: SpawnManager not available"));
        return nullptr;
    }

    if (!DepositType)
    {
        UE_LOG(LogTemp, Error, TEXT("BlueprintDepositManager: DepositType is null"));
        return nullptr;
    }

    AResourceDeposit* SpawnedDeposit = SpawnManager->SpawnDepositAtLocation(DepositType, Location);
    
    if (SpawnedDeposit)
    {
        SpawnedDeposits.Add(SpawnedDeposit);
        
        // Auto-connect to nearest hub if enabled (skip for now - no TransportHub)
        if (bAutoConnectToNearestHub)
        {
            if (bLogSpawnProcess)
            {
                UE_LOG(LogTemp, Log, TEXT("BlueprintDepositManager: Hub connection skipped for manually spawned deposit - TransportHub not implemented yet"));
            }
            
            // TODO: Uncomment when TransportHub is implemented
            /*
            ATransportHub* NearestHub = FindNearestHub(Location);
            if (NearestHub)
            {
                float Distance = FVector::Dist(Location, NearestHub->GetActorLocation());
                if (Distance <= HubSearchRadius)
                {
                    SpawnedDeposit->ConnectToHub(NearestHub);
                    
                    if (bLogSpawnProcess)
                    {
                        UE_LOG(LogTemp, Log, TEXT("BlueprintDepositManager: Auto-connected spawned deposit to hub at distance %.0f"), Distance);
                    }
                }
            }
            else if (bAutoCreateHubsIfNeeded)
            {
                SpawnHubForDeposit(SpawnedDeposit);
            }
            */
        }

        if (bLogSpawnProcess)
        {
            UE_LOG(LogTemp, Log, TEXT("BlueprintDepositManager: Manually spawned %s at %s"), 
                   *DepositType->DepositName.ToString(), *Location.ToString());
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
        else
        {
            Center = GetActorLocation() + SpawnAreaBounds->GetRelativeLocation();
            Size = SpawnAreaBounds->GetScaledBoxExtent() * 2.0f;
        }

        // Draw preview box
        DrawDebugBox(GetWorld(), Center, Size * 0.5f, FColor::Yellow, false, DebugDisplayTime, 0, 10.0f);
        
        // Draw center point
        DrawDebugSphere(GetWorld(), Center, 100.0f, 8, FColor::Orange, false, DebugDisplayTime, 0, 5.0f);
        
        // Draw corner markers
        FVector HalfSize = Size * 0.5f;
        TArray<FVector> Corners = {
            Center + FVector(-HalfSize.X, -HalfSize.Y, 0),
            Center + FVector(HalfSize.X, -HalfSize.Y, 0),
            Center + FVector(HalfSize.X, HalfSize.Y, 0),
            Center + FVector(-HalfSize.X, HalfSize.Y, 0)
        };

        for (const FVector& Corner : Corners)
        {
            DrawDebugSphere(GetWorld(), Corner, 50.0f, 6, FColor::Red, false, DebugDisplayTime, 0, 3.0f);
        }

        // Draw info text
        DrawDebugString(GetWorld(), Center + FVector(0, 0, Size.Z * 0.6f), 
                       FString::Printf(TEXT("Deposit Spawn Area Preview\nSize: %.0fx%.0fx%.0f"), Size.X, Size.Y, Size.Z),
                       nullptr, FColor::Yellow, DebugDisplayTime);

        if (bLogSpawnProcess)
        {
            UE_LOG(LogTemp, Log, TEXT("BlueprintDepositManager: Previewing spawn area for %.1f seconds"), DebugDisplayTime);
        }
    }
}

int32 ABlueprintDepositManager::GetTotalSpawnedDeposits() const
{
    return SpawnedDeposits.Num();
}

TArray<AResourceDeposit*> ABlueprintDepositManager::GetSpawnedDepositsByType(UDepositDefinition* DepositType) const
{
    if (!SpawnManager || !DepositType)
    {
        return TArray<AResourceDeposit*>();
    }

    return SpawnManager->GetDepositsByType(DepositType);
}

AResourceDeposit* ABlueprintDepositManager::FindNearestDeposit(FVector Location, UDepositDefinition* DepositType) const
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
               AddedRules, bUseDefaultSpawnRules ? TEXT("Yes") : TEXT("No"));
    }
}

void ABlueprintDepositManager::SetupSpawnArea()
{
    if (!SpawnManager)
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
    else
    {
        Center = GetActorLocation() + SpawnAreaBounds->GetRelativeLocation();
        Size = SpawnAreaBounds->GetScaledBoxExtent() * 2.0f;
    }

    SpawnManager->SetSpawnArea(Center, Size);

    if (bLogSpawnProcess)
    {
        UE_LOG(LogTemp, Log, TEXT("BlueprintDepositManager: Setup spawn area - Center: %s, Size: %s"), 
               *Center.ToString(), *Size.ToString());
    }
}

// ZAKOMENTOWANE FUNKCJE ZWIĄZANE Z TRANSPORTHUB (do użycia gdy TransportHub będzie gotowy)
/*
void ABlueprintDepositManager::ConnectDepositsToHubs()
{
    if (!SpawnManager)
    {
        return;
    }

    TArray<AResourceDeposit*> AllDeposits = SpawnManager->GetAllSpawnedDeposits();
    int32 ConnectedCount = 0;
    int32 HubsCreatedCount = 0;
    
    for (AResourceDeposit* Deposit : AllDeposits)
    {
        if (!Deposit || !Deposit->RequiresHub()) 
        {
            continue;
        }

        ATransportHub* NearestHub = FindNearestHub(Deposit->GetActorLocation());
        
        if (NearestHub)
        {
            float Distance = FVector::Dist(Deposit->GetActorLocation(), 
                                         NearestHub->GetActorLocation());
            
            if (Distance <= HubSearchRadius)
            {
                Deposit->ConnectToHub(NearestHub);
                ConnectedCount++;
                
                if (bLogSpawnProcess)
                {
                    UE_LOG(LogTemp, VeryVerbose, TEXT("BlueprintDepositManager: Connected %s to Hub at distance %.0f"), 
                        *Deposit->GetDepositName().ToString(), Distance);
                }
            }
        }
        else if (bAutoCreateHubsIfNeeded)
        {
            SpawnHubForDeposit(Deposit);
            HubsCreatedCount++;
        }
    }

    if (bLogSpawnProcess)
    {
        UE_LOG(LogTemp, Log, TEXT("BlueprintDepositManager: Connected %d deposits to hubs, created %d new hubs"), 
               ConnectedCount, HubsCreatedCount);
    }
}

ATransportHub* ABlueprintDepositManager::FindNearestHub(const FVector& Location) const
{
    TArray<AActor*> FoundHubs;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATransportHub::StaticClass(), FoundHubs);
    
    ATransportHub* NearestHub = nullptr;
    float NearestDistance = FLT_MAX;
    
    for (AActor* Actor : FoundHubs)
    {
        ATransportHub* Hub = Cast<ATransportHub>(Actor);
        if (Hub)
        {
            float Distance = FVector::Dist(Location, Hub->GetActorLocation());
            if (Distance < NearestDistance && Distance <= HubSearchRadius)
            {
                NearestDistance = Distance;
                NearestHub = Hub;
            }
        }
    }
    
    return NearestHub;
}

void ABlueprintDepositManager::SpawnHubForDeposit(AResourceDeposit* Deposit)
{
    if (!Deposit)
    {
        return;
    }

    // TODO: Implement hub spawning logic
    // This would require access to HubDefinition and hub spawning system
    // For now, just log the request
    
    if (bLogSpawnProcess)
    {
        UE_LOG(LogTemp, Log, TEXT("BlueprintDepositManager: Auto-spawn hub requested for %s (not implemented)"), 
               *Deposit->GetDepositName().ToString());
    }

    // Future implementation would:
    // 1. Get default hub definition from DataTableManager
    // 2. Find suitable location near deposit
    // 3. Spawn hub actor
    // 4. Connect deposit to new hub
}
*/

void ABlueprintDepositManager::OnDepositSpawned(AResourceDeposit* Deposit, FVector Location)
{
    if (Deposit)
    {
        SpawnedDeposits.AddUnique(Deposit);
        
        // Call Blueprint event
        OnDepositSpawned_BP(Deposit, Location);
        
        if (bLogSpawnProcess)
        {
            UE_LOG(LogTemp, VeryVerbose, TEXT("BlueprintDepositManager: Deposit spawned - %s at %s"), 
                   *Deposit->GetDepositName().ToString(), *Location.ToString());
        }
    }
}

void ABlueprintDepositManager::OnAllDepositsSpawned(const TArray<FSpawnedDepositInfo>& SpawnedInfo)
{
    int32 TotalSpawned = SpawnedInfo.Num();
    
    // Call Blueprint event
    OnAllDepositsGenerated_BP(TotalSpawned);
    
    // Process spawn rules for Blueprint events
    TMap<UDepositDefinition*, int32> DepositCounts;
    for (const FSpawnedDepositInfo& Info : SpawnedInfo)
    {
        if (Info.DepositDefinition)
        {
            DepositCounts.FindOrAdd(Info.DepositDefinition)++;
        }
    }

    // Trigger Blueprint events for each deposit type
    for (const auto& Pair : DepositCounts)
    {
        int32 MaxCount = 0;
        for (const FBlueprintSpawnRule& Rule : CustomSpawnRules)
        {
            if (Rule.DepositType == Pair.Key)
            {
                MaxCount = Rule.MaxCount;
                break;
            }
        }
        
        OnSpawnRuleProcessed_BP(Pair.Key, Pair.Value, MaxCount);
    }
    
    if (bLogSpawnProcess)
    {
        UE_LOG(LogTemp, Log, TEXT("BlueprintDepositManager: All deposits generated - Total: %d"), TotalSpawned);
        
        // Log breakdown by type
        for (const auto& Pair : DepositCounts)
        {
            UE_LOG(LogTemp, Log, TEXT("  %s: %d deposits"), 
                   *Pair.Key->DepositName.ToString(), Pair.Value);
        }
    }
}

void ABlueprintDepositManager::DelayedGeneration()
{
    if (bLogSpawnProcess)
    {
        UE_LOG(LogTemp, Log, TEXT("BlueprintDepositManager: Executing delayed generation"));
    }
    
    GenerateDeposits();
}

FDepositSpawnRule ABlueprintDepositManager::ConvertBlueprintRule(const FBlueprintSpawnRule& BPRule) const
{
    FDepositSpawnRule SpawnRule;
    SpawnRule.DepositDefinition = BPRule.DepositType;
    SpawnRule.SpawnProbability = BPRule.SpawnProbability;
    SpawnRule.MaxDepositCount = BPRule.MaxCount;
    SpawnRule.MinDistanceFromOthers = BPRule.MinDistance;
    SpawnRule.PreferredTerrainTypes = BPRule.TerrainTypes;
    SpawnRule.MinElevation = BPRule.MinElevation;
    SpawnRule.MaxElevation = BPRule.MaxElevation;
    SpawnRule.PreferCoastline = BPRule.bPreferCoastline;
    
    return SpawnRule;
}

void ABlueprintDepositManager::UpdateSpawnAreaVisualization()
{
    if (!SpawnAreaBounds)
    {
        return;
    }

    // Update visibility based on settings
    SpawnAreaBounds->SetVisibility(bShowSpawnArea);
    SpawnAreaBounds->SetHiddenInGame(!bShowSpawnArea);
    
    // Update box color based on state
    if (bShowSpawnArea)
    {
        FColor BoxColor = bHasGenerated ? FColor::Green : FColor::Yellow;
        SpawnAreaBounds->ShapeColor = BoxColor;
    }
}

void ABlueprintDepositManager::DrawDebugDeposits() const
{
    if (!GetWorld() || !bDrawDebugSpheres)
    {
        return;
    }

    for (AResourceDeposit* Deposit : SpawnedDeposits)
    {
        if (IsValid(Deposit))
        {
            FVector Location = Deposit->GetActorLocation();
            FColor DepositColor = FColor::Blue;
            
            // Color by deposit type (simple hashing for consistent colors)
            FString DepositName = Deposit->GetDepositName().ToString();
            if (DepositName.Contains("Iron"))
            {
                DepositColor = FColor::Red;
            }
            else if (DepositName.Contains("Oil"))
            {
                DepositColor = FColor::Black;
            }
            else if (DepositName.Contains("Wheat"))
            {
                DepositColor = FColor::Yellow;
            }
            else if (DepositName.Contains("Coal"))
            {
                DepositColor = FColor::White;
            }
            else if (DepositName.Contains("Stone"))
            {
                DepositColor = FColor::Cyan;
            }
            else if (DepositName.Contains("Wood"))
            {
                DepositColor = FColor::Green;
            }
            
            // Draw sphere at deposit location
            DrawDebugSphere(GetWorld(), Location, 150.0f, 8, DepositColor, false, DebugDisplayTime, 0, 5.0f);
            
            // Draw deposit name
            DrawDebugString(GetWorld(), Location + FVector(0, 0, 200), 
                          DepositName, nullptr, DepositColor, DebugDisplayTime);
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
           bUseDefaultSpawnRules ? TEXT("Yes") : TEXT("No"));
    UE_LOG(LogTemp, Log, TEXT("Custom Rules Count: %d"), CustomSpawnRules.Num());
    UE_LOG(LogTemp, Log, TEXT("Auto Connect to Hubs: %s (disabled - TransportHub not implemented)"), 
           bAutoConnectToNearestHub ? TEXT("Yes") : TEXT("No"));
    UE_LOG(LogTemp, Log, TEXT("Hub Search Radius: %.0f"), HubSearchRadius);
    UE_LOG(LogTemp, Log, TEXT("Max Spawn Attempts: %d"), MaxSpawnAttempts);
    UE_LOG(LogTemp, Log, TEXT("Grid Resolution: %d"), GridResolution);
    
    if (bUseCustomBounds)
    {
        UE_LOG(LogTemp, Log, TEXT("Custom Spawn Area: Center=%s, Size=%s"), 
               *CustomSpawnCenter.ToString(), *CustomSpawnSize.ToString());
    }
    else if (SpawnAreaBounds)
    {
        FVector Center = GetActorLocation() + SpawnAreaBounds->GetRelativeLocation();
        FVector Size = SpawnAreaBounds->GetScaledBoxExtent() * 2.0f;
        UE_LOG(LogTemp, Log, TEXT("Bounds Spawn Area: Center=%s, Size=%s"), 
               *Center.ToString(), *Size.ToString());
    }
    
    UE_LOG(LogTemp, Log, TEXT("============================================"));
}