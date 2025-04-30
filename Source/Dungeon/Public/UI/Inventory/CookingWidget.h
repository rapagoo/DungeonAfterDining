// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/DataTable.h"
#include "Inventory/SlotStruct.h"
#include "CookingWidget.generated.h"

// Forward declaration for the item actor
class AInventoryItemActor;
class AInteractableTable;

/**
 * Widget for the cooking interface.
 */
UCLASS()
class DUNGEON_API UCookingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Updates the widget based on the currently detected nearby item actor */
	void UpdateNearbyIngredient(AInventoryItemActor* ItemActor);

	/** Function to associate this widget with a specific table (or Pot) */
	UFUNCTION(BlueprintCallable, Category = "Cooking Logic")
	void SetAssociatedTable(AInteractableTable* Table);

	/** Function called by AInteractablePot to update the ingredient list display */
	UFUNCTION(BlueprintCallable, Category = "Cooking UI")
	void UpdateIngredientList(const TArray<FName>& IngredientIDs);

	/** Function to add an ingredient to the internal list and potentially UI - DEPRECATED */
	// UFUNCTION(BlueprintCallable, Category = "Cooking Logic")
	// void AddIngredient(FName IngredientID);

	/** Finds the first InventoryItemActor that is sliced and near the interactable */
	AInventoryItemActor* FindNearbySlicedIngredient();

protected:
	virtual void NativeConstruct() override;

	/** Button to interact with nearby ingredient (Add to Pot originally, now Add to Inventory) */
	UPROPERTY(meta = (BindWidget))
	class UButton* AddIngredientButton;

	/** Button to initiate cooking */
	UPROPERTY(meta = (BindWidget))
	class UButton* CookButton;

	/** Vertical box to list the added ingredients */
	UPROPERTY(meta = (BindWidget))
	class UVerticalBox* IngredientsList;

	/** Data table containing cooking recipes - DEPRECATED in Widget */
	// UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking")
	// UDataTable* RecipeDataTable;

	/** The class of widget to represent a single ingredient in the list */
	UPROPERTY(EditDefaultsOnly, Category="Cooking UI")
	TSubclassOf<UUserWidget> IngredientEntryWidgetClass;

	/** Reference to the table/pot this widget is associated with */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cooking Logic")
	AInteractableTable* AssociatedInteractable; // Renamed for clarity

	/** Array to keep track of added ingredient IDs - DEPRECATED in Widget */
	// UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cooking Logic")
	// TArray<FName> AddedIngredientIDs;

	/** Pointer to the item actor currently detected nearby on the table/pot surface */
	UPROPERTY()
	TWeakObjectPtr<AInventoryItemActor> NearbyIngredient;

	/** Called when the Add Ingredient button is clicked (Now adds nearby item to inventory) */
	UFUNCTION()
	void OnAddIngredientClicked();

	/** Called when the Cook button is clicked */
	UFUNCTION()
	void OnCookClicked();

	/** Checks if the currently added ingredients match any recipe - DEPRECATED */
	// bool CheckRecipe(FName& OutResultItemID) const;

}; 