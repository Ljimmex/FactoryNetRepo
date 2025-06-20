// TransportData.h
// Lokalizacja: Source/FactoryNet/Public/Data/TransportData.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "TransportData.generated.h"

// Forward declarations
class UHubDefinition;
class UVehicleDefinition;
class URoadDefinition;

UENUM(BlueprintType)
enum class ETransportType : uint8
{
    None        UMETA(DisplayName = "None"),
    Road        UMETA(DisplayName = "Road"),
    Rail        UMETA(DisplayName = "Rail"),
    Water       UMETA(DisplayName = "Water"),
    Air         UMETA(DisplayName = "Air"),
    Pipeline    UMETA(DisplayName = "Pipeline")
};

USTRUCT(BlueprintType)
struct FACTORYNET_API FTransportRoute : public FTableRowBase
{
    GENERATED_BODY()

    FTransportRoute()
    {
        TransportType = ETransportType::Road;
        Distance = 0.0f;
        MaxThroughput = 100;
        CurrentLoad = 0;
        IsActive = true;
    }

    // Reference to start hub instead of ID
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Route", meta = (AllowedClasses = "HubDefinition"))
    TSoftObjectPtr<UHubDefinition> StartHubReference;

    // Reference to end hub instead of ID
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Route", meta = (AllowedClasses = "HubDefinition"))
    TSoftObjectPtr<UHubDefinition> EndHubReference;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Route")
    ETransportType TransportType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Route")
    float Distance;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capacity")
    int32 MaxThroughput;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capacity")
    int32 CurrentLoad;

    // Reference to vehicle instead of ID
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle", meta = (AllowedClasses = "VehicleDefinition"))
    TSoftObjectPtr<UVehicleDefinition> VehicleReference;

    // Reference to road type instead of ID
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Infrastructure", meta = (AllowedClasses = "RoadDefinition"))
    TSoftObjectPtr<URoadDefinition> RoadReference;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
    bool IsActive;

    // Cargo manifest with resource references
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cargo")
    TMap<FDataTableRowHandle, int32> CargoManifest;
};