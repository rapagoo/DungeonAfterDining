// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
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
    FName ResultItem;

    // Add other recipe properties here if needed (e.g., CookingTime)

    FCookingRecipeStruct()
    {
        // Default constructor if needed
    }
}; 