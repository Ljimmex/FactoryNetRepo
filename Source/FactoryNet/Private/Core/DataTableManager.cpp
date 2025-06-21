// DataTableManager.cpp
// Lokalizacja: Source/FactoryNet/Private/Core/DataTableManager.cpp

#include "Core/DataTableManager.h"
#include "Data/FactoryDefinition.h"
#include "Data/HubDefinition.h"
#include "Data/VehicleDefinition.h"
#include "Data/RoadDefinition.h"
#include "Data/DepositDefinition.h"
#include "Data/DemandDefinition.h"
#include "Data/UpgradeData.h"
#include "Engine/World.h"

UDataTableManager::UDataTableManager()
{
    ResourceDataTable = nullptr;
    ProductionDataTable = nullptr;
    TransportDataTable = nullptr;
    UpgradeDataTable = nullptr;
}

void UDataTableManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
    UE_LOG(LogTemp, Log, TEXT("DataTableManager: Initializing with reference system..."));
    
    LoadAllDataTables();
}

void UDataTableManager::Deinitialize()
{
    ResourceDataTable = nullptr;
    ProductionDataTable = nullptr;
    TransportDataTable = nullptr;
    UpgradeDataTable = nullptr;
    
    FactoryDefinitions.Empty();
    HubDefinitions.Empty();
    VehicleDefinitions.Empty();
    RoadDefinitions.Empty();
    DepositDefinitions.Empty();
    DemandDefinitions.Empty();
    
    bDataTablesLoaded = false;
    bDataAssetsLoaded = false;
    
    Super::Deinitialize();
}

void UDataTableManager::LoadAllDataTables()
{
    UE_LOG(LogTemp, Log, TEXT("DataTableManager: Loading all data tables..."));
    
    bDataTablesLoaded = (ResourceDataTable != nullptr && 
                        ProductionDataTable != nullptr && 
                        TransportDataTable != nullptr &&
                        UpgradeDataTable != nullptr);
    
    if (bDataTablesLoaded)
    {
        UE_LOG(LogTemp, Log, TEXT("DataTableManager: DataTables loaded successfully"));
        
        LoadDataAssets();
        ValidateDataIntegrity();
        LogDataTableStats();
        OnDataTablesLoaded.Broadcast();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("DataTableManager: Some DataTables not assigned"));
        UE_LOG(LogTemp, Warning, TEXT("ResourceDataTable: %s"), ResourceDataTable ? TEXT("OK") : TEXT("NULL"));
        UE_LOG(LogTemp, Warning, TEXT("ProductionDataTable: %s"), ProductionDataTable ? TEXT("OK") : TEXT("NULL"));
        UE_LOG(LogTemp, Warning, TEXT("TransportDataTable: %s"), TransportDataTable ? TEXT("OK") : TEXT("NULL"));
        UE_LOG(LogTemp, Warning, TEXT("UpgradeDataTable: %s"), UpgradeDataTable ? TEXT("OK") : TEXT("NULL"));
    }
}

void UDataTableManager::LoadDataAssets()
{
    UE_LOG(LogTemp, Log, TEXT("DataTableManager: DataAssets are directly assigned as arrays"));
    bDataAssetsLoaded = true;
}

// === RESOURCE FUNCTIONS ===
bool UDataTableManager::GetResourceDataByReference(const FDataTableRowHandle& ResourceReference, FResourceTableRow& OutResourceData)
{
    FResourceTableRow* FoundRow = GetResourceDataInternal(ResourceReference);
    if (FoundRow)
    {
        OutResourceData = *FoundRow;
        return true;
    }
    
    return false;
}

TArray<FResourceTableRow> UDataTableManager::GetAllResources()
{
    TArray<FResourceTableRow> AllResources;
    
    if (ResourceDataTable)
    {
        TArray<FResourceTableRow*> RowPointers;
        ResourceDataTable->GetAllRows<FResourceTableRow>(TEXT("GetAllResources"), RowPointers);
        
        for (FResourceTableRow* Row : RowPointers)
        {
            if (Row)
            {
                AllResources.Add(*Row);
            }
        }
    }
    
    return AllResources;
}

