// BlueprintDepositManager.h
// Lokalizacja: Source/FactoryNet/Public/Core/BlueprintDepositManager.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/DepositSpawnManager.h"
#include "Data/DepositDefinition.h"
#include "Components/BillboardComponent.h"
#include "Components/BoxComponent.h"
#include "TimerManager.h"
#include "BlueprintDepositManager.generated.h"

// Forward declarations
class AResourceDeposit;
class UDepositDefinition;
// class ATransportHub; // ZAKOMENTOWANE - nie istnieje jeszcze

UENUM(BlueprintType)
enum class ESpawnTriggerType : uint8
{
    Manual          UMETA(DisplayName = "Manual"),
    OnBeginPlay     UMETA(DisplayName = "On Begin Play"),
    OnPlayerEnter   UMETA(DisplayName = "On Player Enter"),
    Delayed         UMETA(DisplayName = "Delayed Start")
};

USTRUCT(BlueprintType)
struct FACTORYNET_API FBlueprintSpawnRule
{
    GENERATED_BODY()

    FBlueprintSpawnRule()
    {
        DepositType = nullptr;
        SpawnProbability = 0.1f;
        MaxCount = 5;
        MinDistance = 2000.0f;
        TerrainTypes = {ETerrainType::Plains};
        MinElevation = -1000.0f;
        MaxElevation = 1000.0f;
        bPreferCoastline = false;
    }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Rule")
    UDepositDefinition* DepositType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Rule", 
              meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SpawnProbability;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Rule")
    int32 MaxCount;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Rule")
    float MinDistance;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Rule")
    TArray<ETerrainType> TerrainTypes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Rule")
    float MinElevation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Rule")
    float MaxElevation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Rule")
    bool bPreferCoastline;
};

// ✅ DODANO BRAKUJĄCĄ STRUKTURĘ FDepositInfo
USTRUCT(BlueprintType)
struct FACTORYNET_API FDepositInfo
{
    GENERATED_BODY()

    FDepositInfo()
    {
        DepositType = nullptr;
        TotalCount = 0;
        ActiveCount = 0;
        TotalResources = 0;
    }

    // Typ depozytu
    UPROPERTY(BlueprintReadOnly, Category = "Deposit Info")
    UDepositDefinition* DepositType;

    // Całkowita liczba depozytów tego typu
    UPROPERTY(BlueprintReadOnly, Category = "Deposit Info")
    int32 TotalCount;

    // Liczba aktywnych depozytów
    UPROPERTY(BlueprintReadOnly, Category = "Deposit Info")
    int32 ActiveCount;

    // Całkowita ilość zasobów we wszystkich depozytach tego typu
    UPROPERTY(BlueprintReadOnly, Category = "Deposit Info")
    int32 TotalResources;
};

UCLASS(BlueprintType, Blueprintable)
class FACTORYNET_API ABlueprintDepositManager : public AActor
{
    GENERATED_BODY()

public:
    ABlueprintDepositManager();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    // === BLUEPRINT INTERFACE ===
    UFUNCTION(BlueprintCallable, Category = "Deposit Spawning")
    void GenerateDeposits();

    UFUNCTION(BlueprintCallable, Category = "Deposit Spawning")
    void ClearAllDeposits();

    UFUNCTION(BlueprintCallable, Category = "Deposit Spawning")
    void RegenerateDeposits();

    UFUNCTION(BlueprintCallable, Category = "Deposit Spawning")
    AResourceDeposit* SpawnDepositAtLocation(UDepositDefinition* DepositType, const FVector& Location);

    // === CONFIGURATION ===
    UFUNCTION(BlueprintCallable, Category = "Configuration")
    void SetSpawnAreaFromBounds();

    UFUNCTION(BlueprintCallable, Category = "Configuration")
    void AddCustomSpawnRule(const FBlueprintSpawnRule& CustomRule);

    UFUNCTION(BlueprintCallable, Category = "Configuration")
    void PreviewSpawnArea();

    // === QUERY FUNCTIONS ===
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Query")
    TArray<AResourceDeposit*> GetAllSpawnedDeposits();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Query")
    TArray<AResourceDeposit*> GetDepositsByType(UDepositDefinition* DepositType);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Query")
    AResourceDeposit* GetNearestDeposit(const FVector& Location, UDepositDefinition* DepositType);

    // ✅ NAPRAWIONA FUNKCJA GetDepositInfo - teraz używa poprawnie zdefiniowanej struktury FDepositInfo
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Query")
    FDepositInfo GetDepositInfo(UDepositDefinition* DepositType);

    // === DEBUG FUNCTIONS ===
    UFUNCTION(BlueprintCallable, Category = "Debug")
    void DebugSpawnedDeposits();

    // === EVENTS (Blueprint Implementable) ===
    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnDepositSpawned_BP(AResourceDeposit* SpawnedDeposit, FVector Location);

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnAllDepositsSpawned_BP(const TArray<FSpawnedDepositInfo>& SpawnedDeposits);

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnDepositsRegenerated_BP();

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnDepositsCleared_BP();

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnDepositGenerationStarted_BP();

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnSpawnAreaChanged_BP(FVector NewCenter, FVector NewSize);

public:
    // === SPAWN CONFIGURATION ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Configuration")
    ESpawnTriggerType SpawnTrigger;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Configuration",
              meta = (ClampMin = "0.1", ClampMax = "30.0"))
    float DelayTime;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Configuration")
    EDepositDensity DepositDensity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Rules")
    bool bUseDefaultSpawnRules;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Rules")
    TArray<FBlueprintSpawnRule> CustomSpawnRules;

    // === SPAWN AREA CONFIGURATION ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Area")
    bool bUseCustomBounds;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Area",
              meta = (EditCondition = "bUseCustomBounds"))
    FVector CustomSpawnCenter;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Area",
              meta = (EditCondition = "bUseCustomBounds"))
    FVector CustomSpawnSize;

    // === DEBUG & VISUALIZATION ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bLogSpawnProcess;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bShowSpawnArea;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug",
              meta = (ClampMin = "1.0", ClampMax = "60.0"))
    float DebugDisplayTime;

protected:
    // === COMPONENTS ===
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UBillboardComponent* BillboardComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UBoxComponent* SpawnAreaBounds;

    // === RUNTIME DATA ===
    UPROPERTY()
    UDepositSpawnManager* SpawnManager;

    UPROPERTY()
    bool bHasGenerated;

    UPROPERTY()
    FTimerHandle DelayedSpawnTimerHandle;

private:
    // === INTERNAL FUNCTIONS ===
    void InitializeSpawnManager();
    void SetupSpawnRules();
    FDepositSpawnRule ConvertBlueprintRule(const FBlueprintSpawnRule& BPRule);
    
    // Event handlers
    UFUNCTION()
    void OnDepositSpawned(AResourceDeposit* SpawnedDeposit, FVector SpawnLocation);
    
    UFUNCTION()
    void OnAllDepositsSpawned(const TArray<FSpawnedDepositInfo>& SpawnedDeposits);
    
    // Utility functions
    void UpdateSpawnAreaVisualization();
    bool ValidateSpawnConfiguration() const;
    void LogConfigurationSummary() const;
};