// HubDefinition.h
// Lokalizacja: Source/FactoryNet/Public/Data/HubDefinition.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/DataTable.h"
#include "Data/TransportData.h"
#include "HubDefinition.generated.h"

USTRUCT(BlueprintType)
struct FACTORYNET_API FHubLevel
{
    GENERATED_BODY()

    FHubLevel()
    {
        Level = 1;
        MaxConnections = 2;
        ThroughputMultiplier = 1.0f;
        StorageCapacity = 500;
        ProcessingSpeed = 1.0f;
        UpgradeCost = 500.0f;
    }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level")
    int32 Level;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connections")
    int32 MaxConnections;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Processing")
    float ThroughputMultiplier;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Storage")
    int32 StorageCapacity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Processing")
    float ProcessingSpeed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
    float UpgradeCost;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    TSoftObjectPtr<UStaticMesh> LevelMesh;
};

UCLASS(BlueprintType)
class FACTORYNET_API UHubDefinition : public UDataAsset
{
    GENERATED_BODY()

public:
    UHubDefinition()
    {
        HubName = FText::FromString("Basic Hub");
        BuildCost = 500.0f;
        MaxLevel = 3;
        BaseStorageCapacity = 200;
        BaseProcessingSpeed = 1.0f;
    }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Info")
    FText HubName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Info")
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    TSoftObjectPtr<UStaticMesh> BaseMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    TSoftObjectPtr<UTexture2D> Icon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Levels")
    TArray<FHubLevel> HubLevels;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Levels")
    int32 MaxLevel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cost")
    float BuildCost;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
    int32 BaseStorageCapacity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
    float BaseProcessingSpeed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transport")
    TArray<ETransportType> SupportedTransportTypes;

    // ✅ DODANE: Wymagane technologie dla hubów
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements", 
              meta = (RowType = "UpgradeTableRow"))
    TArray<FDataTableRowHandle> RequiredTechnologies;
};