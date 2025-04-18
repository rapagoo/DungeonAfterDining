// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h" // Include for FTableRowBase
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

	// Default constructor
	FInventoryItemStruct()
		: StackSize(1)
		, Power(0.0f)
	{
		// Initialize default values if needed
	}
};
