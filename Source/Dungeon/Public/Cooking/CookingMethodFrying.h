#pragma once

#include "CoreMinimal.h"
#include "Cooking/CookingMethodBase.h"
#include "GameplayTagContainer.h"
#include "CookingMethodFrying.generated.h"

/**
 * Represents a frying cooking method.
 */
UCLASS(Blueprintable, BlueprintType)
class DUNGEON_API UCookingMethodFrying : public UCookingMethodBase
{
	GENERATED_BODY()

public:
	UCookingMethodFrying();

	// Override ProcessIngredients from the base class
	virtual bool ProcessIngredients_Implementation(const TArray<FName>& Ingredients, UDataTable* RecipeDataTable, UDataTable* ItemDataTable, FName& OutCookedItemID, float& OutCookingDuration) override;

	// Override GetCookingMethodName
	virtual FText GetCookingMethodName_Implementation() const override;

protected:
	// Example: Frying might have specific parameters
	UPROPERTY(EditDefaultsOnly, Category = "Frying")
	float OilTemperature = 180.0f; // Example property specific to frying

	// Maybe a specific sound or particle for frying
	UPROPERTY(EditDefaultsOnly, Category = "Frying|Visuals")
	TObjectPtr<UParticleSystem> FryingOilSplatterEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Frying|Sound")
	TObjectPtr<USoundBase> FryingSizzleSound; 
}; 