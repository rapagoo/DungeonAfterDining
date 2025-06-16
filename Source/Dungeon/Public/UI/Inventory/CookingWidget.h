// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CookingWidget.generated.h"

// Forward Declarations
class UButton;
class UVerticalBox;
class UTextBlock;
class UProgressBar;
class UOverlay;
class UImage;
class UTimerMinigame;
class UCookingMinigameBase;
class UIngredientSlotWidget;
struct FSlotStruct;
struct FItemData;

UCLASS()
class DUNGEON_API UCookingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Delegate broadcasted when the "Start Cooking" button is clicked */
	DECLARE_EVENT(UCookingWidget, FOnStartCooking)
	FOnStartCooking OnStartCooking;

	/** Delegate broadcasted when the "Collect Results" button is clicked */
	DECLARE_EVENT(UCookingWidget, FOnCollectResults)
	FOnCollectResults OnCollectResults;

	/** Delegate broadcasted when the "Add Ingredient" button is clicked */
	DECLARE_EVENT(UCookingWidget, FOnAddIngredient)
	FOnAddIngredient OnAddIngredient;

	/**
	 * Updates the list of ingredients displayed on the widget.
	 * @param Ingredients The array of ingredient data to display.
	 */
	UFUNCTION(BlueprintCallable, Category = "Cooking UI")
	void UpdateIngredientsList(const TArray<FSlotStruct>& Ingredients);

	/**
	 * Called from the owning actor when a minigame starts.
	 * @param Minigame The instance of the started minigame.
	 */
	UFUNCTION(BlueprintCallable, Category = "Cooking UI")
	void OnMinigameStarted(UCookingMinigameBase* Minigame);

	/**
	 * Called from the owning actor when a minigame ends.
	 * @param bSuccess Whether the cooking was successful.
	 * @param Results The items resulting from the cooking process.
	 */
	UFUNCTION(BlueprintCallable, Category = "Cooking UI")
	void OnMinigameEnded(bool bSuccess, const TArray<FItemData>& Results);

	/**
	 * Resets the widget to its initial state, clearing ingredient lists and status messages.
	 */
	UFUNCTION(BlueprintCallable, Category = "Cooking UI")
	void ResetWidget();

	/**
	 * Sets the visibility of the main cooking UI elements (ingredient list, add/start buttons).
	 * @param bShow Whether to show or hide the UI.
	 */
	UFUNCTION(BlueprintCallable, Category = "Cooking UI")
	void SetUIVisibility(bool bShow);


protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Populates the ingredient list with one starting "???" item */
	void InitializeIngredientList();
	
	/** Called when the Start button is clicked */
	UFUNCTION()
	void OnStartButtonClicked();

	/** Called when the Collect button is clicked */
	UFUNCTION()
	void OnCollectButtonClicked();
	
	/** Called when the Add Ingredient button is clicked */
	UFUNCTION()
	void OnAddIngredientClicked();

	/** Called when the new event action button is clicked */
	UFUNCTION()
	void OnEventActionButtonClicked();

private:
	/** Binds to the minigame's OnTimerEventSpawned delegate */
	UFUNCTION()
	void HandleTimerEventSpawned(const struct FTimerEvent& EventData);

	/** Binds to the minigame's OnTimerEventCompleted delegate */
	UFUNCTION()
	void HandleTimerEventCompleted(bool bSuccess);

protected:
	/** The class of widget to represent a single ingredient in the list */
	UPROPERTY(EditDefaultsOnly, Category="Cooking UI")
	TSubclassOf<UIngredientSlotWidget> IngredientSlotWidgetClass;

	/* Main UI Components */
	UPROPERTY(meta = (BindWidget))
	UButton* AddIngredientButton;

	UPROPERTY(meta = (BindWidget))
	UVerticalBox* IngredientsList;

	UPROPERTY(meta = (BindWidget))
	UButton* StartButton;

	UPROPERTY(meta = (BindWidget))
	UButton* CollectButton;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* StatusText;

	/* Timer Minigame UI Elements */
	UPROPERTY(meta = (BindWidget))
	UProgressBar* OverallProgressBar;

	UPROPERTY(meta = (BindWidget))
	UOverlay* TimerEventOverlay;

	UPROPERTY(meta = (BindWidget))
	UImage* EventBackground;
	
	UPROPERTY(meta = (BindWidget))
	UImage* EventSuccessZone;

	UPROPERTY(meta = (BindWidget))
	UImage* EventArrow;

	UPROPERTY(meta = (BindWidget))
	UButton* EventActionButton;

private:
	/** A pointer to the active timer minigame instance */
	UPROPERTY()
	TWeakObjectPtr<UTimerMinigame> CurrentTimerMinigame;

	/** A dynamic material instance for the success zone */
	UPROPERTY()
	UMaterialInstanceDynamic* SuccessZoneMID;

	/** Current active minigame reference */
	UPROPERTY()
	UCookingMinigameBase* CurrentMinigame = nullptr;
}; 