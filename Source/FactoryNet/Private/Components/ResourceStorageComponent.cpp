// ResourceStorageComponent.cpp
// Lokalizacja: Source/FactoryNet/Private/Components/ResourceStorageComponent.cpp

#include "Components/ResourceStorageComponent.h"
#include "Core/DataTableManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

UResourceStorageComponent::UResourceStorageComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    MaxCapacity = 100;
    bSingleResourceMode = true;
    bAllowOverflow = false;
}

void UResourceStorageComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // Initialize with default resource if set
    if (bSingleResourceMode && IsValidResourceReference(StoredResourceType))
    {
        // Ensure we have an entry for the stored resource type
        if (FindResourceIndex(StoredResourceType) == INDEX_NONE)
        {
            FStoredResource NewResource;
            NewResource.ResourceReference = StoredResourceType;
            NewResource.Quantity = 0;
            StoredResources.Add(NewResource);
        }
    }
}

bool UResourceStorageComponent::AddResource(const FDataTableRowHandle& ResourceType, int32 Amount)
{
    if (Amount <= 0 || !IsValidResourceReference(ResourceType))
    {
        return false;
    }

    if (!CanAcceptResourceType(ResourceType))
    {
        UE_LOG(LogTemp, Warning, TEXT("ResourceStorageComponent: Cannot accept resource type %s"), 
               *ResourceType.RowName.ToString());
        return false;
    }

    int32 ResourceIndex = FindResourceIndex(ResourceType);
    int32 OldAmount = 0;
    
    if (ResourceIndex == INDEX_NONE)
    {
        // Create new resource entry
        FStoredResource NewResource;
        NewResource.ResourceReference = ResourceType;
        NewResource.Quantity = 0;
        StoredResources.Add(NewResource);
        ResourceIndex = StoredResources.Num() - 1;
    }
    else
    {
        OldAmount = StoredResources[ResourceIndex].Quantity;
    }

    // Check capacity constraints
    int32 CurrentTotal = GetTotalStoredResources();
    int32 AvailableSpace = MaxCapacity - CurrentTotal;
    
    if (!bAllowOverflow && Amount > AvailableSpace)
    {
        Amount = AvailableSpace;
    }

    if (Amount <= 0)
    {
        return false;
    }

    // Add the resource
    StoredResources[ResourceIndex].Quantity += Amount;
    int32 NewAmount = StoredResources[ResourceIndex].Quantity;

    // Broadcast events
    BroadcastStorageEvents(ResourceType, OldAmount, NewAmount, true);

    UE_LOG(LogTemp, VeryVerbose, TEXT("ResourceStorageComponent: Added %d of %s (Total: %d/%d)"), 
           Amount, *ResourceType.RowName.ToString(), NewAmount, MaxCapacity);

    return true;
}

int32 UResourceStorageComponent::RemoveResource(const FDataTableRowHandle& ResourceType, int32 Amount)
{
    if (Amount <= 0 || !IsValidResourceReference(ResourceType))
    {
        return 0;
    }

    int32 ResourceIndex = FindResourceIndex(ResourceType);
    if (ResourceIndex == INDEX_NONE)
    {
        return 0;
    }

    int32 OldAmount = StoredResources[ResourceIndex].Quantity;
    int32 ActualRemoved = FMath::Min(Amount, OldAmount);
    
    if (ActualRemoved <= 0)
    {
        return 0;
    }

    StoredResources[ResourceIndex].Quantity -= ActualRemoved;
    int32 NewAmount = StoredResources[ResourceIndex].Quantity;

    // Remove entry if empty and not in single resource mode
    if (NewAmount <= 0 && !bSingleResourceMode)
    {
        StoredResources.RemoveAt(ResourceIndex);
        NewAmount = 0;
    }

    // Broadcast events
    BroadcastStorageEvents(ResourceType, OldAmount, NewAmount, false);

    UE_LOG(LogTemp, VeryVerbose, TEXT("ResourceStorageComponent: Removed %d of %s (Remaining: %d)"), 
           ActualRemoved, *ResourceType.RowName.ToString(), NewAmount);

    return ActualRemoved;
}
// ResourceStorageComponent.cpp (Część 2)
// Kontynuacja implementacji

bool UResourceStorageComponent::CanStoreResource(const FDataTableRowHandle& ResourceType, int32 Amount) const
{
    if (Amount <= 0 || !IsValidResourceReference(ResourceType))
    {
        return false;
    }

    if (!CanAcceptResourceType(ResourceType))
    {
        return false;
    }

    if (bAllowOverflow)
    {
        return true;
    }

    int32 CurrentTotal = GetTotalStoredResources();
    return (CurrentTotal + Amount) <= MaxCapacity;
}

