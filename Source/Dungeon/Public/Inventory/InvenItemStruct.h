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

	// NEW: Multi-stage slicing system
	// Tooltip: SlicedOnceItemID - The Row Name of the item after being sliced once (improperly prepared)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Properties | Cooking")
	FName SlicedOnceItemID; // 1번 썬 재료 (애매하게 손질됨)

	// Tooltip: SlicedTwiceItemID - The Row Name of the item after being sliced twice (improperly prepared)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Properties | Cooking")
	FName SlicedTwiceItemID; // 2번 썬 재료 (애매하게 손질됨)

	// Tooltip: SlicedThreeTimesItemID - The Row Name of the item after being sliced three times (fully prepared)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Properties | Cooking")
	FName SlicedThreeTimesItemID; // 3번 썬 재료 (완전히 손질됨)

	// Tooltip: bCanBeUsedInCooking - Whether this item can be used as an ingredient for cooking
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Properties | Cooking")
	bool bCanBeUsedInCooking = true; // 요리에 사용 가능한지 여부

	// Add a new field to link Recipe items to actual cooking recipes
	// This should be the Row Name of the recipe in the RecipeDataTable (e.g., "PotatoSoup")
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	FName UnlocksRecipeID = NAME_None; // Default to None

	// NEW: Button-based cutting system - Different cutting styles with pre-made meshes
	// Tooltip: DicedMesh - Mesh for diced cutting style (깍둑썰기)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Properties | Cutting Styles")
	TSoftObjectPtr<UStaticMesh> DicedMesh;

	// Tooltip: JulienneMesh - Mesh for julienne cutting style (채썰기)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Properties | Cutting Styles")
	TSoftObjectPtr<UStaticMesh> JulienneMesh;

	// Tooltip: MincedMesh - Mesh for minced cutting style (다지기)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Properties | Cutting Styles")
	TSoftObjectPtr<UStaticMesh> MincedMesh;

	// NEW: Item IDs for cut ingredients - these should reference separate data table entries
	// Tooltip: DicedItemID - The Row Name of the diced version of this ingredient
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Properties | Cutting Styles")
	FName DicedItemID = NAME_None;

	// Tooltip: JulienneItemID - The Row Name of the julienne version of this ingredient
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Properties | Cutting Styles")
	FName JulienneItemID = NAME_None;

	// Tooltip: MincedItemID - The Row Name of the minced version of this ingredient
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Properties | Cutting Styles")
	FName MincedItemID = NAME_None;

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
