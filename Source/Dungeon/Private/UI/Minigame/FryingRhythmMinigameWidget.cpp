// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/Minigame/FryingRhythmMinigameWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Cooking/FryingRhythmMinigame.h"
#include "TimerManager.h"

void UFryingRhythmMinigameWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	if (ShakeButton) ShakeButton->OnClicked.AddDynamic(this, &UFryingRhythmMinigameWidget::OnShakeButtonClicked);
	if (CheckTempButton) CheckTempButton->OnClicked.AddDynamic(this, &UFryingRhythmMinigameWidget::OnCheckTempButtonClicked);

	// Hide the overlay by default
	if (RhythmGameOverlay)
	{
		RhythmGameOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UFryingRhythmMinigameWidget::InitializeMinigameWidget_Implementation(UCookingMinigameBase* Minigame)
{
	FryingRhythmMinigame = Cast<UFryingRhythmMinigame>(Minigame);
	UE_LOG(LogTemp, Log, TEXT("FryingRhythmMinigameWidget initialized."));

	// Set initial UI state
	if (ComboText) ComboText->SetText(FText::FromString(TEXT("콤보: 0")));
	if (TemperatureText) TemperatureText->SetText(FText::FromString(TEXT("온도: 50.0% (적정)")));
}

void UFryingRhythmMinigameWidget::UpdateMinigameWidget_Implementation(float Score, int32 Phase)
{
	if (!FryingRhythmMinigame.IsValid()) return;
	
	if (ComboText)
	{
		int32 CurrentCombo = FryingRhythmMinigame->GetCurrentCombo();
		ComboText->SetText(FText::Format(FText::FromString("콤보: {0}"), FText::AsNumber(CurrentCombo)));
	}
	if (TemperatureText)
	{
		float Temperature = FryingRhythmMinigame->GetCookingTemperature();
		bool bOptimal = FryingRhythmMinigame->IsTemperatureOptimal();
		FString TempMessage = FString::Printf(TEXT("온도: %.1f%% %s"), Temperature * 100.0f, bOptimal ? TEXT("(적정)") : TEXT("(주의!)"));
		TemperatureText->SetText(FText::FromString(TempMessage));
	}
}

FReply UFryingRhythmMinigameWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::SpaceBar)
	{
		OnShakeButtonClicked();
		return FReply::Handled();
	}
	if (InKeyEvent.GetKey() == EKeys::V)
	{
		OnCheckTempButtonClicked();
		return FReply::Handled();
	}
	
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}


void UFryingRhythmMinigameWidget::OnShakeButtonClicked()
{
	if (FryingRhythmMinigame.IsValid())
	{
		FryingRhythmMinigame->HandlePlayerInput(TEXT("Stir"));
	}
}

void UFryingRhythmMinigameWidget::OnCheckTempButtonClicked()
{
	if (FryingRhythmMinigame.IsValid())
	{
		FryingRhythmMinigame->HandlePlayerInput(TEXT("Check"));
	}
}

void UFryingRhythmMinigameWidget::StartRhythmNote(const FString& ActionType, float NoteDuration)
{
	if (!RhythmGameOverlay || !RhythmInnerCircle) return;

	GetWorld()->GetTimerManager().ClearTimer(HideUITimerHandle);

	RhythmGameOverlay->SetVisibility(ESlateVisibility::Visible);
	if(RhythmOuterCircle) RhythmOuterCircle->SetVisibility(ESlateVisibility::Visible);
	RhythmInnerCircle->SetVisibility(ESlateVisibility::Visible);
	RhythmInnerCircle->SetRenderScale(FVector2D(InitialCircleScale, InitialCircleScale));
	
	if (RhythmActionText)
	{
		FString ActionMessage = ActionType == TEXT("Stir") ? TEXT("흔들기 (Space)") : TEXT("온도 확인 (V)");
		RhythmActionText->SetText(FText::FromString(ActionMessage));
		RhythmActionText->SetVisibility(ESlateVisibility::Visible);
	}
	if (RhythmTimingText)
	{
		RhythmTimingText->SetText(FText::FromString(TEXT("")));
		RhythmTimingText->SetVisibility(ESlateVisibility::Visible);
	}
}

void UFryingRhythmMinigameWidget::UpdateRhythmTiming(float Progress)
{
	if (!RhythmInnerCircle) return;
	
	float CurrentScale = FMath::Lerp(InitialCircleScale, 1.0f, Progress);
	RhythmInnerCircle->SetRenderScale(FVector2D(CurrentScale, CurrentScale));
}

void UFryingRhythmMinigameWidget::EndRhythmNote()
{
	GetWorld()->GetTimerManager().SetTimer(HideUITimerHandle, [this]() {
		if (RhythmGameOverlay) RhythmGameOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}, 1.0f, false);
}

void UFryingRhythmMinigameWidget::ShowRhythmResult(const FString& Result)
{
	if (RhythmTimingText)
	{
		RhythmTimingText->SetText(FText::FromString(Result));
	}
} 