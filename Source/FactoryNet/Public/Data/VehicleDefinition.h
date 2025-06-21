// VehicleDefinition.h
// Lokalizacja: Source/FactoryNet/Public/Data/VehicleDefinition.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/DataTable.h"
#include "Data/TransportData.h"
#include "VehicleDefinition.generated.h"

// Forward declaration
class URoadDefinition;

UCLASS(BlueprintType)
class FACTORYNET_API UVehicleDefinition : public UDataAsset
{
    GENERATED_BODY()

public:
    UVehicleDefinition()
    {
        VehicleName = FText::FromString("Basic Truck");
        TransportType = ETransportType::Road;
        MaxSpeed = 50.0f;
        CargoCapacity = 20;
        FuelConsumption = 1.0f;
        LoadingTime = 5.0f;
        UnloadingTime = 5.0f;
        PurchaseCost = 2000.0f;
        MaintenanceCost = 10.0f;
    }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Info")
    FText VehicleName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Info")
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transport")
    ETransportType TransportType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    TSoftObjectPtr<UStaticMesh> VehicleMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    TSoftObjectPtr<UTexture2D> Icon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    float MaxSpeed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cargo")
    int32 CargoCapacity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    float FuelConsumption;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Operations")
    float LoadingTime;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Operations")
    float UnloadingTime;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cost")
    float PurchaseCost;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cost")
    float MaintenanceCost;

    // References to supported road types instead of IDs
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements", meta = (AllowedClasses = "RoadDefinition"))
    TArray<TSoftObjectPtr<URoadDefinition>> SupportedRoadTypes;

    // ✅ DODANE: Wymagane technologie dla pojazdu
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements", 
              meta = (RowType = "UpgradeTableRow"))
    TArray<FDataTableRowHandle> RequiredTechnologies;
};