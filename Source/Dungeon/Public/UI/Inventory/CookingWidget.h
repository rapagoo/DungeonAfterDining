// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "CookingWidget.generated.h"

// Forward declaration for the item actor
class AInventoryItemActor;

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

protected:
	virtual void NativeConstruct() override;

	/** Button to add an ingredient from the table */
	UPROPERTY(meta = (BindWidget))
	class UButton* AddIngredientButton;

	/** Button to initiate cooking */
	UPROPERTY(meta = (BindWidget))
	class UButton* CookButton;

	/** Vertical box to list the added ingredients */
	UPROPERTY(meta = (BindWidget))
	class UVerticalBox* IngredientsList;

	/** Called when the Add Ingredient button is clicked */
	UFUNCTION()
	void OnAddIngredientClicked();

	/** Called when the Cook button is clicked */
	UFUNCTION()
	void OnCookClicked();

private:
	/** Pointer to the item actor currently detected nearby */
	TWeakObjectPtr<AInventoryItemActor> NearbyIngredient;

	/** List to keep track of the names of ingredients added */
	UPROPERTY()
	TArray<FString> AddedIngredientNames; // Use FItemData or similar later

}; 