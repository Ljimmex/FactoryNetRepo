// DepositSpawnManager.cpp
// Lokalizacja: Source/FactoryNet/Private/Core/DepositSpawnManager.cpp

#include "Core/DepositSpawnManager.h"
#include "Core/DataTableManager.h"
#include "Buildings/Base/ResourceDeposit.h"
#include "Data/DepositDefinition.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
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
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        DataTableManager = GameInstance->GetSubsystem<UDataTableManager>();
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
    
    // Clear existing deposits
    ClearAllSpawnedDeposits();
    
    // Generate spawn candidates based on grid
    TArray<FVector> SpawnCandidates = GenerateSpawnCandidates();
    
    UE_LOG(LogTemp, Log, TEXT("DepositSpawnManager: Generated %d spawn candidates"), SpawnCandidates.Num());
    
    // For each spawn rule, try to spawn deposits
    for (const FDepositSpawnRule& SpawnRule : SpawnRules)
    {
        if (!SpawnRule.DepositDefinition)
        {
            continue;
        }
        
        int32 SpawnedCount = 0;
        int32 AttemptCount = 0;
        
        UE_LOG(LogTemp, Log, TEXT("DepositSpawnManager: Processing spawn rule for %s"), 
            *SpawnRule.DepositDefinition->DepositName.ToString());
        
        // Shuffle candidates for randomness
        TArray<FVector> ShuffledCandidates = SpawnCandidates;
        for (int32 i = ShuffledCandidates.Num() - 1; i > 0; i--)
        {
            int32 j = FMath::RandRange(0, i);
            ShuffledCandidates.Swap(i, j);
        }
        
        for (const FVector& Candidate : ShuffledCandidates)
        {
            if (SpawnedCount >= SpawnRule.MaxDepositCount || AttemptCount >= MaxSpawnAttempts)
            {
                break;
            }
            
            AttemptCount++;
            
            // Check spawn probability
            if (FMath::RandRange(0.0f, 1.0f) > SpawnRule.SpawnProbability)
            {
                continue;
            }
            
            // Validate spawn location
            if (IsValidSpawnLocation(Candidate, SpawnRule))
            {
                SpawnDepositFromRule(SpawnRule, Candidate);
                SpawnedCount++;
                
                UE_LOG(LogTemp, Log, TEXT("DepositSpawnManager: Spawned %s at %s"), 
                    *SpawnRule.DepositDefinition->DepositName.ToString(),
                    *Candidate.ToString());
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
    if (!DepositDef || !GetWorld())
    {
        return nullptr;
    }
    
    // Spawn the deposit actor
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    
    AResourceDeposit* SpawnedDeposit = GetWorld()->SpawnActor<AResourceDeposit>(
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
        SpawnRule.DepositDefinition ? 
        *SpawnRule.DepositDefinition->DepositName.ToString() : TEXT("NULL"));
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
    bool NearWater = IsLocationNearWater(Location, 2000.0f);
    
    // Terrain classification logic
    if (NearWater && Elevation < 100.0f)
    {
        return ETerrainType::Coastline;
    }
    else if (Elevation > 1000.0f && Slope > 0.3f)
    {
        return ETerrainType::Mountains;
    }
    else if (Elevation > 500.0f && Slope > 0.15f)
    {
        return ETerrainType::Hills;
    }
    else if (Slope < 0.05f && Elevation < 200.0f)
    {
        return ETerrainType::Plains;
    }
    else
    {
        return ETerrainType::Forest; // Default for mixed terrain
    }
}

float UDepositSpawnManager::GetElevationAtLocation(const FVector& Location) const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return 0.0f;
    }
    
    // Line trace to get ground height
    FHitResult HitResult;
    FVector StartLocation = Location + FVector(0, 0, 10000.0f);
    FVector EndLocation = Location - FVector(0, 0, 10000.0f);
    
    FCollisionQueryParams QueryParams;
    QueryParams.bTraceComplex = false;
    
    if (World->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, 
                                       ECC_WorldStatic, QueryParams))
    {
        return HitResult.Location.Z;
    }
    
    return Location.Z;
}

