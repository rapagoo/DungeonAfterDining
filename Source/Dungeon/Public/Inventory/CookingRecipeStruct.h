// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "CookingRecipeStruct.generated.h"

/**
 * Structure defining a cooking recipe.
 */
USTRUCT(BlueprintType)
struct FCookingRecipeStruct : public FTableRowBase
{
    GENERATED_BODY()

public:
    /** List of required ingredient Item IDs (RowNames from ItemDataTable) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
    TArray<FName> RequiredIngredients;

    /** The resulting Item ID (RowName from ItemDataTable) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
    FName ResultingItemID;

    /** Time in seconds this recipe takes to cook. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recipe")
    float CookingDuration;

    // NEW: Gameplay Tags to specify which cooking methods can use this recipe.
    // Example Tags: "Cooking.Method.Grill", "Cooking.Method.Fry", "Cooking.Method.Boil"
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recipe", meta = (DisplayName = "Allowed Cooking Methods"))
    FGameplayTagContainer AllowedCookingMethods;

    // Optional: A specific cooking method this recipe is primarily designed for (if only one)
    // UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recipe", meta = (DisplayName = "Primary Cooking Method"))
    // FGameplayTag PrimaryCookingMethod;

    FCookingRecipeStruct()
    {
        // Default constructor
        CookingDuration = 5.0f; // Default cooking duration
    }
}; 