// BlueprintDepositManager.h
// Lokalizacja: Source/FactoryNet/Public/Core/BlueprintDepositManager.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/DepositSpawnManager.h"
#include "Components/BillboardComponent.h"
#include "Components/BoxComponent.h"
#include "BlueprintDepositManager.generated.h"

// Forward declarations
// class ATransportHub; // ZAKOMENTOWANE - nie istnieje jeszcze
class AResourceDeposit;

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
    AResourceDeposit* SpawnDepositAt(UDepositDefinition* DepositType, FVector Location);

    // === CONFIGURATION ===
    UFUNCTION(BlueprintCallable, Category = "Configuration")
    void SetSpawnAreaFromBounds();

    UFUNCTION(BlueprintCallable, Category = "Configuration")
    void AddCustomSpawnRule(const FBlueprintSpawnRule& CustomRule);

    UFUNCTION(BlueprintCallable, Category = "Configuration")
    void PreviewSpawnArea();

    // === QUERY FUNCTIONS ===
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Query")
    int32 GetTotalSpawnedDeposits() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Query")
    TArray<AResourceDeposit*> GetSpawnedDepositsByType(UDepositDefinition* DepositType) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Query")
    AResourceDeposit* FindNearestDeposit(FVector Location, UDepositDefinition* DepositType = nullptr) const;

    // === EVENTS (Blueprint Implementable) ===
    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnDepositSpawned_BP(AResourceDeposit* SpawnedDeposit, FVector Location);

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnAllDepositsGenerated_BP(int32 TotalSpawned);

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnSpawnRuleProcessed_BP(UDepositDefinition* DepositType, int32 SpawnedCount, int32 MaxCount);

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnSpawnAreaChanged_BP(FVector NewCenter, FVector NewSize);

protected:
    // === COMPONENTS ===
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UBillboardComponent* BillboardComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UBoxComponent* SpawnAreaBounds;

    // === SPAWN CONFIGURATION ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Configuration")
    ESpawnTriggerType SpawnTrigger = ESpawnTriggerType::OnBeginPlay;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Configuration", 
              meta = (EditCondition = "SpawnTrigger == ESpawnTriggerType::Delayed"))
    float DelayTime = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Configuration")
    EDepositDensity DepositDensity = EDepositDensity::Normal;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Configuration")
    TArray<FBlueprintSpawnRule> CustomSpawnRules;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Configuration")
    bool bUseDefaultSpawnRules = true;

    // HUB RELATED (DISABLED FOR NOW - no TransportHub implementation)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hub Configuration (Disabled)", 
              meta = (EditCondition = "false", ToolTip = "Disabled until TransportHub is implemented"))
    bool bAutoConnectToNearestHub = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hub Configuration (Disabled)", 
              meta = (EditCondition = "false", ToolTip = "Disabled until TransportHub is implemented"))
    float HubSearchRadius = 5000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hub Configuration (Disabled)", 
              meta = (EditCondition = "false", ToolTip = "Disabled until TransportHub is implemented"))
    bool bAutoCreateHubsIfNeeded = false;

    // === ADVANCED SETTINGS ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced")
    int32 MaxSpawnAttempts = 1000;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced")
    int32 GridResolution = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced")
    bool bUseCustomBounds = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced", 
              meta = (EditCondition = "bUseCustomBounds"))
    FVector CustomSpawnCenter = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced", 
              meta = (EditCondition = "bUseCustomBounds"))
    FVector CustomSpawnSize = FVector(10000.0f, 10000.0f, 5000.0f);

    // === VISUAL & DEBUG ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    bool bShowSpawnArea = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bShowDebugInfo = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bLogSpawnProcess = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bDrawDebugSpheres = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    float DebugDisplayTime = 60.0f;

private:
    // === INTERNAL STATE ===
    UPROPERTY()
    UDepositSpawnManager* SpawnManager;

    UPROPERTY()
    TArray<AResourceDeposit*> SpawnedDeposits;

    bool bHasGenerated = false;

    // === INTERNAL FUNCTIONS ===
    void InitializeSpawnManager();
    void SetupSpawnRules();
    void SetupSpawnArea();
    
    // HUB RELATED (DISABLED FOR NOW)
    // void ConnectDepositsToHubs();
    // ATransportHub* FindNearestHub(const FVector& Location) const;
    // void SpawnHubForDeposit(AResourceDeposit* Deposit);
    
    // Event handlers
    UFUNCTION()
    void OnDepositSpawned(AResourceDeposit* Deposit, FVector Location);

    UFUNCTION()
    void OnAllDepositsSpawned(const TArray<FSpawnedDepositInfo>& SpawnedInfo);

    // Timer functions
    UFUNCTION()
    void DelayedGeneration();

    // Helper functions
    FDepositSpawnRule ConvertBlueprintRule(const FBlueprintSpawnRule& BPRule) const;
    void UpdateSpawnAreaVisualization();
    void DrawDebugDeposits() const;
    
    // Validation functions
    bool ValidateSpawnConfiguration() const;
    void LogConfigurationSummary() const;
};