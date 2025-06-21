// UpgradeData.h
// Lokalizacja: Source/FactoryNet/Public/Data/UpgradeData.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "UpgradeData.generated.h"

UENUM(BlueprintType)
enum class EUpgradeCategory : uint8
{
    None            UMETA(DisplayName = "None"),
    Technology      UMETA(DisplayName = "Technology"),
    Equipment       UMETA(DisplayName = "Equipment"),
    Process         UMETA(DisplayName = "Process"),
    Automation      UMETA(DisplayName = "Automation"),
    Efficiency      UMETA(DisplayName = "Efficiency"),
    Quality         UMETA(DisplayName = "Quality"),
    Safety          UMETA(DisplayName = "Safety"),
    Environmental   UMETA(DisplayName = "Environmental")
};

UENUM(BlueprintType)
enum class EUpgradeType : uint8
{
    None                UMETA(DisplayName = "None"),
    ProductionSpeed     UMETA(DisplayName = "Production Speed"),
    EnergyEfficiency    UMETA(DisplayName = "Energy Efficiency"),
    QualityImprovement  UMETA(DisplayName = "Quality Improvement"),
    NewRecipe           UMETA(DisplayName = "New Recipe"),
    StorageIncrease     UMETA(DisplayName = "Storage Increase"),
    AutomationLevel     UMETA(DisplayName = "Automation Level"),
    SafetyProtocol      UMETA(DisplayName = "Safety Protocol"),
    WasteReduction      UMETA(DisplayName = "Waste Reduction")
};

USTRUCT(BlueprintType)
struct FACTORYNET_API FUpgradeRequirement
{
    GENERATED_BODY()

    FUpgradeRequirement()
        : IsOptional(false)
    {
        RequiredUpgradeReference = FDataTableRowHandle();
    }

    // Reference to required upgrade instead of ID
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirement", meta = (RowType = "UpgradeTableRow"))
    FDataTableRowHandle RequiredUpgradeReference;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirement")
    bool IsOptional;
};

USTRUCT(BlueprintType)
struct FACTORYNET_API FUpgradeEffect
{
    GENERATED_BODY()

    FUpgradeEffect()
        : EffectValue(0.0f)
        , IsPercentage(false)
        , IsAdditive(true)
    {
        EffectName = FText::FromString("Default Effect");
    }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    FText EffectName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    float EffectValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    bool IsPercentage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    bool IsAdditive;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    FText EffectDescription;
};

USTRUCT(BlueprintType)
struct FACTORYNET_API FUpgradeTableRow : public FTableRowBase
{
    GENERATED_BODY()

    FUpgradeTableRow()
        : UpgradeCategory(EUpgradeCategory::Technology)
        , UpgradeType(EUpgradeType::ProductionSpeed)
        , ResearchCost(1000.0f)
        , ResearchTime(60.0f)
        , TechLevel(1)
        , IsRepeatable(false)
        , MaxRepeatCount(1)
        , UnlockLevel(1)
    {
        UpgradeName = FText::FromString("Default Upgrade");
    }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Info")
    FText UpgradeName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Info")
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Classification")
    EUpgradeCategory UpgradeCategory;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Classification")
    EUpgradeType UpgradeType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    TSoftObjectPtr<UTexture2D> Icon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cost")
    float ResearchCost;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Research")
    float ResearchTime;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Research")
    int32 TechLevel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
    TArray<FUpgradeRequirement> Prerequisites;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
    TArray<FUpgradeEffect> Effects;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repeatability")
    bool IsRepeatable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repeatability")
    int32 MaxRepeatCount;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unlock")
    int32 UnlockLevel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unlock")
    TArray<int32> UnlocksBuildings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unlock")
    TArray<FDataTableRowHandle> UnlocksRecipes;
};