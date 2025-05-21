#include "Cooking/CookingMethodBase.h"
#include "Engine/DataTable.h"
#include "Inventory/CookingRecipeStruct.h" // For FCookingRecipeStruct
#include "Inventory/InvenItemStruct.h"     // For FInventoryItemStruct (though not directly used in base, good for context)

UCookingMethodBase::UCookingMethodBase()
{
	// Constructor logic, if any
}

bool UCookingMethodBase::ProcessIngredients_Implementation(const TArray<FName>& Ingredients, UDataTable* RecipeDataTable, UDataTable* ItemDataTable, FName& OutCookedItemID, float& OutCookingDuration)
{
	// Base implementation - This should be overridden by derived classes.
	// For now, let's assume a generic check or return false if not overridden.
	UE_LOG(LogTemp, Warning, TEXT("ProcessIngredients_Implementation called on UCookingMethodBase directly. This should be overridden in a child class."));
	
	OutCookedItemID = NAME_None;
	OutCookingDuration = 0.0f;

	if (!RecipeDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("RecipeDataTable is null in CookingMethodBase."));
		return false;
	}

	// Basic recipe check (similar to InteractablePot's CheckRecipeInternal, but generalized)
    // This is a placeholder. Specific cooking methods might have different ways to validate recipes or might use this as a fallback.
    for (auto It = RecipeDataTable->GetRowMap().CreateConstIterator(); It; ++It)
    {
        FName RowName = It.Key();
        FCookingRecipeStruct* Recipe = reinterpret_cast<FCookingRecipeStruct*>(It.Value());

        if (Recipe && Recipe->RequiredIngredients.Num() == Ingredients.Num())
        {
            TArray<FName> TempIngredients = Ingredients;
            bool bMatch = true;
            for (const FName& RequiredIngredient : Recipe->RequiredIngredients)
            {
                if (TempIngredients.RemoveSingle(RequiredIngredient) == 0)
                {
                    bMatch = false;
                    break;
                }
            }
            if (bMatch)
            {
                OutCookedItemID = Recipe->ResultingItemID;
                OutCookingDuration = Recipe->CookingDuration > 0 ? Recipe->CookingDuration : 5.0f; // Default if not specified
                // UE_LOG(LogTemp, Log, TEXT("Recipe found by base: %s, Duration: %f"), *OutCookedItemID.ToString(), OutCookingDuration);
                return true; // Found a matching recipe
            }
        }
    }

	return false; // No recipe found by the base logic
}

FText UCookingMethodBase::GetCookingMethodName_Implementation() const
{
	// Default name, should be overridden
	return FText::FromString(TEXT("Generic Cooking"));
} 