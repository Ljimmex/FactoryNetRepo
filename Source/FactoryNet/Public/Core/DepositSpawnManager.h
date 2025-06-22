// DepositSpawnManager.h
// Lokalizacja: Source/FactoryNet/Public/Core/DepositSpawnManager.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/DataTable.h"
#include "Data/DepositDefinition.h"
#include "DepositSpawnManager.generated.h"

// Forward declarations
class AResourceDeposit;
class UDataTableManager;
class UDepositDefinition;
// class ALandscape; // Commented out - not used yet

UENUM(BlueprintType)
enum class EDepositDensity : uint8
{
    Sparse      UMETA(DisplayName = "Sparse (Rzadkie)"),
    Normal      UMETA(DisplayName = "Normal (Normalne)"),
    Dense       UMETA(DisplayName = "Dense (Gęste)"),
    VeryDense   UMETA(DisplayName = "Very Dense (Bardzo Gęste)")
};

UENUM(BlueprintType)
enum class ETerrainType : uint8
{
    Plains      UMETA(DisplayName = "Plains (Równiny)"),
    Hills       UMETA(DisplayName = "Hills (Wzgórza)"),
    Mountains   UMETA(DisplayName = "Mountains (Góry)"),
    Coastline   UMETA(DisplayName = "Coastline (Wybrzeże)"),
    Forest      UMETA(DisplayName = "Forest (Las)"),
    Desert      UMETA(DisplayName = "Desert (Pustynia)"),
    Wetlands    UMETA(DisplayName = "Wetlands (Mokradła)")
};

USTRUCT(BlueprintType)
struct FACTORYNET_API FDepositSpawnRule
{
    GENERATED_BODY()

    FDepositSpawnRule()
    {
        DepositDefinition = nullptr;
        SpawnProbability = 0.1f;
        MinDistanceFromOthers = 2000.0f;
        MaxDepositCount = 10;
        PreferredTerrainTypes = {ETerrainType::Plains};
        MinElevation = -1000.0f;
        MaxElevation = 1000.0f;
        MinDistanceFromWater = 0.0f;
        PreferCoastline = false;
    }

    // Reference do definicji złoża
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deposit")
    UDepositDefinition* DepositDefinition;

    // Prawdopodobieństwo spawnu (0.0 - 1.0)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", 
              meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SpawnProbability;

    // Minimalna odległość od innych złóż
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
    float MinDistanceFromOthers;

    // Maksymalna liczba złóż tego typu
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
    int32 MaxDepositCount;

    // Preferowane typy terenu
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
    TArray<ETerrainType> PreferredTerrainTypes;

    // Ograniczenia wysokości
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
    float MinElevation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
    float MaxElevation;

    // Ograniczenia wodne
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water")
    float MinDistanceFromWater;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water")
    bool PreferCoastline;
};

USTRUCT(BlueprintType)
struct FACTORYNET_API FSpawnedDepositInfo
{
    GENERATED_BODY()

    FSpawnedDepositInfo()
    {
        SpawnedActor = nullptr;
        DepositDefinition = nullptr;
        SpawnLocation = FVector::ZeroVector;
        TerrainType = ETerrainType::Plains;
        Elevation = 0.0f;
    }

    UPROPERTY(BlueprintReadOnly, Category = "Spawn Info")
    AResourceDeposit* SpawnedActor;

    UPROPERTY(BlueprintReadOnly, Category = "Spawn Info")
    UDepositDefinition* DepositDefinition;

    UPROPERTY(BlueprintReadOnly, Category = "Spawn Info")
    FVector SpawnLocation;

    UPROPERTY(BlueprintReadOnly, Category = "Spawn Info")
    ETerrainType TerrainType;

    UPROPERTY(BlueprintReadOnly, Category = "Spawn Info")
    float Elevation;
};

// Delegate declarations
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDepositSpawned, AResourceDeposit*, SpawnedDeposit, FVector, SpawnLocation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAllDepositsSpawned, const TArray<FSpawnedDepositInfo>&, SpawnedDeposits);

