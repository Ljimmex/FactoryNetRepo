// ResourceStorageComponent.h
// Lokalizacja: Source/FactoryNet/Public/Components/ResourceStorageComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "Data/ResourceData.h"
#include "ResourceStorageComponent.generated.h"

USTRUCT(BlueprintType)
struct FACTORYNET_API FStoredResource
{
    GENERATED_BODY()

    FStoredResource()
    {
        Quantity = 0;
    }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Storage", meta = (RowType = "ResourceTableRow"))
    FDataTableRowHandle ResourceReference;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Storage")
    int32 Quantity;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnStorageChanged, FDataTableRowHandle, ResourceType, int32, NewAmount, int32, MaxCapacity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnResourceAdded, FDataTableRowHandle, ResourceType, int32, Amount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnResourceRemoved, FDataTableRowHandle, ResourceType, int32, Amount);

UCLASS(BlueprintType, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FACTORYNET_API UResourceStorageComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UResourceStorageComponent();

protected:
    virtual void BeginPlay() override;

public:
    // === STORAGE MANAGEMENT ===
    UFUNCTION(BlueprintCallable, Category = "Storage")
    bool AddResource(const FDataTableRowHandle& ResourceType, int32 Amount);

    UFUNCTION(BlueprintCallable, Category = "Storage")
    int32 RemoveResource(const FDataTableRowHandle& ResourceType, int32 Amount);

    UFUNCTION(BlueprintCallable, Category = "Storage")
    bool CanStoreResource(const FDataTableRowHandle& ResourceType, int32 Amount) const;

    UFUNCTION(BlueprintCallable, Category = "Storage")
    bool HasResource(const FDataTableRowHandle& ResourceType, int32 Amount) const;

    // === QUERIES ===
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Storage")
    int32 GetCurrentAmount(const FDataTableRowHandle& ResourceType) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Storage")
    int32 GetAvailableSpace(const FDataTableRowHandle& ResourceType) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Storage")
    int32 GetTotalStoredResources() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Storage")
    TArray<FStoredResource> GetAllStoredResources() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Storage")
    bool IsEmpty() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Storage")
    bool IsFull() const;

    // === CONFIGURATION ===
    UFUNCTION(BlueprintCallable, Category = "Configuration")
    void SetMaxCapacity(int32 NewMaxCapacity);

    UFUNCTION(BlueprintCallable, Category = "Configuration")
    void SetResourceType(const FDataTableRowHandle& NewResourceType);

    UFUNCTION(BlueprintCallable, Category = "Configuration")
    void SetSingleResourceMode(bool bSingleResource);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Configuration")
    int32 GetMaxCapacity() const { return MaxCapacity; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Configuration")
    FDataTableRowHandle GetStoredResourceType() const { return StoredResourceType; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Configuration")
    bool IsSingleResourceMode() const { return bSingleResourceMode; }

    // === UTILITY ===
    UFUNCTION(BlueprintCallable, Category = "Utility")
    void ClearAllResources();

    UFUNCTION(BlueprintCallable, Category = "Utility")
    bool TransferResourceTo(UResourceStorageComponent* TargetStorage, const FDataTableRowHandle& ResourceType, int32 Amount);

    UFUNCTION(BlueprintCallable, Category = "Utility")
    void SetInitialResource(const FDataTableRowHandle& ResourceType, int32 Amount);

    // === EVENTS ===
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnStorageChanged OnStorageChanged;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnResourceAdded OnResourceAdded;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnResourceRemoved OnResourceRemoved;

    // === BLUEPRINT EVENTS ===
    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnStorageChanged_BP(FDataTableRowHandle ResourceType, int32 NewAmount, int32 MaxCapacityParam);

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnResourceAdded_BP(FDataTableRowHandle ResourceType, int32 Amount);

    UFUNCTION(BlueprintImplementableEvent, Category = "Events")
    void OnResourceRemoved_BP(FDataTableRowHandle ResourceType, int32 Amount);

protected:
    // === STORAGE CONFIGURATION ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Storage Configuration")
    int32 MaxCapacity = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Storage Configuration")
    bool bSingleResourceMode = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Storage Configuration", 
              meta = (EditCondition = "bSingleResourceMode", RowType = "ResourceTableRow"))
    FDataTableRowHandle StoredResourceType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Storage Configuration")
    bool bAllowOverflow = false;

    // === RUNTIME DATA ===
    UPROPERTY(BlueprintReadOnly, Category = "Runtime Data")
    TArray<FStoredResource> StoredResources;

private:
    // === INTERNAL FUNCTIONS ===
    int32 FindResourceIndex(const FDataTableRowHandle& ResourceType) const;
    bool IsValidResourceReference(const FDataTableRowHandle& ResourceType) const;
    bool CanAcceptResourceType(const FDataTableRowHandle& ResourceType) const;
    void BroadcastStorageEvents(const FDataTableRowHandle& ResourceType, int32 OldAmount, int32 NewAmount, bool bWasAdded);
};