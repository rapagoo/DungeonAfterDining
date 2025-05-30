#pragma once

#include "CoreMinimal.h"
#include "Cooking/CookingMethodBase.h"
#include "GameplayTagContainer.h"
#include "CookingMethodFrying.generated.h"

/**
 * 튀기기 요리법 클래스
 */
UCLASS(Blueprintable, BlueprintType)
class DUNGEON_API UCookingMethodFrying : public UCookingMethodBase
{
	GENERATED_BODY()

public:
	UCookingMethodFrying();

	// UCookingMethodBase 인터페이스 구현
	virtual bool ProcessIngredients_Implementation(const TArray<FName>& Ingredients, UDataTable* RecipeDataTable, UDataTable* ItemDataTable, FName& OutCookedItemID, float& OutCookingDuration) override;
	virtual FText GetCookingMethodName_Implementation() const override;

protected:
	/** 기본 튀기기 시간 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frying Settings")
	float BaseFryingTime = 15.0f;

	/** 재료 개수에 따른 시간 추가 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frying Settings")
	float TimePerIngredient = 2.0f;

	/** 최대 튀기기 시간 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Frying Settings")
	float MaxFryingTime = 25.0f;

	// Example: Frying might have specific parameters
	UPROPERTY(EditDefaultsOnly, Category = "Frying")
	float OilTemperature = 180.0f; // Example property specific to frying

	// Maybe a specific sound or particle for frying
	UPROPERTY(EditDefaultsOnly, Category = "Frying|Visuals")
	TObjectPtr<UParticleSystem> FryingOilSplatterEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Frying|Sound")
	TObjectPtr<USoundBase> FryingSizzleSound; 
}; 