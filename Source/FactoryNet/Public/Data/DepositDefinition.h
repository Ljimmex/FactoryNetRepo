// DepositDefinition.h
// Lokalizacja: Source/FactoryNet/Public/Data/DepositDefinition.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/DataTable.h"
#include "DepositDefinition.generated.h"

USTRUCT(BlueprintType)
struct FACTORYNET_API FDepositLevel
{
    GENERATED_BODY()

    FDepositLevel()
    {
        Level = 1;
        ExtractionRate = 1.0f;
        MaxStorage = 100;
        EnergyConsumption = 5.0f;
        UpgradeCost = 500.0f;
    }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level")
    int32 Level;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction")
    float ExtractionRate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Storage")
    int32 MaxStorage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Energy")
    float EnergyConsumption;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
    float UpgradeCost;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    TSoftObjectPtr<UStaticMesh> LevelMesh;
};

UCLASS(BlueprintType)
class FACTORYNET_API UDepositDefinition : public UDataAsset
{
    GENERATED_BODY()

public:
    UDepositDefinition()
    {
        DepositName = FText::FromString("Iron Ore Deposit");
        MaxLevel = 5;
        IsRenewable = false;
        BaseExtractionRate = 1.0f;
        TotalReserves = 10000;
        RegenerationRate = 0.0f;
        BuildCost = 1000.0f;
        RequiresHub = true;
    }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Info")
    FText DepositName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Info")
    FText Description;

    // Reference to resource instead of ID
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource", meta = (RowType = "ResourceTableRow"))
    FDataTableRowHandle ResourceReference;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    TSoftObjectPtr<UStaticMesh> BaseMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    TSoftObjectPtr<UTexture2D> Icon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Levels")
    TArray<FDepositLevel> DepositLevels;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Levels")
    int32 MaxLevel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction")
    bool IsRenewable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction")
    float BaseExtractionRate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction")
    int32 TotalReserves;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction")
    float RegenerationRate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cost")
    float BuildCost;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
    bool RequiresHub;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
    TArray<int32> RequiredTechnologies;
};