#include "Cooking/CookingMethodGrilling.h"
#include "Engine/DataTable.h"
#include "Inventory/CookingRecipeStruct.h" // For FCookingRecipeStruct
#include "GameplayTagsManager.h" // For UGameplayTagsManager

UCookingMethodGrilling::UCookingMethodGrilling()
{
	// Constructor for Grilling method
	// Set the specific tag for this cooking method
	CookingMethodTag = UGameplayTagsManager::Get().RequestGameplayTag(FName("Cooking.Method.Grill"));
}

bool UCookingMethodGrilling::ProcessIngredients_Implementation(const TArray<FName>& Ingredients, UDataTable* RecipeDataTable, UDataTable* ItemDataTable, FName& OutCookedItemID, float& OutCookingDuration)
{
	// For Grilling, we might want to check for recipes specifically tagged for grilling,
	// or modify the outcome or duration based on the fact it's grilling.

	// For this example, let's assume grilling uses a general recipe table but might have a preferred duration multiplier or specific outcomes.
	// We can call the base class implementation for now, or implement a more specific one.

	UE_LOG(LogTemp, Log, TEXT("Processing ingredients with Grilling method (Tag: %s)."), *CookingMethodTag.ToString());

	if (!RecipeDataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("RecipeDataTable is null in CookingMethodGrilling."));
        OutCookedItemID = NAME_None;
        OutCookingDuration = 0.0f;
        return false;
    }

    if (!CookingMethodTag.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("CookingMethodGrilling: CookingMethodTag is not valid!"));
        OutCookedItemID = NAME_None;
        OutCookingDuration = 0.0f;
        return false;
    }

    // Example: Iterate through recipes and find a match.
    // This could be more sophisticated, e.g. looking for a recipe with a specific tag "Grillable"
    // or modifying the result if it's a grill recipe.
    for (auto It = RecipeDataTable->GetRowMap().CreateConstIterator(); It; ++It)
    {
        FName RowName = It.Key();
        FCookingRecipeStruct* Recipe = reinterpret_cast<FCookingRecipeStruct*>(It.Value());

        // Check if the recipe allows this cooking method and ingredients match
        if (Recipe && Recipe->AllowedCookingMethods.HasTag(CookingMethodTag) && Recipe->RequiredIngredients.Num() == Ingredients.Num())
        {
            TArray<FName> TempIngredients = Ingredients;
            bool bMatch = true;
            for (const FName& RequiredIngredient : Recipe->RequiredIngredients)
            {
                // Ensure the ingredient exists and remove one instance
                if (TempIngredients.RemoveSingle(RequiredIngredient) == 0)
                {
                    bMatch = false;
                    break;
                }
            }

            if (bMatch)
            {
                // Found a matching recipe
                OutCookedItemID = Recipe->ResultingItemID; 
                // Example: Grilling might take slightly longer or shorter than default
                OutCookingDuration = Recipe->CookingDuration > 0 ? Recipe->CookingDuration * 1.1f : 6.0f; // Grilling takes 10% longer, or 6s default
                
                UE_LOG(LogTemp, Log, TEXT("Grilling Recipe Found: %s, Output: %s, Duration: %f"), *RowName.ToString(), *OutCookedItemID.ToString(), OutCookingDuration);
                return true;
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("No suitable grilling recipe (Tag: %s) found for the given ingredients."), *CookingMethodTag.ToString());
    OutCookedItemID = NAME_None;
    OutCookingDuration = 0.0f;
	return false; // No suitable grilling recipe found
}

FText UCookingMethodGrilling::GetCookingMethodName_Implementation() const
{
	return FText::FromString(TEXT("Grilling"));
} 