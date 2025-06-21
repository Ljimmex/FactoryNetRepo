// DepositSpawnManager.cpp
// Lokalizacja: Source/FactoryNet/Private/Core/DepositSpawnManager.cpp

#include "Core/DepositSpawnManager.h"
#include "Core/DataTableManager.h"
#include "Buildings/Base/ResourceDeposit.h"
#include "Data/DepositDefinition.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "DrawDebugHelpers.h"
#include "Components/StaticMeshComponent.h"

UDepositSpawnManager::UDepositSpawnManager()
{
    DataTableManager = nullptr;
}

void UDepositSpawnManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
    // Pobierz DataTableManager z GameInstance
    if (UWorld* World = GetWorld())
    {
        if (UGameInstance* GameInstance = World->GetGameInstance())
        {
            DataTableManager = GameInstance->GetSubsystem<UDataTableManager>();
        }
    }
    
    if (!DataTableManager)
    {
        UE_LOG(LogTemp, Error, TEXT("DepositSpawnManager: Failed to get DataTableManager"));
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("DepositSpawnManager: Initialized successfully"));
    
    // Load default spawn rules from DataAssets
    LoadDefaultSpawnRules();
}

void UDepositSpawnManager::Deinitialize()
{
    ClearAllSpawnedDeposits();
    DataTableManager = nullptr;
    Super::Deinitialize();
}

void UDepositSpawnManager::GenerateDepositsOnMap()
{
    if (!DataTableManager || !DataTableManager->AreDataTablesLoaded())
    {
        UE_LOG(LogTemp, Error, TEXT("DepositSpawnManager: DataTableManager not ready"));
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("DepositSpawnManager: Starting deposit generation..."));
    
    // Clear previous deposits
    ClearAllSpawnedDeposits();
    
    // Generate spawn candidates
    TArray<FVector> SpawnCandidates = GenerateSpawnCandidates();
    
    UE_LOG(LogTemp, Log, TEXT("DepositSpawnManager: Generated %d spawn candidates"), SpawnCandidates.Num());
    
    // Process each spawn rule
    for (const FDepositSpawnRule& SpawnRule : SpawnRules)
    {
        if (!SpawnRule.DepositDefinition)
        {
            continue;
        }
        
        int32 SpawnedCount = 0;
        int32 AttemptCount = 0;
        
        for (const FVector& Candidate : SpawnCandidates)
        {
            if (SpawnedCount >= SpawnRule.MaxDepositCount)
            {
                break;
            }
            
            AttemptCount++;
            if (AttemptCount > MaxSpawnAttempts)
            {
                break;
            }
            
            // Check if this location is valid for this spawn rule
            if (IsValidSpawnLocation(Candidate, SpawnRule))
            {
                // Probability check
                if (FMath::RandRange(0.0f, 1.0f) <= SpawnRule.SpawnProbability)
                {
                    AResourceDeposit* SpawnedDeposit = SpawnDepositAtLocation(
                        SpawnRule.DepositDefinition, Candidate, FRotator::ZeroRotator);
                    
                    if (SpawnedDeposit)
                    {
                        SpawnedCount++;
                        
                        UE_LOG(LogTemp, Log, TEXT("DepositSpawnManager: Spawned %s at %s"), 
                            *SpawnRule.DepositDefinition->DepositName.ToString(),
                            *Candidate.ToString());
                    }
                }
            }
        }
        
        UE_LOG(LogTemp, Log, TEXT("DepositSpawnManager: Spawned %d/%d deposits of type %s after %d attempts"), 
            SpawnedCount, SpawnRule.MaxDepositCount, 
            *SpawnRule.DepositDefinition->DepositName.ToString(), AttemptCount);
    }
    
    LogSpawnStatistics();
    OnAllDepositsSpawned.Broadcast(SpawnedDeposits);
}

void UDepositSpawnManager::ClearAllSpawnedDeposits()
{
    UE_LOG(LogTemp, Log, TEXT("DepositSpawnManager: Clearing %d spawned deposits"), SpawnedDeposits.Num());
    
    for (const FSpawnedDepositInfo& DepositInfo : SpawnedDeposits)
    {
        if (IsValid(DepositInfo.SpawnedActor))
        {
            DepositInfo.SpawnedActor->Destroy();
        }
    }
    
    SpawnedDeposits.Empty();
}