bool UDepositSpawnManager::IsLocationNearWater(const FVector& Location, float MaxDistance) const
{
    // Simplified water detection - check for low elevation areas
    float Elevation = GetElevationAtLocation(Location);
    
    // Check surrounding area for water bodies
    int32 WaterCount = 0;
    int32 SampleCount = 8;
    
    for (int32 i = 0; i < SampleCount; i++)
    {
        float Angle = (2.0f * PI * i) / SampleCount;
        FVector SampleLocation = Location + FVector(
            FMath::Cos(Angle) * MaxDistance,
            FMath::Sin(Angle) * MaxDistance,
            0.0f
        );
        
        float SampleElevation = GetElevationAtLocation(SampleLocation);
        if (SampleElevation < Elevation - 50.0f) // Water is significantly lower
        {
            WaterCount++;
        }
    }
    
    return WaterCount >= 2; // At least 25% of samples indicate water
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
    
    // Check minimum distance from other deposits
    return IsMinimumDistanceRespected(Location, SpawnRule.DepositDefinition, SpawnRule.MinDistanceFromOthers);
}

// === PRIVATE FUNCTIONS ===

void UDepositSpawnManager::LoadDefaultSpawnRules()
{
    if (!DataTableManager)
    {
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("DepositSpawnManager: Loading default spawn rules..."));
    
    // Get all deposit definitions from DataTableManager
    const TArray<UDepositDefinition*>& AllDeposits = DataTableManager->DepositDefinitions;
    
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
            DefaultRule.PreferredTerrainTypes = {ETerrainType::Coastline, ETerrainType::Plains};
            DefaultRule.MaxDepositCount = 4;
            DefaultRule.MinDistanceFromOthers = 5000.0f;
            DefaultRule.PreferCoastline = true;
        }
        else if (DepositName.Contains("wheat") || DepositName.Contains("farm"))
        {
            DefaultRule.SpawnProbability = 0.2f;
            DefaultRule.PreferredTerrainTypes = {ETerrainType::Plains};
            DefaultRule.MaxDepositCount = 12;
            DefaultRule.MinDistanceFromOthers = 2000.0f;
            DefaultRule.MaxElevation = 300.0f;
        }
        else if (DepositName.Contains("wood") || DepositName.Contains("forest"))
        {
            DefaultRule.SpawnProbability = 0.25f;
            DefaultRule.PreferredTerrainTypes = {ETerrainType::Forest, ETerrainType::Hills};
            DefaultRule.MaxDepositCount = 15;
            DefaultRule.MinDistanceFromOthers = 1500.0f;
        }
        else if (DepositName.Contains("stone") || DepositName.Contains("quarry"))
        {
            DefaultRule.SpawnProbability = 0.12f;
            DefaultRule.PreferredTerrainTypes = {ETerrainType::Mountains, ETerrainType::Hills};
            DefaultRule.MaxDepositCount = 6;
            DefaultRule.MinDistanceFromOthers = 2500.0f;
            DefaultRule.MinElevation = 200.0f;
        }
        
        AddSpawnRule(DefaultRule);
    }
    
    UE_LOG(LogTemp, Log, TEXT("DepositSpawnManager: Loaded %d default spawn rules"), SpawnRules.Num());
}

TArray<FVector> UDepositSpawnManager::GenerateSpawnCandidates()
{
    TArray<FVector> Candidates;
    
    int32 GridSteps = FMath::Max(10, GridResolution);
    float StepSizeX = SpawnAreaSize.X / GridSteps;
    float StepSizeY = SpawnAreaSize.Y / GridSteps;
    
    FVector MinBounds = SpawnAreaCenter - SpawnAreaSize * 0.5f;
    
    for (int32 X = 0; X < GridSteps; X++)
    {
        for (int32 Y = 0; Y < GridSteps; Y++)
        {
            FVector GridLocation = MinBounds + FVector(
                X * StepSizeX + FMath::RandRange(-StepSizeX * 0.4f, StepSizeX * 0.4f),
                Y * StepSizeY + FMath::RandRange(-StepSizeY * 0.4f, StepSizeY * 0.4f),
                0.0f
            );
            
            // Adjust Z to ground level
            GridLocation.Z = GetElevationAtLocation(GridLocation);
            
            Candidates.Add(GridLocation);
        }
    }
    
    return Candidates;
}

bool UDepositSpawnManager::ValidateSpawnLocation(const FVector& Location, const FDepositSpawnRule& Rule)
{
    return IsValidSpawnLocation(Location, Rule);
}

UDepositDefinition* UDepositSpawnManager::SelectDepositTypeForLocation(const FVector& Location)
{
    ETerrainType TerrainType = AnalyzeTerrainType(Location);
    float Elevation = GetElevationAtLocation(Location);
    
    // Find suitable deposit types for this location
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
    SpawnDepositAtLocation(Rule.DepositDefinition, Location);
}

