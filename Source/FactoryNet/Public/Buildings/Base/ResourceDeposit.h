// ResourceDeposit.h
// Lokalizacja: Source/FactoryNet/Public/Buildings/Base/ResourceDeposit.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/DataTable.h"
#include "Data/DepositDefinition.h"
#include "Components/ResourceStorageComponent.h"
#include "ResourceDeposit.generated.h"

// Forward declarations
class UDepositDefinition;
// Temporarily commented out until TransportHub is implemented
// class ATransportHub;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnResourceExtracted, AResourceDeposit*, Deposit, FDataTableRowHandle, ResourceType, int32, Amount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDepositDepleted, AResourceDeposit*, Deposit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDepositLevelChanged, AResourceDeposit*, Deposit, int32, NewLevel);

UCLASS(BlueprintType)
class FACTORYNET_API AResourceDeposit : public AActor
{
    GENERATED_BODY()

public:
    AResourceDeposit();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    // === INITIALIZATION ===
    UFUNCTION(BlueprintCallable, Category = "Deposit")
    void InitializeWithDefinition(UDepositDefinition* DepositDef);

    UFUNCTION(BlueprintCallable, Category = "Deposit")
    void InitializeFromSpawn(UDepositDefinition* DepositDef, int32 InitialLevel = 1);

    // === EXTRACTION ===
    UFUNCTION(BlueprintCallable, Category = "Extraction")
    int32 ExtractResource(int32 RequestedAmount);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Extraction")
    bool CanExtractResource(int32 RequestedAmount) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Extraction")
    float GetCurrentExtractionRate() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Extraction")
    int32 GetAvailableResource() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Extraction")
    int32 GetMaxStorage() const;

    // === LEVEL MANAGEMENT ===
    UFUNCTION(BlueprintCallable, Category = "Level")
    bool UpgradeToLevel(int32 TargetLevel);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Level")
    bool CanUpgradeToLevel(int32 TargetLevel) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Level")
    float GetUpgradeCost(int32 TargetLevel) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Level")
    int32 GetCurrentLevel() const { return CurrentLevel; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Level")
    int32 GetMaxLevel() const;

    // === HUB INTEGRATION (TEMPORARILY DISABLED) ===
    // Uncomment when TransportHub is implemented
    /*
    UFUNCTION(BlueprintCallable, Category = "Hub")
    void ConnectToHub(ATransportHub* Hub);

    UFUNCTION(BlueprintCallable, Category = "Hub")
    void DisconnectFromHub();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Hub")
    ATransportHub* GetConnectedHub() const { return ConnectedHub; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Hub")
    bool IsConnectedToHub() const { return ConnectedHub != nullptr; }
    */

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Hub")
    bool RequiresHub() const;

    // === RESOURCE INFO ===
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Resource")
    FDataTableRowHandle GetResourceType() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Resource")
    FText GetDepositName() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Resource")
    bool IsRenewable() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Resource")
    bool IsDepleted() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Resource")
    float GetDepletionPercentage() const;

    // === VISUAL UPDATES ===
    UFUNCTION(BlueprintCallable, Category = "Visual")
    void UpdateVisualMesh();

    UFUNCTION(BlueprintImplementableEvent, Category = "Visual")
    void OnDepositLevelChanged_BP(int32 NewLevel);

    UFUNCTION(BlueprintImplementableEvent, Category = "Visual")
    void OnResourceExtracted_BP(int32 Amount);

    UFUNCTION(BlueprintImplementableEvent, Category = "Visual")
    void OnDepositDepleted_BP();

    // === EVENTS ===
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnResourceExtracted OnResourceExtracted;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnDepositDepleted OnDepositDepleted;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnDepositLevelChanged OnDepositLevelChanged;

protected:
    // === COMPONENTS ===
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* RootSceneComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* DepositMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UResourceStorageComponent* StorageComponent;

    // === DEPOSIT CONFIGURATION ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deposit Configuration")
    UDepositDefinition* DepositDefinition;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deposit State")
    int32 CurrentLevel = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deposit State")
    int32 CurrentReserves;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deposit State")
    float LastExtractionTime;

    // === HUB CONNECTION (TEMPORARILY DISABLED) ===
    // Uncomment when TransportHub is implemented
    /*
    UPROPERTY(BlueprintReadOnly, Category = "Hub")
    ATransportHub* ConnectedHub;
    */

    // === AUTO EXTRACTION ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto Extraction")
    bool bAutoExtractToStorage = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto Extraction")
    float ExtractionTickRate = 1.0f;

    // === DEBUG ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bShowDebugInfo = false;

private:
    // === INTERNAL FUNCTIONS ===
    void TickAutoExtraction(float DeltaTime);
    void UpdateMeshForLevel();
    void RegenerateResource(float DeltaTime);
    FDepositLevel GetCurrentLevelData() const;
    void BroadcastExtractionEvent(int32 Amount);
    void CheckForDepletion();

    // === INTERNAL STATE ===
    float TimeSinceLastExtraction = 0.0f;
    bool bHasBeenInitialized = false;
};