TArray<FResourceTableRow> UDataTableManager::GetResourcesByType(EResourceType ResourceType)
{
    TArray<FResourceTableRow> FilteredResources;
    TArray<FResourceTableRow> AllResources = GetAllResources();
    
    for (const FResourceTableRow& Resource : AllResources)
    {
        if (Resource.ResourceType == ResourceType)
        {
            FilteredResources.Add(Resource);
        }
    }
    
    return FilteredResources;
}

bool UDataTableManager::IsValidResourceReference(const FDataTableRowHandle& ResourceReference)
{
    return GetResourceDataInternal(ResourceReference) != nullptr;
}

FString UDataTableManager::GetResourceNameFromReference(const FDataTableRowHandle& ResourceReference)
{
    FResourceTableRow* ResourceData = GetResourceDataInternal(ResourceReference);
    if (ResourceData)
    {
        return ResourceData->ResourceName.ToString();
    }
    
    return TEXT("Unknown Resource");
}

// === PRODUCTION FUNCTIONS ===
bool UDataTableManager::GetProductionRecipeByReference(const FDataTableRowHandle& RecipeReference, FProductionRecipe& OutRecipe)
{
    FProductionRecipe* FoundRow = GetProductionRecipeInternal(RecipeReference);
    if (FoundRow)
    {
        OutRecipe = *FoundRow;
        return true;
    }
    
    return false;
}

TArray<FProductionRecipe> UDataTableManager::GetAllRecipes()
{
    TArray<FProductionRecipe> AllRecipes;
    
    if (ProductionDataTable)
    {
        TArray<FProductionRecipe*> RowPointers;
        ProductionDataTable->GetAllRows<FProductionRecipe>(TEXT("GetAllRecipes"), RowPointers);
        
        for (FProductionRecipe* Row : RowPointers)
        {
            if (Row)
            {
                AllRecipes.Add(*Row);
            }
        }
    }
    
    return AllRecipes;
}

TArray<FProductionRecipe> UDataTableManager::GetRecipesForFactory(UFactoryDefinition* FactoryDef)
{
    TArray<FProductionRecipe> FactoryRecipes;
    
    if (!FactoryDef)
    {
        return FactoryRecipes;
    }
    
    for (const FDataTableRowHandle& RecipeRef : FactoryDef->SupportedRecipes)
    {
        FProductionRecipe Recipe;
        if (GetProductionRecipeByReference(RecipeRef, Recipe))
        {
            FactoryRecipes.Add(Recipe);
        }
    }
    
    return FactoryRecipes;
}

TArray<FProductionRecipe> UDataTableManager::GetRecipesByOutputResource(const FDataTableRowHandle& ResourceReference)
{
    TArray<FProductionRecipe> OutputRecipes;
    TArray<FProductionRecipe> AllRecipes = GetAllRecipes();
    
    for (const FProductionRecipe& Recipe : AllRecipes)
    {
        if (Recipe.OutputResourceReference.RowName == ResourceReference.RowName && 
            Recipe.OutputResourceReference.DataTable == ResourceReference.DataTable)
        {
            OutputRecipes.Add(Recipe);
        }
    }
    
    return OutputRecipes;
}

FString UDataTableManager::GetRecipeNameFromReference(const FDataTableRowHandle& RecipeReference)
{
    FProductionRecipe* RecipeData = GetProductionRecipeInternal(RecipeReference);
    if (RecipeData)
    {
        return RecipeData->RecipeName.ToString();
    }
    
    return TEXT("Unknown Recipe");
}

// === TRANSPORT FUNCTIONS ===
bool UDataTableManager::GetTransportRouteByReference(const FDataTableRowHandle& RouteReference, FTransportRoute& OutRoute)
{
    if (!IsDataTableRowHandleValid(RouteReference))
    {
        return false;
    }
    
    FTransportRoute* FoundRow = RouteReference.GetRow<FTransportRoute>(TEXT("GetTransportRouteByReference"));
    if (FoundRow)
    {
        OutRoute = *FoundRow;
        return true;
    }
    
    return false;
}