UCLASS(BlueprintType)
class FACTORYNET_API UDepositSpawnManager : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UDepositSpawnManager();

    // USubsystem Interface
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // === MAIN SPAWN FUNCTIONS ===
    UFUNCTION(BlueprintCallable, Category = "Deposit Spawning")
    void GenerateDepositsOnMap();

    UFUNCTION(BlueprintCallable, Category = "Deposit Spawning")
    void ClearAllSpawnedDeposits();

    UFUNCTION(BlueprintCallable, Category = "Deposit Spawning")
    AResourceDeposit* SpawnDepositAtLocation(UDepositDefinition* DepositDef, 
                                           const FVector& Location, 
                                           const FRotator& Rotation = FRotator::ZeroRotator);

    // === CONFIGURATION ===
    UFUNCTION(BlueprintCallable, Category = "Configuration")
    void SetSpawnArea(const FVector& Center, const FVector& Size);

    UFUNCTION(BlueprintCallable, Category = "Configuration")
    void AddSpawnRule(const FDepositSpawnRule& SpawnRule);

    UFUNCTION(BlueprintCallable, Category = "Configuration")
    void ClearSpawnRules();

    UFUNCTION(BlueprintCallable, Category = "Configuration")
    void SetDepositDensity(EDepositDensity NewDensity);

    // ✅ DODANO: Debug testing function
    UFUNCTION(BlueprintCallable, Category = "Debug")
    void TestProbabilityGeneration(float TestProbability = 0.5f, int32 TestCount = 100);

    // === QUERY FUNCTIONS ===
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Query")
    TArray<AResourceDeposit*> GetAllSpawnedDeposits() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Query")
    TArray<AResourceDeposit*> GetDepositsByType(UDepositDefinition* DepositType) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Query")
    AResourceDeposit* GetNearestDepositOfType(const FVector& Location, UDepositDefinition* DepositType) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Query")
    float GetMinimumDistanceBetweenDeposits(UDepositDefinition* DepositType) const;

    // === TERRAIN ANALYSIS ===
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terrain")
    ETerrainType AnalyzeTerrainType(const FVector& Location) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terrain")
    float GetElevationAtLocation(const FVector& Location) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terrain")
    bool IsLocationNearWater(const FVector& Location, float MaxDistance = 5000.0f) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terrain")
    bool IsValidSpawnLocation(const FVector& Location, const FDepositSpawnRule& SpawnRule) const;

    // === EVENTS ===
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnDepositSpawned OnDepositSpawned;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnAllDepositsSpawned OnAllDepositsSpawned;

protected:
    // === SPAWN CONFIGURATION ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Configuration")
    TArray<FDepositSpawnRule> SpawnRules;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Configuration")
    EDepositDensity DepositDensity = EDepositDensity::Normal;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Area")
    FVector SpawnAreaCenter = FVector(0.0f, 0.0f, 0.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Area")
    FVector SpawnAreaSize = FVector(50000.0f, 50000.0f, 10000.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Configuration")
    int32 MaxSpawnAttempts = 1000;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Configuration")
    int32 GridResolution = 100;
    

    // === RUNTIME DATA ===
    UPROPERTY()
    TArray<FSpawnedDepositInfo> SpawnedDeposits;

    UPROPERTY()
    UDataTableManager* DataTableManager;

private:
    // === INTERNAL FUNCTIONS ===
    void LoadDefaultSpawnRules();
    void CreateFallbackSpawnRules();
    TArray<FVector> GenerateSpawnCandidates();
    bool ValidateSpawnLocation(const FVector& Location, const FDepositSpawnRule& Rule);
    UDepositDefinition* SelectDepositTypeForLocation(const FVector& Location);
    void SpawnDepositFromRule(const FDepositSpawnRule& Rule, const FVector& Location);
    
    // Terrain analysis helpers
    float CalculateSlope(const FVector& Location) const;
    bool IsLocationInWater(const FVector& Location) const;
    
    // ✅ NAPRAWIONE: Dodano const qualifier do funkcji IsMinimumDistanceRespected
    // Distance checking - MUSI być const, żeby można było wywoływać z const funkcji IsValidSpawnLocation
    bool IsMinimumDistanceRespected(const FVector& Location, UDepositDefinition* DepositType, float MinDistance) const;

    // ✅ DODANO: Nowa funkcja collision checking
    bool IsLocationSafeForSpawn(const FVector& Location, float MinDistance) const;
    
    // Debug helpers
    void DrawDebugSpawnArea() const;
    void LogSpawnStatistics() const;
    
    // Utility functions
    float GetDensityMultiplier() const;
};