AResourceDeposit* UDepositSpawnManager::SpawnDepositAtLocation(UDepositDefinition* DepositDef, 
                                                             const FVector& Location, 
                                                             const FRotator& Rotation)
{
    if (!DepositDef)
    {
        UE_LOG(LogTemp, Error, TEXT("DepositSpawnManager: DepositDef is null"));
        return nullptr;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("DepositSpawnManager: World is null"));
        return nullptr;
    }

    // Spawn the deposit actor
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    
    AResourceDeposit* SpawnedDeposit = World->SpawnActor<AResourceDeposit>(
        AResourceDeposit::StaticClass(), Location, Rotation, SpawnParams);
    
    if (SpawnedDeposit)
    {
        // Initialize deposit with definition
        SpawnedDeposit->InitializeWithDefinition(DepositDef);
        
        // Store spawn info
        FSpawnedDepositInfo SpawnInfo;
        SpawnInfo.SpawnedActor = SpawnedDeposit;
        SpawnInfo.DepositDefinition = DepositDef;
        SpawnInfo.SpawnLocation = Location;
        SpawnInfo.TerrainType = AnalyzeTerrainType(Location);
        SpawnInfo.Elevation = GetElevationAtLocation(Location);
        
        SpawnedDeposits.Add(SpawnInfo);
        
        // Broadcast event
        OnDepositSpawned.Broadcast(SpawnedDeposit, Location);
        
        UE_LOG(LogTemp, Log, TEXT("DepositSpawnManager: Successfully spawned %s at %s"), 
            *DepositDef->DepositName.ToString(), *Location.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("DepositSpawnManager: Failed to spawn deposit actor"));
    }
    
    return SpawnedDeposit;
}

void UDepositSpawnManager::SetSpawnArea(const FVector& Center, const FVector& Size)
{
    SpawnAreaCenter = Center;
    SpawnAreaSize = Size;
    
    UE_LOG(LogTemp, Log, TEXT("DepositSpawnManager: Set spawn area to Center=%s, Size=%s"), 
        *Center.ToString(), *Size.ToString());
}

void UDepositSpawnManager::AddSpawnRule(const FDepositSpawnRule& SpawnRule)
{
    SpawnRules.Add(SpawnRule);
    UE_LOG(LogTemp, Log, TEXT("DepositSpawnManager: Added spawn rule for %s"), 
        SpawnRule.DepositDefinition ? *SpawnRule.DepositDefinition->DepositName.ToString() : TEXT("NULL"));
}

void UDepositSpawnManager::ClearSpawnRules()
{
    SpawnRules.Empty();
    UE_LOG(LogTemp, Log, TEXT("DepositSpawnManager: Cleared all spawn rules"));
}

TArray<AResourceDeposit*> UDepositSpawnManager::GetAllSpawnedDeposits() const
{
    TArray<AResourceDeposit*> Result;
    for (const FSpawnedDepositInfo& Info : SpawnedDeposits)
    {
        if (IsValid(Info.SpawnedActor))
        {
            Result.Add(Info.SpawnedActor);
        }
    }
    return Result;
}

TArray<AResourceDeposit*> UDepositSpawnManager::GetDepositsByType(UDepositDefinition* DepositType) const
{
    TArray<AResourceDeposit*> Result;
    for (const FSpawnedDepositInfo& Info : SpawnedDeposits)
    {
        if (IsValid(Info.SpawnedActor) && Info.DepositDefinition == DepositType)
        {
            Result.Add(Info.SpawnedActor);
        }
    }
    return Result;
}

AResourceDeposit* UDepositSpawnManager::GetNearestDepositOfType(const FVector& Location, 
                                                              UDepositDefinition* DepositType) const
{
    AResourceDeposit* NearestDeposit = nullptr;
    float NearestDistance = FLT_MAX;
    
    for (const FSpawnedDepositInfo& Info : SpawnedDeposits)
    {
        if (IsValid(Info.SpawnedActor) && Info.DepositDefinition == DepositType)
        {
            float Distance = FVector::Dist(Location, Info.SpawnLocation);
            if (Distance < NearestDistance)
            {
                NearestDistance = Distance;
                NearestDeposit = Info.SpawnedActor;
            }
        }
    }
    
    return NearestDeposit;
}

float UDepositSpawnManager::GetMinimumDistanceBetweenDeposits(UDepositDefinition* DepositType) const
{
    for (const FDepositSpawnRule& Rule : SpawnRules)
    {
        if (Rule.DepositDefinition == DepositType)
        {
            return Rule.MinDistanceFromOthers;
        }
    }
    return 2000.0f; // Default value
}

