// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h" // For FDataTableRowHandle, though we'll use FName for ItemID
#include "Inventory/InvenItemEnum.h" // Include the header where EInventoryItemType is defined
// Include the header where E_InvenItemTypes (or its C++ equivalent like EInventoryItemType) is defined
// #include "Inventory/InventoryEnumTypes.h" // Example path
#include "SlotStruct.generated.h"

// Removed the placeholder enum definition, as it's now in InvenItemEnum.h

USTRUCT(BlueprintType) // Allow this struct to be used in Blueprints
struct FSlotStruct
{
	GENERATED_BODY()

public:
	// Corresponds to 'ItemID' (Data Table Row Handle in BP)
	// Using FDataTableRowHandle provides the editor UI with DataTable and Row Name selection.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
	FDataTableRowHandle ItemID;

	// Corresponds to 'Quantity' (Integer)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
	int32 Quantity;

	// Corresponds to 'ItemType' (E Inven Item Types)
	// Using the EInventoryItemType defined in InvenItemEnum.h
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
	EInventoryItemType ItemType;

	// Default constructor
	FSlotStruct()
		: Quantity(0)
		// FDataTableRowHandle default constructor handles initialization
		, ItemType(EInventoryItemType::EIT_Eatables) // Default to Eatables
	{
	}
};
