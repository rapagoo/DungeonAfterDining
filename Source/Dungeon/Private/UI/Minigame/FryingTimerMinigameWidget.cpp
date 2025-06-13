// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/Minigame/FryingTimerMinigameWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Cooking/FryingTimerMinigame.h"
#include "TimerManager.h"

void UFryingTimerMinigameWidget::InitializeMinigameWidget_Implementation(UCookingMinigameBase* Minigame)
{
	FryingTimerMinigame = Cast<UFryingTimerMinigame>(Minigame);
	UE_LOG(LogTemp, Log, TEXT("FryingTimerMinigameWidget initialized."));

	// Set initial UI state
	if (EventProgressBar) EventProgressBar->SetPercent(0.0f);
	if (EventDescriptionText) EventDescriptionText->SetText(FText::GetEmpty());
	if (EventResultText) EventResultText->SetVisibility(ESlateVisibility::Collapsed);
}

void UFryingTimerMinigameWidget::OnEventStart(const FFryingTimerEvent& Event)
{
	if (EventDescriptionText)
	{
		EventDescriptionText->SetText(Event.Description);
	}
	if (EventProgressBar)
	{
		EventProgressBar->SetVisibility(ESlateVisibility::Visible);
		EventProgressBar->SetPercent(0.0f);
	}
	if (EventResultText)
	{
		EventResultText->SetVisibility(ESlateVisibility::Collapsed);
	}
	GetWorld()->GetTimerManager().ClearTimer(HideResultTimerHandle);
}

void UFryingTimerMinigameWidget::OnEventUpdate(float Progress)
{
	if (EventProgressBar)
	{
		EventProgressBar->SetPercent(Progress);
	}
}

void UFryingTimerMinigameWidget::OnEventEnd()
{
	if (EventProgressBar)
	{
		EventProgressBar->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (EventDescriptionText)
	{
		EventDescriptionText->SetText(FText::GetEmpty());
	}
}

void UFryingTimerMinigameWidget::OnEventResult(EFryingTimerEventResult Result)
{
	if (EventResultText)
	{
		FText ResultText;
		switch(Result)
		{
		case EFryingTimerEventResult::Success:
			ResultText = FText::FromString(TEXT("성공!"));
			break;
		case EFryingTimerEventResult::Fail:
			ResultText = FText::FromString(TEXT("실패..."));
			break;
		case EFryingTimerEventResult::TooEarly:
			ResultText = FText::FromString(TEXT("너무 빨라요!"));
			break;
		}
		EventResultText->SetText(ResultText);
		EventResultText->SetVisibility(ESlateVisibility::Visible);
		
		// Hide the result text after a short delay
		GetWorld()->GetTimerManager().SetTimer(HideResultTimerHandle, [this]() {
			if (EventResultText) EventResultText->SetVisibility(ESlateVisibility::Collapsed);
		}, 1.5f, false);
	}
} 