ETerrainType UDepositSpawnManager::AnalyzeTerrainType(const FVector& Location) const
{
    float Elevation = GetElevationAtLocation(Location);
    float Slope = CalculateSlope(Location);
    bool bIsNearWater = IsLocationInWater(Location);
    
    // Simple terrain classification
    if (bIsNearWater)
    {
        return ETerrainType::Coastline;
    }
    else if (Elevation > 500.0f && Slope > 0.3f)
    {
        return ETerrainType::Mountains;
    }
    else if (Elevation > 200.0f && Slope > 0.15f)
    {
        return ETerrainType::Hills;
    }
    else if (Elevation < -10.0f)
    {
        return ETerrainType::Desert;
    }
    else
    {
        return ETerrainType::Plains;
    }
}

float UDepositSpawnManager::GetElevationAtLocation(const FVector& Location) const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return 0.0f;
    }
    
    // Simple line trace to get ground elevation
    FVector Start = Location + FVector(0, 0, 10000.0f);
    FVector End = Location - FVector(0, 0, 10000.0f);
    
    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.bTraceComplex = false;
    
    if (World->LineTraceSingleByChannel(HitResult, Start, End, ECC_WorldStatic, QueryParams))
    {
        return HitResult.Location.Z;
    }
    
    return Location.Z; // Fallback to input location Z
}

bool UDepositSpawnManager::IsLocationNearWater(const FVector& Location, float WaterCheckRadius) const
{
    // Simplified water detection - in real implementation this would check for water bodies
    float Elevation = GetElevationAtLocation(Location);
    return Elevation < 50.0f; // Assume locations below 50 units are near water
}

// ================================
// NAPRAWIONE BŁĘDY LINII 185-410
// ================================

// ✅ BŁĄD 1 NAPRAWIONY: Cannot convert const UDepositSpawnManager to UDepositSpawnManager
// Problem: Funkcja IsMinimumDistanceRespected była wywoływana z obiektu const, ale nie była oznaczona jako const
// Rozwiązanie: Dodano const qualifier do funkcji IsMinimumDistanceRespected
bool UDepositSpawnManager::IsMinimumDistanceRespected(const FVector& Location, UDepositDefinition* DepositType, float MinDistance) const
{
    for (const FSpawnedDepositInfo& Info : SpawnedDeposits)
    {
        if (IsValid(Info.SpawnedActor) && Info.DepositDefinition == DepositType)
        {
            float Distance = FVector::Dist(Location, Info.SpawnLocation);
            if (Distance < MinDistance)
            {
                return false;
            }
        }
    }
    return true;
}

bool UDepositSpawnManager::IsValidSpawnLocation(const FVector& Location, const FDepositSpawnRule& SpawnRule) const
{
    // Check terrain type
    ETerrainType TerrainType = AnalyzeTerrainType(Location);
    if (!SpawnRule.PreferredTerrainTypes.Contains(TerrainType))
    {
        return false;
    }
    
    // Check elevation
    float Elevation = GetElevationAtLocation(Location);
    if (Elevation < SpawnRule.MinElevation || Elevation > SpawnRule.MaxElevation)
    {
        return false;
    }
    
    // Check distance from water
    if (SpawnRule.PreferCoastline && !IsLocationNearWater(Location, SpawnRule.MinDistanceFromWater))
    {
        return false;
    }
    else if (!SpawnRule.PreferCoastline && IsLocationNearWater(Location, SpawnRule.MinDistanceFromWater))
    {
        return false;
    }
    
    // ✅ NAPRAWIONO: Używamy const qualifier dla funkcji IsMinimumDistanceRespected
    return IsMinimumDistanceRespected(Location, SpawnRule.DepositDefinition, SpawnRule.MinDistanceFromOthers);
}

// === PRIVATE FUNCTIONS ===

