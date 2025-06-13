// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Inventory/MinigameWidgetInterface.h"
#include "GrillingMinigameWidget.generated.h"

class UButton;
class UTextBlock;
class UGrillingMinigame;

/**
 * UI Widget for the Grilling Minigame.
 * Handles all visual elements and player input for grilling.
 */
UCLASS()
class DUNGEON_API UGrillingMinigameWidget : public UUserWidget, public IMinigameWidgetInterface
{
	GENERATED_BODY()

protected:
	//~ Begin UUserWidget Interface
	virtual void NativeConstruct() override;
	//~ End UUserWidget Interface

	//~ Begin IMinigameWidgetInterface
	virtual void InitializeMinigameWidget_Implementation(UCookingMinigameBase* Minigame) override;
	virtual void UpdateMinigameWidget_Implementation(float Score, int32 Phase) override;
	virtual void UpdateRequiredAction_Implementation(const FString& ActionType, bool bIsRequired) override;
	virtual void ShowEventResult_Implementation(const FString& ResultText) override;
	//~ End IMinigameWidgetInterface

	/** Handles input for the grilling minigame */
	UFUNCTION()
	void HandleGrillingInput(const FString& InputType);

	/** Button click handlers */
	UFUNCTION()
	void OnFlipButtonClicked();
	UFUNCTION()
	void OnHeatUpButtonClicked();
	UFUNCTION()
	void OnHeatDownButtonClicked();
	UFUNCTION()
	void OnCheckButtonClicked();

protected:
	UPROPERTY(meta = (BindWidget))
	UButton* FlipButton;

	UPROPERTY(meta = (BindWidget))
	UButton* HeatUpButton;

	UPROPERTY(meta = (BindWidget))
	UButton* HeatDownButton;

	UPROPERTY(meta = (BindWidget))
	UButton* CheckButton;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* GrillingStatusText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TemperatureText;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* ActionHintText;

private:
	/** A weak pointer to the minigame logic class */
	UPROPERTY()
	TWeakObjectPtr<UGrillingMinigame> GrillingMinigame;
}; 