TArray<FTransportRoute> UDataTableManager::GetAllRoutes()
{
    TArray<FTransportRoute> AllRoutes;
    
    if (TransportDataTable)
    {
        TArray<FTransportRoute*> RowPointers;
        TransportDataTable->GetAllRows<FTransportRoute>(TEXT("GetAllRoutes"), RowPointers);
        
        for (FTransportRoute* Row : RowPointers)
        {
            if (Row)
            {
                AllRoutes.Add(*Row);
            }
        }
    }
    
    return AllRoutes;
}

TArray<FTransportRoute> UDataTableManager::GetRoutesFromHub(UHubDefinition* HubDef)
{
    TArray<FTransportRoute> FromRoutes;
    TArray<FTransportRoute> AllRoutes = GetAllRoutes();
    
    for (const FTransportRoute& Route : AllRoutes)
    {
        if (Route.StartHubReference.Get() == HubDef)
        {
            FromRoutes.Add(Route);
        }
    }
    
    return FromRoutes;
}

TArray<FTransportRoute> UDataTableManager::GetRoutesToHub(UHubDefinition* HubDef)
{
    TArray<FTransportRoute> ToRoutes;
    TArray<FTransportRoute> AllRoutes = GetAllRoutes();
    
    for (const FTransportRoute& Route : AllRoutes)
    {
        if (Route.EndHubReference.Get() == HubDef)
        {
            ToRoutes.Add(Route);
        }
    }
    
    return ToRoutes;
}

// === UPGRADE FUNCTIONS ===
bool UDataTableManager::GetUpgradeDataByReference(const FDataTableRowHandle& UpgradeReference, FUpgradeTableRow& OutUpgradeData)
{
    FUpgradeTableRow* FoundRow = GetUpgradeDataInternal(UpgradeReference);
    if (FoundRow)
    {
        OutUpgradeData = *FoundRow;
        return true;
    }
    
    return false;
}

TArray<FUpgradeTableRow> UDataTableManager::GetAllUpgrades()
{
    TArray<FUpgradeTableRow> AllUpgrades;
    
    if (UpgradeDataTable)
    {
        TArray<FUpgradeTableRow*> RowPointers;
        UpgradeDataTable->GetAllRows<FUpgradeTableRow>(TEXT("GetAllUpgrades"), RowPointers);
        
        for (FUpgradeTableRow* Row : RowPointers)
        {
            if (Row)
            {
                AllUpgrades.Add(*Row);
            }
        }
    }
    
    return AllUpgrades;
}

TArray<FUpgradeTableRow> UDataTableManager::GetUpgradesByCategory(EUpgradeCategory Category)
{
    TArray<FUpgradeTableRow> FilteredUpgrades;
    TArray<FUpgradeTableRow> AllUpgrades = GetAllUpgrades();
    
    for (const FUpgradeTableRow& Upgrade : AllUpgrades)
    {
        if (Upgrade.UpgradeCategory == Category)
        {
            FilteredUpgrades.Add(Upgrade);
        }
    }
    
    return FilteredUpgrades;
}

TArray<FUpgradeTableRow> UDataTableManager::GetUpgradesByType(EUpgradeType Type)
{
    TArray<FUpgradeTableRow> FilteredUpgrades;
    TArray<FUpgradeTableRow> AllUpgrades = GetAllUpgrades();
    
    for (const FUpgradeTableRow& Upgrade : AllUpgrades)
    {
        if (Upgrade.UpgradeType == Type)
        {
            FilteredUpgrades.Add(Upgrade);
        }
    }
    
    return FilteredUpgrades;
}