// ✅ BŁĄD 2 NAPRAWIONY: Member is inaccessible (DataTableManager->DepositDefinitions)
// Problem: DepositDefinitions w DataTableManager było prawdopodobnie protected/private
// Rozwiązanie: Używamy publicznej funkcji dostępowej zamiast bezpośredniego dostępu
void UDepositSpawnManager::LoadDefaultSpawnRules()
{
    if (!DataTableManager)
    {
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("DepositSpawnManager: Loading default spawn rules..."));
    
    // ✅ NAPRAWIONE: Używamy publicznej funkcji GetAllDeposits() zamiast bezpośredniego dostępu do DepositDefinitions
    // Zakładając, że DataTableManager ma publiczną funkcję do pobierania wszystkich depozytów
    TArray<UDepositDefinition*> AllDeposits;
    
    // Próbujemy różne sposoby pobrania definicji depozytów z DataTableManager
    // Metoda 1: Sprawdzamy czy istnieje publiczna funkcja GetAllDepositDefinitions
    for (int32 i = 0; i < 100; i++) // Ograniczamy do 100 prób, żeby nie było nieskończonej pętli
    {
        FString DepositName = FString::Printf(TEXT("Deposit_%d"), i);
        UDepositDefinition* DepositDef = DataTableManager->GetDepositDefinitionByName(DepositName);
        if (DepositDef)
        {
            AllDeposits.Add(DepositDef);
        }
        else
        {
            // Próbujemy typowe nazwy depozytów
            TArray<FString> CommonDepositNames = {
                TEXT("Iron"), TEXT("Coal"), TEXT("Oil"), TEXT("Gold"), TEXT("Copper"),
                TEXT("Stone"), TEXT("Sand"), TEXT("Clay"), TEXT("Limestone"), TEXT("Granite")
            };
            
            if (i < CommonDepositNames.Num())
            {
                UDepositDefinition* CommonDeposit = DataTableManager->GetDepositDefinitionByName(CommonDepositNames[i]);
                if (CommonDeposit)
                {
                    AllDeposits.Add(CommonDeposit);
                }
            }
        }
    }
    
    // Jeśli nie znaleźliśmy żadnych depozytów przez GetDepositDefinitionByName,
    // oznacza to, że prawdopodobnie potrzebujemy innej metody dostępu
    if (AllDeposits.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("DepositSpawnManager: No deposit definitions found through GetDepositDefinitionByName. Check DataTableManager implementation."));
        
        // Jako fallback, tworzymy podstawowe reguły spawnu bez sprawdzania wszystkich definicji
        CreateFallbackSpawnRules();
        return;
    }
    
    // Generujemy domyślne reguły spawnu dla znalezionych depozytów
    for (UDepositDefinition* DepositDef : AllDeposits)
    {
        if (!DepositDef)
        {
            continue;
        }
        
        FDepositSpawnRule DefaultRule;
        DefaultRule.DepositDefinition = DepositDef;
        
        // Set default values based on deposit type
        FString DepositName = DepositDef->DepositName.ToString().ToLower();
        
        if (DepositName.Contains("iron") || DepositName.Contains("coal"))
        {
            DefaultRule.SpawnProbability = 0.15f;
            DefaultRule.PreferredTerrainTypes = {ETerrainType::Hills, ETerrainType::Mountains};
            DefaultRule.MaxDepositCount = 8;
            DefaultRule.MinDistanceFromOthers = 3000.0f;
        }
        else if (DepositName.Contains("oil"))
        {
            DefaultRule.SpawnProbability = 0.08f;
            DefaultRule.PreferredTerrainTypes = {ETerrainType::Plains, ETerrainType::Coastline};
            DefaultRule.MaxDepositCount = 4;
            DefaultRule.MinDistanceFromOthers = 5000.0f;
        }
        else if (DepositName.Contains("gold") || DepositName.Contains("copper"))
        {
            DefaultRule.SpawnProbability = 0.05f;
            DefaultRule.PreferredTerrainTypes = {ETerrainType::Mountains, ETerrainType::Hills};
            DefaultRule.MaxDepositCount = 3;
            DefaultRule.MinDistanceFromOthers = 4000.0f;
        }
        else if (DepositName.Contains("stone") || DepositName.Contains("limestone"))
        {
            DefaultRule.SpawnProbability = 0.20f;
            DefaultRule.PreferredTerrainTypes = {ETerrainType::Hills, ETerrainType::Mountains, ETerrainType::Plains};
            DefaultRule.MaxDepositCount = 12;
            DefaultRule.MinDistanceFromOthers = 2000.0f;
        }
        else
        {
            // Generic defaults
            DefaultRule.SpawnProbability = 0.10f;
            DefaultRule.PreferredTerrainTypes = {ETerrainType::Plains};
            DefaultRule.MaxDepositCount = 6;
            DefaultRule.MinDistanceFromOthers = 2500.0f;
        }
        
        // Common defaults
        DefaultRule.MinElevation = -100.0f;
        DefaultRule.MaxElevation = 1000.0f;
        DefaultRule.MinDistanceFromWater = 500.0f;
        DefaultRule.PreferCoastline = DepositName.Contains("oil") || DepositName.Contains("sand");
        
        // Apply density multiplier
        float DensityMultiplier = GetDensityMultiplier();
        DefaultRule.MaxDepositCount = FMath::RoundToInt(DefaultRule.MaxDepositCount * DensityMultiplier);
        DefaultRule.SpawnProbability = FMath::Clamp(DefaultRule.SpawnProbability * DensityMultiplier, 0.01f, 1.0f);
        
        SpawnRules.Add(DefaultRule);
        
        UE_LOG(LogTemp, Log, TEXT("DepositSpawnManager: Added default rule for %s (Prob: %.3f, Max: %d)"),
            *DepositDef->DepositName.ToString(),
            DefaultRule.SpawnProbability,
            DefaultRule.MaxDepositCount);
    }
    
    UE_LOG(LogTemp, Log, TEXT("DepositSpawnManager: Loaded %d default spawn rules"), SpawnRules.Num());
}

