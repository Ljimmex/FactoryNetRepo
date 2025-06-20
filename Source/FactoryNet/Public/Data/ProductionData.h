// ProductionData.h
// Lokalizacja: Source/FactoryNet/Public/Data/ProductionData.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Data/ResourceData.h"
#include "ProductionData.generated.h"

USTRUCT(BlueprintType)
struct FACTORYNET_API FResourceRequirement
{
    GENERATED_BODY()

    FResourceRequirement()
    {
        Quantity = 1;
    }

    // Reference to resource in DataTable instead of ID
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource", meta = (RowType = "ResourceTableRow"))
    FDataTableRowHandle ResourceReference;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
    int32 Quantity;
};

USTRUCT(BlueprintType)
struct FACTORYNET_API FProductionRecipe : public FTableRowBase
{
    GENERATED_BODY()

    FProductionRecipe()
    {
        RecipeName = FText::FromString("Default Recipe");
        ProductionTime = 10.0f;
        EnergyRequired = 0.0f;
        OutputQuantity = 1;
        FactoryLevel = 1;
    }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Info")
    FText RecipeName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Info")
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    TArray<FResourceRequirement> InputResources;

    // Reference to output resource instead of ID
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output", meta = (RowType = "ResourceTableRow"))
    FDataTableRowHandle OutputResourceReference;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output")
    int32 OutputQuantity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Production")
    float ProductionTime;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Production")
    float EnergyRequired;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
    int32 FactoryLevel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
    TArray<int32> RequiredUpgrades;
};