TArray<FUpgradeTableRow> UDataTableManager::GetUpgradesByTechLevel(int32 TechLevel)
{
    TArray<FUpgradeTableRow> FilteredUpgrades;
    TArray<FUpgradeTableRow> AllUpgrades = GetAllUpgrades();
    
    for (const FUpgradeTableRow& Upgrade : AllUpgrades)
    {
        if (Upgrade.TechLevel == TechLevel)
        {
            FilteredUpgrades.Add(Upgrade);
        }
    }
    
    return FilteredUpgrades;
}

bool UDataTableManager::IsValidUpgradeReference(const FDataTableRowHandle& UpgradeReference)
{
    return GetUpgradeDataInternal(UpgradeReference) != nullptr;
}

FString UDataTableManager::GetUpgradeNameFromReference(const FDataTableRowHandle& UpgradeReference)
{
    FUpgradeTableRow* UpgradeData = GetUpgradeDataInternal(UpgradeReference);
    if (UpgradeData)
    {
        return UpgradeData->UpgradeName.ToString();
    }
    
    return TEXT("Unknown Upgrade");
}

bool UDataTableManager::AreUpgradePrerequisitesMet(const FDataTableRowHandle& UpgradeReference, const TArray<FDataTableRowHandle>& CompletedUpgrades)
{
    FUpgradeTableRow* UpgradeData = GetUpgradeDataInternal(UpgradeReference);
    if (!UpgradeData)
    {
        return false;
    }
    
    // Sprawdź wszystkie wymagane prerequisite
    for (const FUpgradeRequirement& Prerequisite : UpgradeData->Prerequisites)
    {
        bool bPrerequisiteMet = false;
        
        // Sprawdź czy wymagany upgrade jest w liście ukończonych
        for (const FDataTableRowHandle& CompletedUpgrade : CompletedUpgrades)
        {
            if (CompletedUpgrade.RowName == Prerequisite.RequiredUpgradeReference.RowName &&
                CompletedUpgrade.DataTable == Prerequisite.RequiredUpgradeReference.DataTable)
            {
                bPrerequisiteMet = true;
                break;
            }
        }
        
        // Jeśli prerequisite nie jest spełniony i nie jest opcjonalny, zwróć false
        if (!bPrerequisiteMet && !Prerequisite.IsOptional)
        {
            return false;
        }
    }
    
    return true;
}

// === DATAASSET FUNCTIONS ===
UFactoryDefinition* UDataTableManager::GetFactoryDefinitionByName(const FString& FactoryName)
{
    for (UFactoryDefinition* Factory : FactoryDefinitions)
    {
        if (Factory && Factory->FactoryName.ToString() == FactoryName)
        {
            return Factory;
        }
    }
    
    return nullptr;
}

UHubDefinition* UDataTableManager::GetHubDefinitionByName(const FString& HubName)
{
    for (UHubDefinition* Hub : HubDefinitions)
    {
        if (Hub && Hub->HubName.ToString() == HubName)
        {
            return Hub;
        }
    }
    
    return nullptr;
}

UVehicleDefinition* UDataTableManager::GetVehicleDefinitionByName(const FString& VehicleName)
{
    for (UVehicleDefinition* Vehicle : VehicleDefinitions)
    {
        if (Vehicle && Vehicle->VehicleName.ToString() == VehicleName)
        {
            return Vehicle;
        }
    }
    
    return nullptr;
}

URoadDefinition* UDataTableManager::GetRoadDefinitionByName(const FString& RoadName)
{
    for (URoadDefinition* Road : RoadDefinitions)
    {
        if (Road && Road->RoadName.ToString() == RoadName)
        {
            return Road;
        }
    }
    
    return nullptr;
}

UDepositDefinition* UDataTableManager::GetDepositDefinitionByName(const FString& DepositName)
{
    for (UDepositDefinition* Deposit : DepositDefinitions)
    {
        if (Deposit && Deposit->DepositName.ToString() == DepositName)
        {
            return Deposit;
        }
    }
    
    return nullptr;
}