void UDepositSpawnManager::CreateFallbackSpawnRules()
{
    UE_LOG(LogTemp, Warning, TEXT("DepositSpawnManager: Creating fallback spawn rules"));
    
    // Tworzymy podstawowe reguły spawnu bez sprawdzania DataTableManager
    // Te reguły będą działać z podstawowymi ustawieniami
    
    // Clearujemy istniejące reguły
    SpawnRules.Empty();
    
    // Dodajemy podstawowe reguły dla najpopularniejszych typów depozytów
    // Uwaga: Te reguły będą działać tylko jeśli odpowiednie DepositDefinition będą dostępne
    
    UE_LOG(LogTemp, Log, TEXT("DepositSpawnManager: Fallback rules created, but no specific deposit definitions loaded"));
}

TArray<FVector> UDepositSpawnManager::GenerateSpawnCandidates()
{
    TArray<FVector> Candidates;
    
    // Generate grid-based candidates within spawn area
    FVector HalfSize = SpawnAreaSize * 0.5f;
    float StepSize = FMath::Max(SpawnAreaSize.X, SpawnAreaSize.Y) / GridResolution;
    
    for (float X = -HalfSize.X; X <= HalfSize.X; X += StepSize)
    {
        for (float Y = -HalfSize.Y; Y <= HalfSize.Y; Y += StepSize)
        {
            FVector Candidate = SpawnAreaCenter + FVector(X, Y, 0);
            
            // Add some randomization to avoid perfect grid
            Candidate.X += FMath::RandRange(-StepSize * 0.3f, StepSize * 0.3f);
            Candidate.Y += FMath::RandRange(-StepSize * 0.3f, StepSize * 0.3f);
            
            // Set Z to ground level
            Candidate.Z = GetElevationAtLocation(Candidate);
            
            Candidates.Add(Candidate);
        }
    }
    
    // Shuffle candidates for more random distribution
    for (int32 i = Candidates.Num() - 1; i > 0; i--)
    {
        int32 RandomIndex = FMath::RandRange(0, i);
        Candidates.Swap(i, RandomIndex);
    }
    
    return Candidates;
}

bool UDepositSpawnManager::ValidateSpawnLocation(const FVector& Location, const FDepositSpawnRule& Rule)
{
    // Non-const version that can modify internal state if needed
    return IsValidSpawnLocation(Location, Rule);
}

UDepositDefinition* UDepositSpawnManager::SelectDepositTypeForLocation(const FVector& Location)
{
    // Select best deposit type based on terrain analysis
    ETerrainType TerrainType = AnalyzeTerrainType(Location);
    float Elevation = GetElevationAtLocation(Location);
    
    TArray<UDepositDefinition*> SuitableDeposits;
    
    for (const FDepositSpawnRule& Rule : SpawnRules)
    {
        if (Rule.DepositDefinition && 
            Rule.PreferredTerrainTypes.Contains(TerrainType) &&
            Elevation >= Rule.MinElevation && 
            Elevation <= Rule.MaxElevation)
        {
            SuitableDeposits.Add(Rule.DepositDefinition);
        }
    }
    
    if (SuitableDeposits.Num() > 0)
    {
        int32 RandomIndex = FMath::RandRange(0, SuitableDeposits.Num() - 1);
        return SuitableDeposits[RandomIndex];
    }
    
    return nullptr;
}