bool UDepositSpawnManager::IsMinimumDistanceRespected(const FVector& Location, 
                                                    UDepositDefinition* DepositType, 
                                                    float MinDistance)
{
    for (const FSpawnedDepositInfo& Info : SpawnedDeposits)
    {
        if (Info.DepositDefinition == DepositType)
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

float UDepositSpawnManager::CalculateSlope(const FVector& Location) const
{
    // Sample elevation at 4 points around the location
    float SampleRadius = 100.0f;
    
    FVector North = Location + FVector(0, SampleRadius, 0);
    FVector South = Location + FVector(0, -SampleRadius, 0);
    FVector East = Location + FVector(SampleRadius, 0, 0);
    FVector West = Location + FVector(-SampleRadius, 0, 0);
    
    float ElevationN = GetElevationAtLocation(North);
    float ElevationS = GetElevationAtLocation(South);
    float ElevationE = GetElevationAtLocation(East);
    float ElevationW = GetElevationAtLocation(West);
    
    // Calculate gradients
    float GradientNS = FMath::Abs(ElevationN - ElevationS) / (2.0f * SampleRadius);
    float GradientEW = FMath::Abs(ElevationE - ElevationW) / (2.0f * SampleRadius);
    
    // Return maximum gradient as slope
    return FMath::Max(GradientNS, GradientEW);
}

bool UDepositSpawnManager::IsLocationInWater(const FVector& Location) const
{
    // Simple water detection - check if location is below sea level
    float Elevation = GetElevationAtLocation(Location);
    return Elevation < 0.0f; // Assuming sea level is at Z=0
}

void UDepositSpawnManager::DrawDebugSpawnArea() const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }
    
    // Draw spawn area bounds
    DrawDebugBox(World, SpawnAreaCenter, SpawnAreaSize * 0.5f, 
                FColor::Yellow, false, 60.0f, 0, 50.0f);
    
    // Draw spawned deposits
    for (const FSpawnedDepositInfo& Info : SpawnedDeposits)
    {
        if (IsValid(Info.SpawnedActor))
        {
            FColor DepositColor = FColor::Green;
            
            // Color code by terrain type
            switch (Info.TerrainType)
            {
                case ETerrainType::Plains:
                    DepositColor = FColor::Green;
                    break;
                case ETerrainType::Hills:
                    DepositColor = FColor::Orange;
                    break;
                case ETerrainType::Mountains:
                    DepositColor = FColor::Red;
                    break;
                case ETerrainType::Coastline:
                    DepositColor = FColor::Blue;
                    break;
                case ETerrainType::Forest:
                    DepositColor = FColor::Emerald;
                    break;
                default:
                    DepositColor = FColor::White;
                    break;
            }
            
            DrawDebugSphere(World, Info.SpawnLocation, 200.0f, 12, 
                          DepositColor, false, 60.0f, 0, 10.0f);
            
            // Draw deposit name
            DrawDebugString(World, Info.SpawnLocation + FVector(0, 0, 300), 
                          Info.DepositDefinition->DepositName.ToString(), 
                          nullptr, DepositColor, 60.0f);
        }
    }
}

void UDepositSpawnManager::LogSpawnStatistics() const
{
    UE_LOG(LogTemp, Log, TEXT("=== DEPOSIT SPAWN STATISTICS ==="));
    UE_LOG(LogTemp, Log, TEXT("Total spawned deposits: %d"), SpawnedDeposits.Num());
    UE_LOG(LogTemp, Log, TEXT("Spawn area: Center=%s, Size=%s"), 
        *SpawnAreaCenter.ToString(), *SpawnAreaSize.ToString());
    
    // Count by deposit type
    TMap<UDepositDefinition*, int32> DepositCounts;
    for (const FSpawnedDepositInfo& Info : SpawnedDeposits)
    {
        if (Info.DepositDefinition)
        {
            DepositCounts.FindOrAdd(Info.DepositDefinition)++;
        }
    }
    
    for (const auto& Pair : DepositCounts)
    {
        UE_LOG(LogTemp, Log, TEXT("  %s: %d deposits"), 
            *Pair.Key->DepositName.ToString(), Pair.Value);
    }
    
    // Count by terrain type
    TMap<ETerrainType, int32> TerrainCounts;
    for (const FSpawnedDepositInfo& Info : SpawnedDeposits)
    {
        TerrainCounts.FindOrAdd(Info.TerrainType)++;
    }
    
    UE_LOG(LogTemp, Log, TEXT("Terrain distribution:"));
    for (const auto& Pair : TerrainCounts)
    {
        FString TerrainName = UEnum::GetValueAsString(Pair.Key);
        UE_LOG(LogTemp, Log, TEXT("  %s: %d deposits"), *TerrainName, Pair.Value);
    }
    
    UE_LOG(LogTemp, Log, TEXT("================================"));
}