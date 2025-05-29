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
class AInteractablePot;
class UCookingMinigameBase;
class UButton;
class UTextBlock;
class UVerticalBox;

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

	/** Finds the first InventoryItemActor that is sliced and near the interactable */
	AInventoryItemActor* FindNearbySlicedIngredient();

	/** NEW: Called by InteractablePot to update the entire widget's state */
	void UpdateWidgetState(const TArray<FName>& IngredientIDs, bool bIsPotCooking, bool bIsPotCookingComplete, bool bIsPotBurnt, FName CookedResultID);

	/** NEW: Start a timing event for the minigame */
	UFUNCTION(BlueprintCallable, Category = "Cooking Minigame")
	void StartTimingEvent();

	/** NEW: Check if timing event has expired */
	void CheckTimingEventTimeout();

	/** NEW: Called when the Stir button is clicked during cooking */
	UFUNCTION()
	void OnStirButtonClicked();

	/** NEW: Grilling minigame button handlers */
	UFUNCTION()
	void OnFlipButtonClicked();

	UFUNCTION()
	void OnHeatUpButtonClicked();

	UFUNCTION()
	void OnHeatDownButtonClicked();

	UFUNCTION()
	void OnCheckButtonClicked();

	// --- NEW: Minigame System Functions ---
	/** Called when a cooking minigame starts */
	UFUNCTION(BlueprintCallable, Category = "Cooking Minigame")
	void OnMinigameStarted(UCookingMinigameBase* Minigame);

	/** Called when a cooking minigame updates */
	UFUNCTION(BlueprintCallable, Category = "Cooking Minigame")
	void OnMinigameUpdated(float Score, int32 Phase);

	/** Called when a cooking minigame ends */
	UFUNCTION(BlueprintCallable, Category = "Cooking Minigame")
	void OnMinigameEnded(int32 Result);

	/** Handle minigame input */
	UFUNCTION(BlueprintCallable, Category = "Cooking Minigame")
	void HandleMinigameInput(const FString& InputType);

	/** NEW: Update required action for current minigame */
	UFUNCTION(BlueprintCallable, Category = "Cooking Minigame")
	void UpdateRequiredAction(const FString& ActionType, bool bActionRequired);

protected:
	virtual void NativeConstruct() override;

	/** Button to interact with nearby ingredient (Add to Pot originally, now Add to Inventory) */
	UPROPERTY(meta = (BindWidget))
	UButton* AddIngredientButton;

	/** Button to initiate cooking */
	UPROPERTY(meta = (BindWidget))
	UButton* CookButton;

	/** NEW: Button to collect the cooked item */
	UPROPERTY(meta = (BindWidget))
	UButton* CollectButton;

	/** NEW: Button for timing minigame during cooking */
	UPROPERTY(meta = (BindWidget))
	UButton* StirButton;

	/** NEW: Grilling minigame buttons */
	UPROPERTY(meta = (BindWidget))
	UButton* FlipButton;

	UPROPERTY(meta = (BindWidget))
	UButton* HeatUpButton;

	UPROPERTY(meta = (BindWidget))
	UButton* HeatDownButton;

	UPROPERTY(meta = (BindWidget))
	UButton* CheckButton;

	/** Vertical box to list the added ingredients */
	UPROPERTY(meta = (BindWidget))
	UVerticalBox* IngredientsList;

	/** NEW: TextBlock to display current cooking status */
	UPROPERTY(meta = (BindWidget))
	UTextBlock* StatusText;

	/** NEW: TextBlock to display current required action */
	UPROPERTY(meta = (BindWidget))
	UTextBlock* ActionText;

	/** The class of widget to represent a single ingredient in the list */
	UPROPERTY(EditDefaultsOnly, Category="Cooking UI")
	TSubclassOf<UUserWidget> IngredientEntryWidgetClass;

	/** Reference to the table/pot this widget is associated with */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cooking Logic")
	AInteractableTable* AssociatedInteractable; // Renamed for clarity

	/** Pointer to the item actor currently detected nearby on the table/pot surface */
	UPROPERTY()
	TWeakObjectPtr<AInventoryItemActor> NearbyIngredient;

	/** NEW: Timing minigame state variables */
	UPROPERTY()
	bool bIsTimingEventActive = false;

	UPROPERTY()
	float TimingEventStartTime = 0.0f;

	UPROPERTY()
	float TimingEventDuration = 2.0f; // How long player has to respond

	/** Called when the Add Ingredient button is clicked (Now adds nearby item to inventory) */
	UFUNCTION()
	void OnAddIngredientClicked();

	/** Called when the Cook button is clicked */
	UFUNCTION()
	void OnCookClicked();

	/** NEW: Called when the Collect button is clicked */
	UFUNCTION()
	void OnCollectButtonClicked();

	// UFUNCTION(BlueprintImplementableEvent, Category = "Cooking UI") // 주석 처리 또는 삭제
	// void UpdateIngredientDisplay(const TArray<FName>& IngredientIDs);

private:
	/** NEW: Current active minigame reference */
	UPROPERTY()
	TWeakObjectPtr<UCookingMinigameBase> CurrentMinigame;

	/** NEW: Whether we're currently in minigame mode */
	UPROPERTY()
	bool bIsInMinigameMode = false;

}; 