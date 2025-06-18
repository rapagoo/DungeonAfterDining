#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "Inventory/InvenItemStruct.h"
#include "CookingMethodBase.generated.h"

// Forward declaration
struct FInventoryItemStruct;
class UDataTable;

/**
 * Base class for different cooking methods.
 */
UCLASS(Blueprintable, BlueprintType, Abstract) // Abstract: Cannot be instantiated directly
class DUNGEON_API UCookingMethodBase : public UObject
{
	GENERATED_BODY()

public:
	UCookingMethodBase();

protected:
	// Tag that represents this specific cooking method, to be set in derived classes.
	// Example: "Cooking.Method.Grill"
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking Method")
	FGameplayTag CookingMethodTag;

public:
	/**
	 * Processes the given ingredients using this cooking method.
	 * @param Ingredients A list of ingredient IDs.
	 * @param RecipeDataTable The data table containing cooking recipes.
	 * @param ItemDataTable The data table containing item definitions.
	 * @param OutCookedItemID The ID of the item produced by cooking.
	 * @param OutCookingDuration The time it will take to cook.
	 * @return True if a valid recipe was found and cooking can start, false otherwise.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Cooking")
	bool ProcessIngredients(const TArray<FName>& Ingredients, UDataTable* RecipeDataTable, UDataTable* ItemDataTable, FName& OutCookedItemID, float& OutCookingDuration);
	virtual bool ProcessIngredients_Implementation(const TArray<FName>& Ingredients, UDataTable* RecipeDataTable, UDataTable* ItemDataTable, FName& OutCookedItemID, float& OutCookingDuration);

	/**
	 * Gets a descriptive name for this cooking method (e.g., "Grilling", "Boiling").
	 * This can be used for UI purposes.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Cooking")
	FText GetCookingMethodName() const;
	virtual FText GetCookingMethodName_Implementation() const;

	// You can add more virtual functions here later if needed, for example:
	// - GetParticleEffectForCooking()
	// - GetSoundEffectForCooking()
	// - ApplyVisualChangesToIngredients(TArray<UStaticMeshComponent*> IngredientMeshes)
}; 