UDemandDefinition* UDataTableManager::GetDemandDefinitionByName(const FString& DemandName)
{
    for (UDemandDefinition* Demand : DemandDefinitions)
    {
        if (Demand && Demand->DemandPointName.ToString() == DemandName)
        {
            return Demand;
        }
    }
    
    return nullptr;
}

// === UTILITY FUNCTIONS ===
bool UDataTableManager::AreDataTablesLoaded() const
{
    return bDataTablesLoaded && bDataAssetsLoaded;
}

void UDataTableManager::RefreshDataTables()
{
    UE_LOG(LogTemp, Log, TEXT("DataTableManager: Refreshing data tables..."));
    LoadAllDataTables();
}

// === HELPER FUNCTIONS ===
FDataTableRowHandle UDataTableManager::FindResourceReferenceByName(const FString& ResourceName)
{
    FDataTableRowHandle EmptyHandle;
    
    if (!ResourceDataTable)
    {
        return EmptyHandle;
    }
    
    TArray<FName> RowNames = ResourceDataTable->GetRowNames();
    for (const FName& RowName : RowNames)
    {
        FResourceTableRow* Row = ResourceDataTable->FindRow<FResourceTableRow>(RowName, TEXT("FindResourceReferenceByName"));
        if (Row && Row->ResourceName.ToString() == ResourceName)
        {
            FDataTableRowHandle Handle;
            Handle.DataTable = ResourceDataTable;
            Handle.RowName = RowName;
            return Handle;
        }
    }
    
    return EmptyHandle;
}

FDataTableRowHandle UDataTableManager::FindRecipeReferenceByName(const FString& RecipeName)
{
    FDataTableRowHandle EmptyHandle;
    
    if (!ProductionDataTable)
    {
        return EmptyHandle;
    }
    
    TArray<FName> RowNames = ProductionDataTable->GetRowNames();
    for (const FName& RowName : RowNames)
    {
        FProductionRecipe* Row = ProductionDataTable->FindRow<FProductionRecipe>(RowName, TEXT("FindRecipeReferenceByName"));
        if (Row && Row->RecipeName.ToString() == RecipeName)
        {
            FDataTableRowHandle Handle;
            Handle.DataTable = ProductionDataTable;
            Handle.RowName = RowName;
            return Handle;
        }
    }
    
    return EmptyHandle;
}

FDataTableRowHandle UDataTableManager::FindUpgradeReferenceByName(const FString& UpgradeName)
{
    FDataTableRowHandle EmptyHandle;
    
    if (!UpgradeDataTable)
    {
        return EmptyHandle;
    }
    
    TArray<FName> RowNames = UpgradeDataTable->GetRowNames();
    for (const FName& RowName : RowNames)
    {
        FUpgradeTableRow* Row = UpgradeDataTable->FindRow<FUpgradeTableRow>(RowName, TEXT("FindUpgradeReferenceByName"));
        if (Row && Row->UpgradeName.ToString() == UpgradeName)
        {
            FDataTableRowHandle Handle;
            Handle.DataTable = UpgradeDataTable;
            Handle.RowName = RowName;
            return Handle;
        }
    }
    
    return EmptyHandle;
}

// === DEBUG FUNCTIONS ===
void UDataTableManager::PrintAllResourceData()
{
    TArray<FResourceTableRow> AllResources = GetAllResources();
    
    UE_LOG(LogTemp, Log, TEXT("=== ALL RESOURCE DATA (Reference System) ==="));
    for (const FResourceTableRow& Resource : AllResources)
    {
        UE_LOG(LogTemp, Log, TEXT("Name: %s, Type: %s"), 
            *Resource.ResourceName.ToString(),
            *UEnum::GetValueAsString(Resource.ResourceType));
    }
}

