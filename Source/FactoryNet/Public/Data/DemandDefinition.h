// DemandDefinition.h
// Lokalizacja: Source/FactoryNet/Public/Data/DemandDefinition.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/DataTable.h"
#include "DemandDefinition.generated.h"

USTRUCT(BlueprintType)
struct FACTORYNET_API FResourceDemand
{
    GENERATED_BODY()

    FResourceDemand()
    {
        BaseQuantity = 10;
        PricePerUnit = 100.0f;
        Priority = 1.0f;
        SeasonalMultiplier = 1.0f;
    }

    // Reference to demanded resource instead of ID
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demand", meta = (RowType = "ResourceTableRow"))
    FDataTableRowHandle ResourceReference;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demand")
    int32 BaseQuantity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economy")
    float PricePerUnit;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demand")
    float Priority;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demand")
    float SeasonalMultiplier;
};

USTRUCT(BlueprintType)
struct FACTORYNET_API FDemandLevel
{
    GENERATED_BODY()

    FDemandLevel()
    {
        Level = 1;
        DemandMultiplier = 1.0f;
        MaxStorage = 500;
        ProcessingSpeed = 1.0f;
        UpgradeCost = 1000.0f;
    }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level")
    int32 Level;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demand")
    float DemandMultiplier;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Storage")
    int32 MaxStorage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Processing")
    float ProcessingSpeed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
    float UpgradeCost;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    TSoftObjectPtr<UStaticMesh> LevelMesh;
};

UCLASS(BlueprintType)
class FACTORYNET_API UDemandDefinition : public UDataAsset
{
    GENERATED_BODY()

public:
    UDemandDefinition()
    {
        DemandPointName = FText::FromString("City");
        MaxLevel = 3;
        BuildCost = 5000.0f;
        BaseProcessingSpeed = 1.0f;
        RequiresHub = true;
        DemandCycleTime = 60.0f;
        BasePaymentMultiplier = 1.0f;
        LateDeliveryPenalty = 0.5f;
    }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Info")
    FText DemandPointName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Info")
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    TSoftObjectPtr<UStaticMesh> BaseMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    TSoftObjectPtr<UTexture2D> Icon;

    // Array of demanded resources with references
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demand")
    TArray<FResourceDemand> ResourceDemands;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Levels")
    TArray<FDemandLevel> DemandLevels;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Levels")
    int32 MaxLevel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cost")
    float BuildCost;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Processing")
    float BaseProcessingSpeed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
    bool RequiresHub;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demand")
    float DemandCycleTime; // How often demand is generated (in seconds)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economy")
    float BasePaymentMultiplier;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economy")
    float LateDeliveryPenalty;

    // ✅ ZMIENIONE: Używamy teraz referencji do upgradów zamiast surowych ID
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements", 
              meta = (RowType = "UpgradeTableRow"))
    TArray<FDataTableRowHandle> RequiredTechnologies;
};