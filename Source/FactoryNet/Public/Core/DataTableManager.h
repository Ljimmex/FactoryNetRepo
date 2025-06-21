// DataTableManager.h
// Lokalizacja: Source/FactoryNet/Public/Core/DataTableManager.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/DataTable.h"
#include "Data/ResourceData.h"
#include "Data/ProductionData.h"
#include "Data/TransportData.h"
#include "Data/UpgradeData.h"
#include "DataTableManager.generated.h"

// Forward Declarations
class UFactoryDefinition;
class UHubDefinition;
class UVehicleDefinition;
class URoadDefinition;
class UDepositDefinition;
class UDemandDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDataTablesLoaded);

UCLASS(BlueprintType)
class FACTORYNET_API UDataTableManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UDataTableManager();

    // USubsystem Interface
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // === INITIALIZATION ===
    UFUNCTION(BlueprintCallable, Category = "Data Management")
    void LoadAllDataTables();

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnDataTablesLoaded OnDataTablesLoaded;

    // === RESOURCE FUNCTIONS ===
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Resources")
    bool GetResourceDataByReference(const FDataTableRowHandle& ResourceReference, FResourceTableRow& OutResourceData);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Resources")
    TArray<FResourceTableRow> GetAllResources();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Resources")
    TArray<FResourceTableRow> GetResourcesByType(EResourceType ResourceType);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Resources")
    bool IsValidResourceReference(const FDataTableRowHandle& ResourceReference);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Resources")
    FString GetResourceNameFromReference(const FDataTableRowHandle& ResourceReference);

    // === PRODUCTION FUNCTIONS ===
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Production")
    bool GetProductionRecipeByReference(const FDataTableRowHandle& RecipeReference, FProductionRecipe& OutRecipe);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Production")
    TArray<FProductionRecipe> GetAllRecipes();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Production")
    TArray<FProductionRecipe> GetRecipesForFactory(UFactoryDefinition* FactoryDef);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Production")
    TArray<FProductionRecipe> GetRecipesByOutputResource(const FDataTableRowHandle& ResourceReference);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Production")
    FString GetRecipeNameFromReference(const FDataTableRowHandle& RecipeReference);

    // === TRANSPORT FUNCTIONS ===
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Transport")
    bool GetTransportRouteByReference(const FDataTableRowHandle& RouteReference, FTransportRoute& OutRoute);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Transport")
    TArray<FTransportRoute> GetAllRoutes();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Transport")
    TArray<FTransportRoute> GetRoutesFromHub(UHubDefinition* HubDef);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Transport")
    TArray<FTransportRoute> GetRoutesToHub(UHubDefinition* HubDef);

    // === UPGRADE FUNCTIONS ===
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Upgrades")
    bool GetUpgradeDataByReference(const FDataTableRowHandle& UpgradeReference, FUpgradeTableRow& OutUpgradeData);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Upgrades")
    TArray<FUpgradeTableRow> GetAllUpgrades();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Upgrades")
    TArray<FUpgradeTableRow> GetUpgradesByCategory(EUpgradeCategory Category);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Upgrades")
    TArray<FUpgradeTableRow> GetUpgradesByType(EUpgradeType Type);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Upgrades")
    TArray<FUpgradeTableRow> GetUpgradesByTechLevel(int32 TechLevel);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Upgrades")
    bool IsValidUpgradeReference(const FDataTableRowHandle& UpgradeReference);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Upgrades")
    FString GetUpgradeNameFromReference(const FDataTableRowHandle& UpgradeReference);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Upgrades")
    bool AreUpgradePrerequisitesMet(const FDataTableRowHandle& UpgradeReference, const TArray<FDataTableRowHandle>& CompletedUpgrades);

    // === DATAASSET FUNCTIONS ===
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Factory Definitions")
    UFactoryDefinition* GetFactoryDefinitionByName(const FString& FactoryName);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Hub Definitions")
    UHubDefinition* GetHubDefinitionByName(const FString& HubName);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Vehicle Definitions")
    UVehicleDefinition* GetVehicleDefinitionByName(const FString& VehicleName);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Road Definitions")
    URoadDefinition* GetRoadDefinitionByName(const FString& RoadName);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Deposit Definitions")
    UDepositDefinition* GetDepositDefinitionByName(const FString& DepositName);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Demand Definitions")
    UDemandDefinition* GetDemandDefinitionByName(const FString& DemandName);

    // === UTILITY FUNCTIONS ===
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Utility")
    bool AreDataTablesLoaded() const;

    UFUNCTION(BlueprintCallable, Category = "Utility")
    void RefreshDataTables();

    // === HELPER FUNCTIONS ===
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Helper")
    FDataTableRowHandle FindResourceReferenceByName(const FString& ResourceName);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Helper")
    FDataTableRowHandle FindRecipeReferenceByName(const FString& RecipeName);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Helper")
    FDataTableRowHandle FindUpgradeReferenceByName(const FString& UpgradeName);

    // === DEBUG FUNCTIONS ===
    UFUNCTION(BlueprintCallable, Category = "Debug")
    void PrintAllResourceData();

    UFUNCTION(BlueprintCallable, Category = "Debug")
    void PrintAllRecipeData();

    UFUNCTION(BlueprintCallable, Category = "Debug")
    void PrintAllUpgradeData();

    UFUNCTION(BlueprintCallable, Category = "Debug")
    void ValidateDataIntegrity();

protected:
    // === DATATABLE REFERENCES ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data Tables")
    UDataTable* ResourceDataTable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data Tables")
    UDataTable* ProductionDataTable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data Tables")
    UDataTable* TransportDataTable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data Tables")
    UDataTable* UpgradeDataTable;

    // === DATAASSET COLLECTIONS ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data Assets")
    TArray<UFactoryDefinition*> FactoryDefinitions;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data Assets")
    TArray<UHubDefinition*> HubDefinitions;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data Assets")
    TArray<UVehicleDefinition*> VehicleDefinitions;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data Assets")
    TArray<URoadDefinition*> RoadDefinitions;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data Assets")
    TArray<UDepositDefinition*> DepositDefinitions;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data Assets")
    TArray<UDemandDefinition*> DemandDefinitions;

private:
    // === STATE ===
    bool bDataTablesLoaded = false;
    bool bDataAssetsLoaded = false;

    // === HELPER FUNCTIONS (nie UFUNCTION - tylko do użytku wewnętrznego) ===
    void LoadDataAssets();
    bool ValidateResourceReferences();
    bool ValidateProductionRecipes();
    bool ValidateUpgradeReferences();
    void LogDataTableStats();
    
    // Internal helper functions (nie Blueprint callable)
    FResourceTableRow* GetResourceDataInternal(const FDataTableRowHandle& ResourceReference);
    FProductionRecipe* GetProductionRecipeInternal(const FDataTableRowHandle& RecipeReference);
    FUpgradeTableRow* GetUpgradeDataInternal(const FDataTableRowHandle& UpgradeReference);
    
    // Helper to check if DataTableRowHandle is valid
    bool IsDataTableRowHandleValid(const FDataTableRowHandle& Handle) const;
};