bool UResourceStorageComponent::HasResource(const FDataTableRowHandle& ResourceType, int32 Amount) const
{
    return GetCurrentAmount(ResourceType) >= Amount;
}

int32 UResourceStorageComponent::GetCurrentAmount(const FDataTableRowHandle& ResourceType) const
{
    int32 ResourceIndex = FindResourceIndex(ResourceType);
    if (ResourceIndex != INDEX_NONE)
    {
        return StoredResources[ResourceIndex].Quantity;
    }
    return 0;
}

int32 UResourceStorageComponent::GetAvailableSpace(const FDataTableRowHandle& ResourceType) const
{
    if (!CanAcceptResourceType(ResourceType))
    {
        return 0;
    }

    if (bAllowOverflow)
    {
        return INT32_MAX;
    }

    int32 CurrentTotal = GetTotalStoredResources();
    return FMath::Max(0, MaxCapacity - CurrentTotal);
}

int32 UResourceStorageComponent::GetTotalStoredResources() const
{
    int32 Total = 0;
    for (const FStoredResource& Resource : StoredResources)
    {
        Total += Resource.Quantity;
    }
    return Total;
}

TArray<FStoredResource> UResourceStorageComponent::GetAllStoredResources() const
{
    return StoredResources;
}

bool UResourceStorageComponent::IsEmpty() const
{
    return GetTotalStoredResources() == 0;
}

bool UResourceStorageComponent::IsFull() const
{
    if (bAllowOverflow)
    {
        return false;
    }
    return GetTotalStoredResources() >= MaxCapacity;
}

void UResourceStorageComponent::SetMaxCapacity(int32 NewMaxCapacity)
{
    MaxCapacity = FMath::Max(0, NewMaxCapacity);
    
    UE_LOG(LogTemp, Log, TEXT("ResourceStorageComponent: Set max capacity to %d"), MaxCapacity);
}

void UResourceStorageComponent::SetResourceType(const FDataTableRowHandle& NewResourceType)
{
    if (!bSingleResourceMode)
    {
        UE_LOG(LogTemp, Warning, TEXT("ResourceStorageComponent: Cannot set resource type in multi-resource mode"));
        return;
    }

    StoredResourceType = NewResourceType;
    
    // Clear existing resources and add new type
    StoredResources.Empty();
    
    if (IsValidResourceReference(NewResourceType))
    {
        FStoredResource NewResource;
        NewResource.ResourceReference = NewResourceType;
        NewResource.Quantity = 0;
        StoredResources.Add(NewResource);
        
        UE_LOG(LogTemp, Log, TEXT("ResourceStorageComponent: Set resource type to %s"), 
               *NewResourceType.RowName.ToString());
    }
}

