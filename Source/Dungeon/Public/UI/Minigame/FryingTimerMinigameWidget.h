// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Inventory/MinigameWidgetInterface.h"
#include "Cooking/FryingTimerMinigame.h" // For FFryingTimerEvent and EFryingTimerEventResult enums
#include "FryingTimerMinigameWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UFryingTimerMinigame;

/**
 * 
 */
UCLASS()
class DUNGEON_API UFryingTimerMinigameWidget : public UUserWidget, public IMinigameWidgetInterface
{
	GENERATED_BODY()

public:
	// Called from FryingTimerMinigame logic class
	void OnEventStart(const FFryingTimerEvent& Event);
	void OnEventUpdate(float Progress);
	void OnEventEnd();
	void OnEventResult(EFryingTimerEventResult Result);

protected:
	//~ Begin IMinigameWidgetInterface
	virtual void InitializeMinigameWidget_Implementation(UCookingMinigameBase* Minigame) override;
	// Other interface functions are not strictly needed here but can be implemented if required
	virtual void UpdateMinigameWidget_Implementation(float Score, int32 Phase) override {};
	virtual void UpdateRequiredAction_Implementation(const FString& ActionType, bool bIsRequired) override {};
	virtual void ShowEventResult_Implementation(const FString& ResultText) override {};
	//~ End IMinigameWidgetInterface

protected:
	UPROPERTY(meta = (BindWidget))
	UProgressBar* EventProgressBar;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* EventDescriptionText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* EventResultText;

private:
	UPROPERTY()
	TWeakObjectPtr<UFryingTimerMinigame> FryingTimerMinigame;

	UPROPERTY()
	FTimerHandle HideResultTimerHandle;
}; 