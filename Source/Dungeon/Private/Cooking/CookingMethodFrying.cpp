#include "Cooking/CookingMethodFrying.h"
#include "Engine/DataTable.h"
#include "Inventory/CookingRecipeStruct.h" // For FCookingRecipeStruct
#include "Particles/ParticleSystem.h"      // For UParticleSystem
#include "Sound/SoundBase.h"             // For USoundBase
#include "GameplayTagsManager.h"         // For UGameplayTagsManager

UCookingMethodFrying::UCookingMethodFrying()
{
	// Constructor for Frying method
	CookingMethodTag = UGameplayTagsManager::Get().RequestGameplayTag(FName("Cooking.Method.Fry"));
	OilTemperature = 180.0f;
	FryingOilSplatterEffect = nullptr;
	FryingSizzleSound = nullptr;
}

bool UCookingMethodFrying::ProcessIngredients_Implementation(const TArray<FName>& Ingredients, UDataTable* RecipeDataTable, UDataTable* ItemDataTable, FName& OutCookedItemID, float& OutCookingDuration)
{
	UE_LOG(LogTemp, Log, TEXT("Processing ingredients with Frying method (Tag: %s)."), *CookingMethodTag.ToString());

	if (!RecipeDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("RecipeDataTable is null in CookingMethodFrying."));
		OutCookedItemID = NAME_None;
		OutCookingDuration = 0.0f;
		return false;
	}

	if (!CookingMethodTag.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("CookingMethodFrying: CookingMethodTag is not valid!"));
        OutCookedItemID = NAME_None;
        OutCookingDuration = 0.0f;
        return false;
    }

	// TODO: Implement Frying-specific recipe logic.
	// For example, recipes for frying might require a specific tag, or an "Oil" ingredient.
	// Or, the same ingredients might produce a different result when fried compared to grilled.

	// For now, let's use a simplified logic similar to Grilling, but assume it looks for recipes
	// perhaps ending with "_Fried" in their ResultingItemID, or you could add a boolean like 'bIsFriedRecipe' to FCookingRecipeStruct.
	for (auto It = RecipeDataTable->GetRowMap().CreateConstIterator(); It; ++It)
	{
		FName RowName = It.Key();
		FCookingRecipeStruct* Recipe = reinterpret_cast<FCookingRecipeStruct*>(It.Value());

		// Check if the recipe allows this cooking method (via Gameplay Tag) and ingredients match
		if (Recipe && Recipe->AllowedCookingMethods.HasTag(CookingMethodTag) && Recipe->RequiredIngredients.Num() == Ingredients.Num())
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
				OutCookingDuration = Recipe->CookingDuration > 0 ? Recipe->CookingDuration * 0.8f : 4.0f; 
				
				UE_LOG(LogTemp, Log, TEXT("Frying Recipe Found: %s, Output: %s, Duration: %f"), *RowName.ToString(), *OutCookedItemID.ToString(), OutCookingDuration);
				return true;
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("No suitable frying recipe (Tag: %s) found for the given ingredients."), *CookingMethodTag.ToString());
	OutCookedItemID = NAME_None;
	OutCookingDuration = 0.0f;
	return false; 
}

FText UCookingMethodFrying::GetCookingMethodName_Implementation() const
{
	// You can localize this text later if needed
	return FText::FromString(TEXT("튀기기")); // "Frying" in Korean
} 