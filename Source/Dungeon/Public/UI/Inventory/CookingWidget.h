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
class UImage;
class UOverlay;

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

	/** NEW: Set button text dynamically */
	UFUNCTION(BlueprintCallable, Category = "UI Helper")
	void SetButtonText(UButton* Button, const FString& Text);

	/** NEW: Handle rhythm game input for frying */
	UFUNCTION(BlueprintCallable, Category = "Rhythm Game")
	void HandleRhythmGameInput();

	/** NEW: Rhythm game functions */
	UFUNCTION(BlueprintCallable, Category = "Rhythm Game")
	void StartRhythmGameNote(const FString& ActionType, float NoteDuration);

	UFUNCTION(BlueprintCallable, Category = "Rhythm Game")
	void UpdateRhythmGameTiming(float Progress);

	UFUNCTION(BlueprintCallable, Category = "Rhythm Game")
	void EndRhythmGameNote();

	UFUNCTION(BlueprintCallable, Category = "Rhythm Game")
	void ShowRhythmGameResult(const FString& Result);

	/** NEW: Timer-based minigame UI functions */
	UFUNCTION(BlueprintCallable, Category = "Timer Minigame")
	void StartTimerBasedMinigame(float TotalTime);

	UFUNCTION(BlueprintCallable, Category = "Timer Minigame")
	void UpdateMainTimer(float Progress, float RemainingTime);

	UFUNCTION(BlueprintCallable, Category = "Timer Minigame")
	void StartCircularEvent(float StartAngle, float EndAngle, float ArrowSpeed);

	UFUNCTION(BlueprintCallable, Category = "Timer Minigame")
	void UpdateCircularEvent(float ArrowAngle);

	UFUNCTION(BlueprintCallable, Category = "Timer Minigame")
	void EndCircularEvent(const FString& Result);

	UFUNCTION(BlueprintCallable, Category = "Timer Minigame")
	void HideCircularEvent();

protected:
	virtual void NativeConstruct() override;

	/** NEW: Handle keyboard input */
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

	/** NEW: TextBlock to display rhythm game combo information */
	UPROPERTY(meta = (BindWidget))
	UTextBlock* ComboText;

	/** NEW: TextBlock to display cooking temperature for frying rhythm game */
	UPROPERTY(meta = (BindWidget))
	UTextBlock* TemperatureText;

	/** NEW: Rhythm game circular timing UI elements */
	UPROPERTY(meta = (BindWidget))
	class UImage* RhythmOuterCircle;

	UPROPERTY(meta = (BindWidget))
	class UImage* RhythmInnerCircle;

	UPROPERTY(meta = (BindWidget))
	class UOverlay* RhythmGameOverlay;

	/** NEW: Arrow image for timer-based minigame */
	UPROPERTY(meta = (BindWidget))
	class UImage* RhythmArrowImage;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* RhythmActionText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* RhythmTimingText;

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

	/** NEW: Delay before hiding circular event UI (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timer Minigame", meta = (DisplayName = "UI Hide Delay"))
	float CircularEventHideDelay = 2.5f;


	// UFUNCTION(BlueprintImplementableEvent, Category = "Cooking UI") // 주석 처리 또는 삭제
	// void UpdateIngredientDisplay(const TArray<FName>& IngredientIDs);

private:
	/** NEW: Current active minigame reference */
	UPROPERTY()
	TWeakObjectPtr<UCookingMinigameBase> CurrentMinigame;

	/** NEW: Whether we're currently in minigame mode */
	UPROPERTY()
	bool bIsInMinigameMode = false;

	/** NEW: Rhythm game state variables */
	UPROPERTY()
	bool bIsRhythmNoteActive = false;

	UPROPERTY()
	float RhythmNoteStartTime = 0.0f;

	UPROPERTY()
	float RhythmNoteDuration = 3.0f;

	UPROPERTY()
	FString CurrentRhythmAction = TEXT("");

	UPROPERTY()
	float InitialCircleScale = 3.0f;

	/** NEW: Current required action for cooking */
	UPROPERTY()
	FString CurrentRequiredAction = TEXT("");

	/** NEW: Whether we're in frying minigame mode */
	UPROPERTY()
	bool bIsFryingGame = false;

	/** NEW: Timer handle for UI hiding delay */
	UPROPERTY()
	FTimerHandle HideUITimerHandle;



	/** NEW: Current cooking method reference */
	UPROPERTY()
	TWeakObjectPtr<class UCookingMethodBase> CurrentCookingMethod;

	/** NEW: Timer-based minigame state variables */
	UPROPERTY()
	bool bIsTimerMinigameActive = false;

	UPROPERTY()
	float TimerMinigameTotalTime = 20.0f;

	UPROPERTY()
	float TimerMinigameRemainingTime = 20.0f;

	UPROPERTY()
	bool bIsCircularEventActive = false;

	UPROPERTY()
	float CircularEventStartAngle = 0.0f;

	UPROPERTY()
	float CircularEventEndAngle = 60.0f;

	UPROPERTY()
	float CircularEventArrowAngle = 0.0f;

	UPROPERTY()
	float CircularEventArrowSpeed = 180.0f;

};