// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h" // Include for FTableRowBase
#include "Inventory/InvenItemEnum.h" // Include Enum definition
#include "InvenItemStruct.generated.h"

// Forward declarations for asset types
class UTexture2D;
class UStaticMesh;

USTRUCT(BlueprintType) // Make this struct usable in Blueprints
struct FInventoryItemStruct : public FTableRowBase // Inherit from FTableRowBase to use with Data Tables
{
	GENERATED_BODY()

public:
	// Tooltip: Name
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Properties")
	FText Name;

	// Tooltip: Description
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Properties")
	FText Description;

	// Tooltip: ItemType
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Properties")
	EInventoryItemType ItemType;

	// Tooltip: Thumbnail
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Properties")
	TSoftObjectPtr<UTexture2D> Thumbnail; // Use Soft Ptr for assets

	// Tooltip: StackSize
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Properties")
	int32 StackSize;

	// Tooltip: Power
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Properties")
	float Power;

	// Tooltip: Mesh
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Properties")
	TSoftObjectPtr<UStaticMesh> Mesh; // Use Soft Ptr for assets

	// Tooltip: SlicedItemID - The Row Name of the item this becomes after being sliced. None if not sliceable.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Properties | Cooking")
	FName SlicedItemID; // 잘렸을 때 변환될 아이템의 Row Name

	// Add a new field to link Recipe items to actual cooking recipes
	// This should be the Row Name of the recipe in the RecipeDataTable (e.g., "PotatoSoup")
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	FName UnlocksRecipeID = NAME_None; // Default to None

	// Default constructor
	FInventoryItemStruct()
		: ItemType(EInventoryItemType::EIT_Eatables) // Initialize with a valid existing enum value
		, StackSize(1)
		, Power(0.0f)
		, UnlocksRecipeID(NAME_None)
	{
		// Initialize default values if needed
	}
};
