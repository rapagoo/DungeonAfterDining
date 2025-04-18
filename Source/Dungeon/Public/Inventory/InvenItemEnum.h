// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InvenItemEnum.generated.h"

UENUM(BlueprintType)
enum class EInventoryItemType : uint8
{
	EIT_Sword UMETA(DisplayName = "Sword"),
	EIT_Shield UMETA(DisplayName = "Shield"),
	EIT_Eatables UMETA(DisplayName = "Eatables"),
	// Add other enum values from your E_InvenItemTypes here
};
