// RoadDefinition.h
// Lokalizacja: Source/FactoryNet/Public/Data/RoadDefinition.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/Material.h"
#include "Engine/DataTable.h"
#include "Data/TransportData.h"
#include "RoadDefinition.generated.h"

UCLASS(BlueprintType)
class FACTORYNET_API URoadDefinition : public UDataAsset
{
    GENERATED_BODY()

public:
    URoadDefinition()
    {
        RoadName = FText::FromString("Dirt Road");
        TransportType = ETransportType::Road;
        MaxSpeed = 30.0f;
        SpeedMultiplier = 1.0f;
        BuildCostPerUnit = 10.0f;
        MaintenanceCostPerUnit = 0.1f;
        Durability = 100.0f;
        WeatherResistance = 0.5f;
    }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Info")
    FText RoadName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Info")
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transport")
    ETransportType TransportType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    TSoftObjectPtr<UStaticMesh> RoadMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    TSoftObjectPtr<UMaterial> RoadMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    TSoftObjectPtr<UTexture2D> Icon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    float MaxSpeed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    float SpeedMultiplier;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cost")
    float BuildCostPerUnit;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cost")
    float MaintenanceCostPerUnit;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Durability")
    float Durability;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Durability")
    float WeatherResistance;

    // ✅ ZMIENIONE: Używamy teraz referencji do upgradów zamiast surowych ID
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements", 
              meta = (RowType = "UpgradeTableRow"))
    TArray<FDataTableRowHandle> RequiredTechnologies;
};