void UDataTableManager::PrintAllRecipeData()
{
    TArray<FProductionRecipe> AllRecipes = GetAllRecipes();
    
    UE_LOG(LogTemp, Log, TEXT("=== ALL RECIPE DATA (Reference System) ==="));
    for (const FProductionRecipe& Recipe : AllRecipes)
    {
        FString OutputResourceName = GetResourceNameFromReference(Recipe.OutputResourceReference);
        UE_LOG(LogTemp, Log, TEXT("Recipe: %s, Output: %s, Time: %.1f"), 
            *Recipe.RecipeName.ToString(),
            *OutputResourceName,
            Recipe.ProductionTime);
    }
}

void UDataTableManager::PrintAllUpgradeData()
{
    TArray<FUpgradeTableRow> AllUpgrades = GetAllUpgrades();
    
    UE_LOG(LogTemp, Log, TEXT("=== ALL UPGRADE DATA (Reference System) ==="));
    for (const FUpgradeTableRow& Upgrade : AllUpgrades)
    {
        UE_LOG(LogTemp, Log, TEXT("Name: %s, Category: %s, Type: %s, Cost: %.0f, Tech Level: %d"), 
            *Upgrade.UpgradeName.ToString(),
            *UEnum::GetValueAsString(Upgrade.UpgradeCategory),
            *UEnum::GetValueAsString(Upgrade.UpgradeType),
            Upgrade.ResearchCost,
            Upgrade.TechLevel);
    }
}

void UDataTableManager::ValidateDataIntegrity()
{
    UE_LOG(LogTemp, Log, TEXT("DataTableManager: Validating data integrity (Reference System)..."));
    
    bool bValid = true;
    
    if (!ValidateResourceReferences())
    {
        bValid = false;
    }
    
    if (!ValidateProductionRecipes())
    {
        bValid = false;
    }
    
    if (!ValidateUpgradeReferences())
    {
        bValid = false;
    }
    
    if (bValid)
    {
        UE_LOG(LogTemp, Log, TEXT("DataTableManager: Data integrity validation PASSED"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("DataTableManager: Data integrity validation FAILED"));
    }
}

// === PRIVATE HELPER FUNCTIONS ===
FResourceTableRow* UDataTableManager::GetResourceDataInternal(const FDataTableRowHandle& ResourceReference)
{
    if (!ResourceReference.DataTable || ResourceReference.RowName.IsNone())
    {
        return nullptr;
    }
    
    return ResourceReference.GetRow<FResourceTableRow>(TEXT("GetResourceDataInternal"));
}

FProductionRecipe* UDataTableManager::GetProductionRecipeInternal(const FDataTableRowHandle& RecipeReference)
{
    if (!RecipeReference.DataTable || RecipeReference.RowName.IsNone())
    {
        return nullptr;
    }
    
    return RecipeReference.GetRow<FProductionRecipe>(TEXT("GetProductionRecipeInternal"));
}

FUpgradeTableRow* UDataTableManager::GetUpgradeDataInternal(const FDataTableRowHandle& UpgradeReference)
{
    if (!UpgradeReference.DataTable || UpgradeReference.RowName.IsNone())
    {
        return nullptr;
    }
    
    return UpgradeReference.GetRow<FUpgradeTableRow>(TEXT("GetUpgradeDataInternal"));
}

bool UDataTableManager::IsDataTableRowHandleValid(const FDataTableRowHandle& Handle) const
{
    return (Handle.DataTable != nullptr && !Handle.RowName.IsNone());
}

bool UDataTableManager::ValidateResourceReferences()
{
    bool bValid = true;
    TArray<FProductionRecipe> AllRecipes = GetAllRecipes();
    
    for (const FProductionRecipe& Recipe : AllRecipes)
    {
        // Check output resource reference
        if (!IsValidResourceReference(Recipe.OutputResourceReference))
        {
            UE_LOG(LogTemp, Error, TEXT("Recipe '%s' has invalid output resource reference"), 
                *Recipe.RecipeName.ToString());
            bValid = false;
        }
        
        // Check input resource references
        for (const FResourceRequirement& Input : Recipe.InputResources)
        {
            if (!IsValidResourceReference(Input.ResourceReference))
            {
                UE_LOG(LogTemp, Error, TEXT("Recipe '%s' has invalid input resource reference"), 
                    *Recipe.RecipeName.ToString());
                bValid = false;
            }
        }
    }
    
    return bValid;
}

bool UDataTableManager::ValidateProductionRecipes()
{
    bool bValid = true;
    TArray<FProductionRecipe> AllRecipes = GetAllRecipes();
    
    for (const FProductionRecipe& Recipe : AllRecipes)
    {
        if (Recipe.ProductionTime <= 0.0f)
        {
            UE_LOG(LogTemp, Error, TEXT("Recipe '%s' has invalid production time: %.2f"), 
                *Recipe.RecipeName.ToString(), Recipe.ProductionTime);
            bValid = false;
        }
        
        if (Recipe.OutputQuantity <= 0)
        {
            UE_LOG(LogTemp, Error, TEXT("Recipe '%s' has invalid output quantity: %d"), 
                *Recipe.RecipeName.ToString(), Recipe.OutputQuantity);
            bValid = false;
        }
        
        if (Recipe.InputResources.Num() == 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("Recipe '%s' has no input requirements"), 
                *Recipe.RecipeName.ToString());
        }
    }
    
    return bValid;
}

