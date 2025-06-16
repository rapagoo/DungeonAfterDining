#include "Cooking/CookingMethodBoiling.h"
#include "Engine/DataTable.h"
#include "Inventory/CookingRecipeStruct.h"
#include "Inventory/InvenItemStruct.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"
#include "GameplayTagsManager.h"

UCookingMethodBoiling::UCookingMethodBoiling()
{
	// 끓이기 요리법 생성자
	CookingMethodTag = FGameplayTag::RequestGameplayTag(FName("Cooking.Method.Boil"));
	BoilingTemperature = 100.0f;
	BoilingBubbleEffect = nullptr;
	BoilingBubbleSound = nullptr;
	
	// 기본 설정값
	BaseBoilingTime = 20.0f;
	TimePerIngredient = 3.0f;
	MaxBoilingTime = 35.0f;
}

bool UCookingMethodBoiling::ProcessIngredients_Implementation(const TArray<FName>& Ingredients, UDataTable* RecipeDataTable, UDataTable* ItemDataTable, FName& OutCookedItemID, float& OutCookingDuration)
{
	if (!RecipeDataTable || Ingredients.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("UCookingMethodBoiling::ProcessIngredients - Invalid recipe table or no ingredients"));
		return false;
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
		FCookingRecipeStruct* RecipeRow = RecipeDataTable->FindRow<FCookingRecipeStruct>(RowName, TEXT("BoilingProcessIngredients"));
		if (!RecipeRow)
		{
			continue;
		}

		// 이 레시피가 끓이기 요리법용인지 확인
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
			
			// 끓이기 시간 계산 (재료 개수에 따라 조정)
			OutCookingDuration = FMath::Clamp(
				BaseBoilingTime + (Ingredients.Num() * TimePerIngredient),
				BaseBoilingTime,
				MaxBoilingTime
			);

			UE_LOG(LogTemp, Log, TEXT("UCookingMethodBoiling::ProcessIngredients - Found recipe '%s' -> '%s', Duration: %.2f seconds"), 
				   *RowName.ToString(), *OutCookedItemID.ToString(), OutCookingDuration);
			
			return true;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("UCookingMethodBoiling::ProcessIngredients - No boiling recipe found for %d ingredients"), 
		   Ingredients.Num());
	return false;
}

FText UCookingMethodBoiling::GetCookingMethodName_Implementation() const
{
	return FText::FromString(TEXT("Boiling"));
} 