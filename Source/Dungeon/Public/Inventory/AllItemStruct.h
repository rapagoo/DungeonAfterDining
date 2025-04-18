// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/SlotStruct.h"
#include "AllItemStruct.generated.h"

USTRUCT(BlueprintType)
struct FAllItemStruct
{
	GENERATED_BODY()

public:
	// Represents the 'Eatables' slot
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Slots")
	FSlotStruct Eatables;

	// Add Tooltip and SaveGame meta if needed based on the image
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Slots", meta=(Tooltip="Tooltip for Eatables", SaveGame))
	// FSlotStruct Eatables;

	// Default constructor
	FAllItemStruct()
	{
		// FSlotStruct has its own default constructor
	}
};
