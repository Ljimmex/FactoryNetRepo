// ResourceData.h
// Lokalizacja: Source/FactoryNet/Public/Data/ResourceData.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "ResourceData.generated.h"

UENUM(BlueprintType)
enum class EResourceType : uint8
{
	None            UMETA(DisplayName = "None"),
	RawMaterial     UMETA(DisplayName = "Raw Material"),
	Intermediate    UMETA(DisplayName = "Intermediate Product"),
	FinalProduct    UMETA(DisplayName = "Final Product"),
	Energy          UMETA(DisplayName = "Energy")
};

USTRUCT(BlueprintType)
struct FACTORYNET_API FResourceTableRow : public FTableRowBase
{
	GENERATED_BODY()

	FResourceTableRow()
	{
		ResourceName = FText::FromString("Default Resource");
		ResourceType = EResourceType::RawMaterial;
		ResourceColor = FLinearColor::White;
		MaxStack = 100;
		BaseValue = 1.0f;
		Weight = 1.0f;
		IsRenewable = false;
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Info")
	FText ResourceName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Info")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic Info")
	EResourceType ResourceType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
	FLinearColor ResourceColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties")
	int32 MaxStack;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties")
	float BaseValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties")
	float Weight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties")
	bool IsRenewable;
};