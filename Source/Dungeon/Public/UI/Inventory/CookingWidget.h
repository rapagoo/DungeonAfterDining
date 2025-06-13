// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/ProgressBar.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/DataTable.h"
#include "Inventory/SlotStruct.h"
#include "Cooking/FryingTimerMinigame.h"
#include "CookingWidget.generated.h"

// Forward declaration for the item actor
class AInventoryItemActor;
class AInteractableTable;
class AInteractablePot;
class UCookingMinigameBase;
class UButton;
class UTextBlock;
class UVerticalBox;
class UImage;
class UOverlay;
class UWidgetSwitcher;

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

	// --- All minigame specific functions below will be moved to their respective widgets ---
	// --- We keep the core minigame lifecycle functions to manage the widget switcher ---

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

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** NEW: Handle keyboard input - This will be simplified or moved */
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/** Button to interact with nearby ingredient (Add to Pot originally, now Add to Inventory) */
	UPROPERTY(meta = (BindWidget))
	UButton* AddIngredientButton;

	/** Button to initiate cooking */
	UPROPERTY(meta = (BindWidget))
	UButton* CookButton;

	/** NEW: Button to collect the cooked item */
	UPROPERTY(meta = (BindWidget))
	UButton* CollectButton;

	/** Vertical box to list the added ingredients */
	UPROPERTY(meta = (BindWidget))
	UVerticalBox* IngredientsList;

	/** NEW: TextBlock to display current cooking status */
	UPROPERTY(meta = (BindWidget))
	UTextBlock* StatusText;

	/** NEW: TextBlock to display current required action */
	UPROPERTY(meta = (BindWidget))
	UTextBlock* ActionText;

	/** NEW: Widget Switcher to hold the different minigame UIs */
	UPROPERTY(meta = (BindWidget))
	UWidgetSwitcher* MinigameSwitcher;

	/** The class of widget to represent a single ingredient in the list */
	UPROPERTY(EditDefaultsOnly, Category="Cooking UI")
	TSubclassOf<UUserWidget> IngredientEntryWidgetClass;

	/** NEW: Map of minigame classes to the widget classes that should be displayed for them */
	UPROPERTY(EditDefaultsOnly, Category = "Cooking UI")
	TMap<TSubclassOf<UCookingMinigameBase>, TSubclassOf<UUserWidget>> MinigameWidgetMap;

	/** Reference to the table/pot this widget is associated with */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cooking Logic")
	AInteractableTable* AssociatedInteractable; // Renamed for clarity

	/** Pointer to the item actor currently detected nearby on the table/pot surface */
	UPROPERTY()
	TWeakObjectPtr<AInventoryItemActor> NearbyIngredient;

	/** Called when the Add Ingredient button is clicked (Now adds nearby item to inventory) */
	UFUNCTION()
	void OnAddIngredientClicked();

	/** Called when the Cook button is clicked */
	UFUNCTION()
	void OnCookClicked();

	/** NEW: Called when the Collect button is clicked */
	UFUNCTION()
	void OnCollectButtonClicked();

private:
	/** NEW: Current active minigame reference */
	UPROPERTY()
	TWeakObjectPtr<UCookingMinigameBase> CurrentMinigame;

	/** NEW: Whether we're currently in minigame mode */
	UPROPERTY()
	bool bIsInMinigameMode = false;

	/** NEW: Timer handle for UI hiding delay */
	UPROPERTY()
	FTimerHandle HideUITimerHandle;

	/** NEW: Current cooking method reference */
	UPROPERTY()
	TWeakObjectPtr<class UCookingMethodBase> CurrentCookingMethod;

	/** NEW: Reference to the currently active minigame widget instance */
	UPROPERTY()
	TWeakObjectPtr<UUserWidget> CurrentMinigameWidget;
}; 