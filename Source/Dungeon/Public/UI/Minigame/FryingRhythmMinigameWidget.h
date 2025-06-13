// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Inventory/MinigameWidgetInterface.h"
#include "FryingRhythmMinigameWidget.generated.h"

class UButton;
class UTextBlock;
class UImage;
class UOverlay;
class UFryingRhythmMinigame;

/**
 * Widget for the Frying Rhythm Minigame.
 */
UCLASS()
class DUNGEON_API UFryingRhythmMinigameWidget : public UUserWidget, public IMinigameWidgetInterface
{
	GENERATED_BODY()

public:
	// These functions will be called by the FryingRhythmMinigame logic class to update the UI
	void StartRhythmNote(const FString& ActionType, float NoteDuration);
	void UpdateRhythmTiming(float Progress);
	void EndRhythmNote();
	void ShowRhythmResult(const FString& Result);

protected:
	//~ Begin UUserWidget Interface
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	//~ End UUserWidget Interface

	//~ Begin IMinigameWidgetInterface
	virtual void InitializeMinigameWidget_Implementation(UCookingMinigameBase* Minigame) override;
	virtual void UpdateMinigameWidget_Implementation(float Score, int32 Phase) override;
	// The other interface functions can be left empty if not needed for this specific minigame.
	virtual void UpdateRequiredAction_Implementation(const FString& ActionType, bool bIsRequired) override {};
	virtual void ShowEventResult_Implementation(const FString& ResultText) override {};
	//~ End IMinigameWidgetInterface
	
	/** Button click handlers */
	UFUNCTION()
	void OnShakeButtonClicked(); // Renamed from Stir for clarity
	
	UFUNCTION()
	void OnCheckTempButtonClicked();

protected:
	UPROPERTY(meta = (BindWidget))
	UTextBlock* ComboText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TemperatureText;
	
	UPROPERTY(meta = (BindWidget))
	UOverlay* RhythmGameOverlay;

	UPROPERTY(meta = (BindWidget))
	UImage* RhythmOuterCircle;

	UPROPERTY(meta = (BindWidget))
	UImage* RhythmInnerCircle;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* RhythmActionText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* RhythmTimingText;

	// These buttons might not be direct children but exist for keyboard shortcuts
	UPROPERTY(meta = (BindWidget))
	UButton* ShakeButton; // Corresponds to the old "StirButton"

	UPROPERTY(meta = (BindWidget))
	UButton* CheckTempButton; // Corresponds to the old "CheckButton"

private:
	UPROPERTY()
	TWeakObjectPtr<UFryingRhythmMinigame> FryingRhythmMinigame;
	
	UPROPERTY()
	FTimerHandle HideUITimerHandle;
	
	UPROPERTY()
	float InitialCircleScale = 3.0f;
}; 