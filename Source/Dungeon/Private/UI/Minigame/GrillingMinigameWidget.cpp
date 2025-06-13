// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/Minigame/GrillingMinigameWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Cooking/GrillingMinigame.h"

void UGrillingMinigameWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Bind button clicks to the input handler
	if (FlipButton) FlipButton->OnClicked.AddDynamic(this, &UGrillingMinigameWidget::OnFlipButtonClicked);
	if (HeatUpButton) HeatUpButton->OnClicked.AddDynamic(this, &UGrillingMinigameWidget::OnHeatUpButtonClicked);
	if (HeatDownButton) HeatDownButton->OnClicked.AddDynamic(this, &UGrillingMinigameWidget::OnHeatDownButtonClicked);
	if (CheckButton) CheckButton->OnClicked.AddDynamic(this, &UGrillingMinigameWidget::OnCheckButtonClicked);
}

void UGrillingMinigameWidget::InitializeMinigameWidget_Implementation(UCookingMinigameBase* Minigame)
{
	GrillingMinigame = Cast<UGrillingMinigame>(Minigame);
	if (GrillingMinigame.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("GrillingMinigameWidget initialized with %s"), *GrillingMinigame->GetName());
		// Set initial UI state
		if (GrillingStatusText)
		{
			GrillingStatusText->SetText(FText::FromString(TEXT("굽기 미니게임을 시작합니다!")));
		}
		if (TemperatureText)
		{
			TemperatureText->SetText(FText::FromString(TEXT("온도: -")));
		}
		if (ActionHintText)
		{
			ActionHintText->SetText(FText::FromString(TEXT("...")));
		}
	}
}

void UGrillingMinigameWidget::UpdateMinigameWidget_Implementation(float Score, int32 Phase)
{
	if (GrillingMinigame.IsValid())
	{
		// Update score text, temperature text, etc.
		if (GrillingStatusText)
		{
			GrillingStatusText->SetText(FText::Format(FText::FromString("점수: {0}"), FText::AsNumber(Score)));
		}
		if (TemperatureText)
		{
			// Assuming GrillingMinigame has a function to get temperature as a string or float
			// float CurrentTemp = GrillingMinigame->GetCurrentTemperature();
			// TemperatureText->SetText(FText::Format(FText::FromString("온도: {0}"), FText::AsNumber(CurrentTemp)));
		}
	}
}

void UGrillingMinigameWidget::UpdateRequiredAction_Implementation(const FString& ActionType, bool bIsRequired)
{
	// Disable all buttons first
	if(FlipButton) FlipButton->SetIsEnabled(false);
	if(HeatUpButton) HeatUpButton->SetIsEnabled(true); // Usually, temp can always be changed
	if(HeatDownButton) HeatDownButton->SetIsEnabled(true);
	if(CheckButton) CheckButton->SetIsEnabled(true); // Checking is always available

	if (bIsRequired)
	{
		if (ActionHintText)
		{
			ActionHintText->SetText(FText::Format(FText::FromString("필요한 행동: {0}!"), FText::FromString(ActionType)));
		}

		// Enable the specific button required for the action
		if (ActionType == TEXT("Flip") && FlipButton)
		{
			FlipButton->SetIsEnabled(true);
		}
		// Other actions could be handled here if needed
	}
	else
	{
		if (ActionHintText)
		{
			ActionHintText->SetText(FText::FromString(TEXT("...")));
		}
	}
}

void UGrillingMinigameWidget::ShowEventResult_Implementation(const FString& ResultText)
{
	if (ActionHintText)
	{
		ActionHintText->SetText(FText::FromString(ResultText));

		// Optional: Clear the text after a short delay
	}
}

void UGrillingMinigameWidget::HandleGrillingInput(const FString& InputType)
{
	if (GrillingMinigame.IsValid())
	{
		GrillingMinigame->HandlePlayerInput(InputType);
	}
}

void UGrillingMinigameWidget::OnFlipButtonClicked()
{
	HandleGrillingInput(TEXT("Flip"));
}

void UGrillingMinigameWidget::OnHeatUpButtonClicked()
{
	HandleGrillingInput(TEXT("HeatUp"));
}

void UGrillingMinigameWidget::OnHeatDownButtonClicked()
{
	HandleGrillingInput(TEXT("HeatDown"));
}

void UGrillingMinigameWidget::OnCheckButtonClicked()
{
	HandleGrillingInput(TEXT("Check"));
} 