void UResourceStorageComponent::SetSingleResourceMode(bool bSingleResource)
{
    if (bSingleResourceMode == bSingleResource)
    {
        return;
    }

    bSingleResourceMode = bSingleResource;
    
    if (bSingleResourceMode)
    {
        // Keep only the first resource
        if (StoredResources.Num() > 1)
        {
            FStoredResource FirstResource = StoredResources[0];
            StoredResources.Empty();
            StoredResources.Add(FirstResource);
            StoredResourceType = FirstResource.ResourceReference;
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("ResourceStorageComponent: Set single resource mode to %s"), 
           bSingleResource ? TEXT("true") : TEXT("false"));
}
// ResourceStorageComponent.cpp (Część 3)
// Kontynuacja implementacji - utility functions

void UResourceStorageComponent::ClearAllResources()
{
    TArray<FStoredResource> OldResources = StoredResources;
    
    for (FStoredResource& Resource : StoredResources)
    {
        Resource.Quantity = 0;
    }

    // Remove empty entries in multi-resource mode
    if (!bSingleResourceMode)
    {
        StoredResources.Empty();
    }

    // Broadcast events for cleared resources
    for (const FStoredResource& OldResource : OldResources)
    {
        if (OldResource.Quantity > 0)
        {
            OnStorageChanged.Broadcast(OldResource.ResourceReference, 0, MaxCapacity);
            OnResourceRemoved.Broadcast(OldResource.ResourceReference, OldResource.Quantity);
            OnStorageChanged_BP(OldResource.ResourceReference, 0, MaxCapacity);
            OnResourceRemoved_BP(OldResource.ResourceReference, OldResource.Quantity);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("ResourceStorageComponent: Cleared all resources"));
}

bool UResourceStorageComponent::TransferResourceTo(UResourceStorageComponent* TargetStorage, 
                                                 const FDataTableRowHandle& ResourceType, 
                                                 int32 Amount)
{
    if (!TargetStorage || Amount <= 0 || !IsValidResourceReference(ResourceType))
    {
        return false;
    }

    // Check if we have enough resource
    int32 AvailableAmount = GetCurrentAmount(ResourceType);
    if (AvailableAmount < Amount)
    {
        Amount = AvailableAmount;
    }

    if (Amount <= 0)
    {
        return false;
    }

    // Check if target can accept the resource
    if (!TargetStorage->CanStoreResource(ResourceType, Amount))
    {
        int32 TargetAvailableSpace = TargetStorage->GetAvailableSpace(ResourceType);
        Amount = TargetAvailableSpace;
        
        if (Amount <= 0)
        {
            return false;
        }
    }

    // Perform the transfer
    int32 RemovedAmount = RemoveResource(ResourceType, Amount);
    if (RemovedAmount > 0)
    {
        bool bAdded = TargetStorage->AddResource(ResourceType, RemovedAmount);
        if (!bAdded)
        {
            // Add back if target failed to accept
            AddResource(ResourceType, RemovedAmount);
            return false;
        }
        return true;
    }

    return false;
}

void UResourceStorageComponent::SetInitialResource(const FDataTableRowHandle& ResourceType, int32 Amount)
{
    if (Amount < 0 || !IsValidResourceReference(ResourceType))
    {
        return;
    }

    if (bSingleResourceMode)
    {
        StoredResourceType = ResourceType;
        StoredResources.Empty();
        
        FStoredResource NewResource;
        NewResource.ResourceReference = ResourceType;
        NewResource.Quantity = Amount;
        StoredResources.Add(NewResource);
    }
    else
    {
        int32 ResourceIndex = FindResourceIndex(ResourceType);
        if (ResourceIndex == INDEX_NONE)
        {
            FStoredResource NewResource;
            NewResource.ResourceReference = ResourceType;
            NewResource.Quantity = Amount;
            StoredResources.Add(NewResource);
        }
        else
        {
            StoredResources[ResourceIndex].Quantity = Amount;
        }
    }

    UE_LOG(LogTemp, Log, TEXT("ResourceStorageComponent: Set initial resource %s to %d"), 
           *ResourceType.RowName.ToString(), Amount);
}

// === PRIVATE HELPER FUNCTIONS ===

int32 UResourceStorageComponent::FindResourceIndex(const FDataTableRowHandle& ResourceType) const
{
    for (int32 i = 0; i < StoredResources.Num(); ++i)
    {
        if (StoredResources[i].ResourceReference.RowName == ResourceType.RowName)
        {
            return i;
        }
    }
    return INDEX_NONE;
}

bool UResourceStorageComponent::IsValidResourceReference(const FDataTableRowHandle& ResourceType) const
{
    return !ResourceType.RowName.IsNone() && ResourceType.DataTable != nullptr;
}

bool UResourceStorageComponent::CanAcceptResourceType(const FDataTableRowHandle& ResourceType) const
{
    if (bSingleResourceMode)
    {
        if (!StoredResourceType.RowName.IsNone())
        {
            return StoredResourceType.RowName == ResourceType.RowName;
        }
        return true; // Can accept any type if none is set
    }
    
    return true; // Multi-resource mode accepts all types
}

void UResourceStorageComponent::BroadcastStorageEvents(const FDataTableRowHandle& ResourceType, 
                                                     int32 OldAmount, 
                                                     int32 NewAmount, 
                                                     bool bWasAdded)
{
    // Broadcast C++ events
    OnStorageChanged.Broadcast(ResourceType, NewAmount, MaxCapacity);
    
    if (bWasAdded)
    {
        OnResourceAdded.Broadcast(ResourceType, NewAmount - OldAmount);
    }
    else
    {
        OnResourceRemoved.Broadcast(ResourceType, OldAmount - NewAmount);
    }

    // Broadcast Blueprint events (fixed parameter name conflict)
    OnStorageChanged_BP(ResourceType, NewAmount, MaxCapacity);
    
    if (bWasAdded)
    {
        OnResourceAdded_BP(ResourceType, NewAmount - OldAmount);
    }
    else
    {
        OnResourceRemoved_BP(ResourceType, OldAmount - NewAmount);
    }
}