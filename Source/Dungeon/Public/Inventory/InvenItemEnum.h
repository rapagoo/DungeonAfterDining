// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InvenItemEnum.generated.h"

UENUM(BlueprintType)
enum class EInventoryItemType : uint8
{
	EIT_Eatables UMETA(DisplayName = "Eatables"),
	EIT_Sword    UMETA(DisplayName = "Sword"),
	EIT_Shield   UMETA(DisplayName = "Shield"),
	EIT_Food     UMETA(DisplayName = "Food"),
	EIT_Recipe   UMETA(DisplayName = "Recipe"),
	EIT_PreparedIngredient UMETA(DisplayName = "Prepared Ingredient"), // 손질된 재료
	// Add other enum values from your E_InvenItemTypes here
};

// NEW: Cutting style enumeration for button-based cutting system
UENUM(BlueprintType)
enum class ECuttingStyle : uint8
{
	ECS_None        UMETA(DisplayName = "None"),         // 썰지 않음
	ECS_Diced       UMETA(DisplayName = "Diced"),        // 깍둑썰기
	ECS_Julienne    UMETA(DisplayName = "Julienne"),     // 채썰기
	ECS_Minced      UMETA(DisplayName = "Minced")        // 다지기
};