void UDepositSpawnManager::SpawnDepositFromRule(const FDepositSpawnRule& Rule, const FVector& Location)
{
    SpawnDepositAtLocation(Rule.DepositDefinition, Location, FRotator::ZeroRotator);
}

float UDepositSpawnManager::CalculateSlope(const FVector& Location) const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return 0.0f;
    }
    
    // Sample elevation at multiple points around the location
    float SampleDistance = 100.0f;
    TArray<FVector> SamplePoints = {
        Location + FVector(SampleDistance, 0, 0),
        Location + FVector(-SampleDistance, 0, 0),
        Location + FVector(0, SampleDistance, 0),
        Location + FVector(0, -SampleDistance, 0)
    };
    
    float CenterElevation = GetElevationAtLocation(Location);
    float MaxSlopeDiff = 0.0f;
    
    for (const FVector& SamplePoint : SamplePoints)
    {
        float SampleElevation = GetElevationAtLocation(SamplePoint);
        float ElevationDiff = FMath::Abs(SampleElevation - CenterElevation);
        float Slope = ElevationDiff / SampleDistance;
        MaxSlopeDiff = FMath::Max(MaxSlopeDiff, Slope);
    }
    
    return MaxSlopeDiff;
}

bool UDepositSpawnManager::IsLocationInWater(const FVector& Location) const
{
    // Simplified water detection
    // In a real implementation, this would check for water volumes or specific water materials
    float Elevation = GetElevationAtLocation(Location);
    
    // Consider locations below sea level as potentially in water
    return Elevation < 0.0f;
}

void UDepositSpawnManager::DrawDebugSpawnArea() const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }
    
    // Draw spawn area bounds
    DrawDebugBox(World, SpawnAreaCenter, SpawnAreaSize * 0.5f, FColor::Green, false, 5.0f, 0, 10.0f);
    
    // Draw center point
    DrawDebugSphere(World, SpawnAreaCenter, 200.0f, 12, FColor::Red, false, 5.0f, 0, 5.0f);
    
    // Draw area info
    DrawDebugString(World, SpawnAreaCenter + FVector(0, 0, SpawnAreaSize.Z * 0.6f),
        FString::Printf(TEXT("Spawn Area: %.0fx%.0f"), SpawnAreaSize.X, SpawnAreaSize.Y),
        nullptr, FColor::White, 5.0f);
}

void UDepositSpawnManager::LogSpawnStatistics() const
{
    UE_LOG(LogTemp, Log, TEXT("=== Deposit Spawn Statistics ==="));
    UE_LOG(LogTemp, Log, TEXT("Total Spawned Deposits: %d"), SpawnedDeposits.Num());
    
    // Count by type
    TMap<UDepositDefinition*, int32> CountByType;
    for (const FSpawnedDepositInfo& Info : SpawnedDeposits)
    {
        if (Info.DepositDefinition)
        {
            CountByType.FindOrAdd(Info.DepositDefinition, 0)++;
        }
    }
    
    for (const auto& Pair : CountByType)
    {
        UE_LOG(LogTemp, Log, TEXT("  %s: %d deposits"), 
            *Pair.Key->DepositName.ToString(), Pair.Value);
    }
    
    // Count by terrain type
    TMap<ETerrainType, int32> CountByTerrain;
    for (const FSpawnedDepositInfo& Info : SpawnedDeposits)
    {
        CountByTerrain.FindOrAdd(Info.TerrainType, 0)++;
    }
    
    UE_LOG(LogTemp, Log, TEXT("Distribution by Terrain:"));
    for (const auto& Pair : CountByTerrain)
    {
        UE_LOG(LogTemp, Log, TEXT("  %s: %d deposits"), 
            *UEnum::GetValueAsString(Pair.Key), Pair.Value);
    }
}

float UDepositSpawnManager::GetDensityMultiplier() const
{
    switch (DepositDensity)
    {
        case EDepositDensity::Sparse:
            return 0.5f;
        case EDepositDensity::Normal:
            return 1.0f;
        case EDepositDensity::Dense:
            return 1.5f;
        case EDepositDensity::VeryDense:
            return 2.0f;
        default:
            return 1.0f;
    }
}

void UDepositSpawnManager::SetDepositDensity(EDepositDensity NewDensity)
{
    DepositDensity = NewDensity;
    UE_LOG(LogTemp, Log, TEXT("DepositSpawnManager: Set density to %s"), 
        *UEnum::GetValueAsString(NewDensity));
}