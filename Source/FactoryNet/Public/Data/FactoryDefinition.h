// FactoryDefinition.h
// Lokalizacja: Source/FactoryNet/Public/Data/FactoryDefinition.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/DataTable.h"
#include "FactoryDefinition.generated.h"

USTRUCT(BlueprintType)
struct FACTORYNET_API FFactoryLevel
{
    GENERATED_BODY()

    FFactoryLevel()
    {
        Level = 1;
        ProductionSpeedMultiplier = 1.0f;
        EnergyConsumptionMultiplier = 1.0f;
        MaxInputStorage = 100;
        MaxOutputStorage = 100;
        UpgradeCost = 1000.0f;
    }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level")
    int32 Level;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Production")
    float ProductionSpeedMultiplier;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Production")
    float EnergyConsumptionMultiplier;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Storage")
    int32 MaxInputStorage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Storage")
    int32 MaxOutputStorage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
    float UpgradeCost;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    TSoftObjectPtr<UStaticMesh> LevelMesh;
};

UCLASS(BlueprintType)
class FACTORYNET_API UFactoryDefinition : public UDataAsset
{
    GENERATED_BODY()

public:
    UFactoryDefinition()
    {
        FactoryName = FText::FromString("Default Factory");
        BuildCost = 1000.0f;
        BaseEnergyConsumption = 10.0f;
        MaxLevel = 5;
        RequiresHub = true;
    }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Info")
    FText FactoryName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Info")
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    TSoftObjectPtr<UStaticMesh> BaseMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    TSoftObjectPtr<UTexture2D> Icon;

    // References to supported recipes instead of IDs
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Production", meta = (RowType = "ProductionRecipe"))
    TArray<FDataTableRowHandle> SupportedRecipes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Levels")
    TArray<FFactoryLevel> FactoryLevels;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Levels")
    int32 MaxLevel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cost")
    float BuildCost;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Energy")
    float BaseEnergyConsumption;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
    bool RequiresHub;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
    TArray<int32> RequiredTechnologies;
};