bool UDataTableManager::ValidateUpgradeReferences()
{
    bool bValid = true;
    TArray<FProductionRecipe> AllRecipes = GetAllRecipes();
    
    UE_LOG(LogTemp, Log, TEXT("DataTableManager: Validating upgrade references..."));
    
    for (const FProductionRecipe& Recipe : AllRecipes)
    {
        // Check upgrade references in recipes
        for (const FDataTableRowHandle& UpgradeRef : Recipe.RequiredUpgrades)
        {
            if (!IsValidUpgradeReference(UpgradeRef))
            {
                UE_LOG(LogTemp, Error, TEXT("Recipe '%s' has invalid upgrade reference"), 
                    *Recipe.RecipeName.ToString());
                bValid = false;
            }
        }
    }
    
    // Validate upgrade prerequisites
    TArray<FUpgradeTableRow> AllUpgrades = GetAllUpgrades();
    for (const FUpgradeTableRow& Upgrade : AllUpgrades)
    {
        for (const FUpgradeRequirement& Prerequisite : Upgrade.Prerequisites)
        {
            if (!IsValidUpgradeReference(Prerequisite.RequiredUpgradeReference))
            {
                UE_LOG(LogTemp, Error, TEXT("Upgrade '%s' has invalid prerequisite reference"), 
                    *Upgrade.UpgradeName.ToString());
                bValid = false;
            }
        }
    }
    
    return bValid;
}

void UDataTableManager::LogDataTableStats()
{
    UE_LOG(LogTemp, Log, TEXT("=== DATA TABLE STATISTICS (Reference System) ==="));
    UE_LOG(LogTemp, Log, TEXT("Resources: %d"), GetAllResources().Num());
    UE_LOG(LogTemp, Log, TEXT("Recipes: %d"), GetAllRecipes().Num());
    UE_LOG(LogTemp, Log, TEXT("Transport Routes: %d"), GetAllRoutes().Num());
    UE_LOG(LogTemp, Log, TEXT("Upgrades: %d"), GetAllUpgrades().Num());
    UE_LOG(LogTemp, Log, TEXT("Factory Definitions: %d"), FactoryDefinitions.Num());
    UE_LOG(LogTemp, Log, TEXT("Hub Definitions: %d"), HubDefinitions.Num());
    UE_LOG(LogTemp, Log, TEXT("Vehicle Definitions: %d"), VehicleDefinitions.Num());
    UE_LOG(LogTemp, Log, TEXT("Road Definitions: %d"), RoadDefinitions.Num());
    UE_LOG(LogTemp, Log, TEXT("Deposit Definitions: %d"), DepositDefinitions.Num());
    UE_LOG(LogTemp, Log, TEXT("Demand Definitions: %d"), DemandDefinitions.Num());
}