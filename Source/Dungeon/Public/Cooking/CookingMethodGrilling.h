#pragma once

#include "CoreMinimal.h"
#include "Cooking/CookingMethodBase.h"
#include "GameplayTagContainer.h"
#include "Inventory/InvenItemStruct.h"
#include "CookingMethodGrilling.generated.h"

/**
 * Represents a grilling cooking method.
 */
UCLASS(Blueprintable, BlueprintType)
class DUNGEON_API UCookingMethodGrilling : public UCookingMethodBase
{
	GENERATED_BODY()

public:
	UCookingMethodGrilling();

	// Override ProcessIngredients from the base class
	virtual bool ProcessIngredients_Implementation(const TArray<FName>& Ingredients, UDataTable* RecipeDataTable, UDataTable* ItemDataTable, FName& OutCookedItemID, float& OutCookingDuration) override;

	// Override GetCookingMethodName
	virtual FText GetCookingMethodName_Implementation() const override;

protected:
	// Example: Grilling might have specific parameters
	UPROPERTY(EditDefaultsOnly, Category = "Grilling")
	float GrillingTemperature = 200.0f; // Example property

	// You could add specific logic or data tables for grilling recipes if they differ significantly
	// from the generic ones, or if this method should only use a subset of recipes.
}; 