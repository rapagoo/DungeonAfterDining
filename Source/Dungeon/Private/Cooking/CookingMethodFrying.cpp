#include "Cooking/CookingMethodFrying.h"
#include "Engine/DataTable.h"
#include "Inventory/CookingRecipeStruct.h"
#include "Inventory/InvenItemStruct.h"
#include "Particles/ParticleSystem.h"      // For UParticleSystem
#include "Sound/SoundBase.h"             // For USoundBase
#include "GameplayTagsManager.h"         // For UGameplayTagsManager

UCookingMethodFrying::UCookingMethodFrying()
{
	// Constructor for Frying method
	CookingMethodTag = FGameplayTag::RequestGameplayTag(FName("Cooking.Method.Fry"));
	OilTemperature = 180.0f;
	FryingOilSplatterEffect = nullptr;
	FryingSizzleSound = nullptr;
	
	// 기본 설정값
	BaseFryingTime = 15.0f;
	TimePerIngredient = 2.0f;
	MaxFryingTime = 25.0f;
}

bool UCookingMethodFrying::ProcessIngredients_Implementation(const TArray<FName>& Ingredients, UDataTable* RecipeDataTable, UDataTable* ItemDataTable, FName& OutCookedItemID, float& OutCookingDuration)
{
	if (!RecipeDataTable || Ingredients.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("UCookingMethodFrying::ProcessIngredients - Invalid recipe table or no ingredients"));
		return false;
	}

	// --- NEW: Check if all ingredients can be used in cooking ---
	if (ItemDataTable)
	{
		for (const FName& IngredientID : Ingredients)
		{
			if (!IngredientID.IsNone())
			{
				FInventoryItemStruct* ItemData = ItemDataTable->FindRow<FInventoryItemStruct>(IngredientID, TEXT("FryingCheckCookingUsability"));
				if (ItemData && !ItemData->bCanBeUsedInCooking)
				{
					UE_LOG(LogTemp, Warning, TEXT("UCookingMethodFrying: Ingredient '%s' is improperly prepared and cannot be used for frying"), 
						   *IngredientID.ToString());
					return false;  // Frying failed due to improperly prepared ingredient
				}
			}
		}
	}

	// 재료 정렬 (일관된 레시피 검색을 위해)
	TArray<FName> SortedIngredients = Ingredients;
	SortedIngredients.Sort([](const FName& A, const FName& B) 
	{
		return A.ToString() < B.ToString();
	});

	// 레시피 테이블에서 매칭되는 레시피 찾기
	const TArray<FName> RowNames = RecipeDataTable->GetRowNames();
	for (const FName& RowName : RowNames)
	{
		FCookingRecipeStruct* RecipeRow = RecipeDataTable->FindRow<FCookingRecipeStruct>(RowName, TEXT("FryingProcessIngredients"));
		if (!RecipeRow)
		{
			continue;
		}

		// 이 레시피가 튀기기 요리법용인지 확인
		if (!RecipeRow->AllowedCookingMethods.HasTag(CookingMethodTag))
		{
			continue;
		}

		// 재료 수가 맞는지 확인
		if (RecipeRow->RequiredIngredients.Num() != SortedIngredients.Num())
		{
			continue;
		}

		// 재료를 정렬하여 비교
		TArray<FName> SortedRecipeIngredients = RecipeRow->RequiredIngredients;
		SortedRecipeIngredients.Sort([](const FName& A, const FName& B) 
		{
			return A.ToString() < B.ToString();
		});

		// 재료가 정확히 일치하는지 확인
		bool bIngredientsMatch = true;
		for (int32 i = 0; i < SortedIngredients.Num(); i++)
		{
			if (SortedIngredients[i] != SortedRecipeIngredients[i])
			{
				bIngredientsMatch = false;
				break;
			}
		}

		if (bIngredientsMatch)
		{
			// 매칭되는 레시피 발견!
			OutCookedItemID = RecipeRow->ResultingItemID;
			
			// 튀기기 시간 계산 (재료 개수에 따라 조정)
			OutCookingDuration = FMath::Clamp(
				BaseFryingTime + (Ingredients.Num() * TimePerIngredient),
				BaseFryingTime,
				MaxFryingTime
			);

			UE_LOG(LogTemp, Log, TEXT("UCookingMethodFrying::ProcessIngredients - Found recipe '%s' -> '%s', Duration: %.2f seconds"), 
				   *RowName.ToString(), *OutCookedItemID.ToString(), OutCookingDuration);
			
			return true;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("UCookingMethodFrying::ProcessIngredients - No frying recipe found for %d ingredients"), 
		   Ingredients.Num());
	return false;
}

FText UCookingMethodFrying::GetCookingMethodName_Implementation() const
{
	return FText::FromString(TEXT("Frying"));
} 