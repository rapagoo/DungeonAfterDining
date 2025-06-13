#include "Cooking/FryingTimerMinigame.h"
#include "UI/Minigame/FryingTimerMinigameWidget.h"
#include "Actors/InteractablePot.h"
#include "TimerManager.h"

UFryingTimerMinigame::UFryingTimerMinigame()
{
	PrimaryComponentTick.bCanEverTick = true;
	bIsGameActive = false;
}

void UFryingTimerMinigame::StartMinigame(TWeakObjectPtr<UUserWidget> InWidget, AInteractablePot* InPot)
{
	Super::StartMinigame(InWidget, InPot);
	FryingTimerWidget = Cast<UFryingTimerMinigameWidget>(InWidget.Get());
	if (!FryingTimerWidget.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("FryingTimerMinigame received an incompatible widget!"));
		return;
	}

	bIsGameActive = true;
	CurrentScore = 0;
	CurrentEventIndex = -1;
	GetWorld()->GetTimerManager().SetTimer(EventTriggerHandle, this, &UFryingTimerMinigame::SetUpNextEvent, 1.0f, false);
}

void UFryingTimerMinigame::UpdateMinigame(float DeltaTime)
{
	Super::UpdateMinigame(DeltaTime);
	if (!bIsGameActive) return;

	if (bEventIsActive)
	{
		EventTimer += DeltaTime;
		if (FryingTimerWidget.IsValid())
		{
			float Progress = FMath::Clamp(EventTimer / CurrentEvent.Duration, 0.0f, 1.0f);
			FryingTimerWidget->OnEventUpdate(Progress);
		}

		if (EventTimer >= CurrentEvent.Duration + CurrentEvent.SuccessWindow)
		{
			if (FryingTimerWidget.IsValid())
			{
				FryingTimerWidget->OnEventResult(EFryingTimerEventResult::Fail);
			}
			EndCurrentEvent();
		}
	}
}

void UFryingTimerMinigame::EndMinigame()
{
	Super::EndMinigame();
	bIsGameActive = false;
	GetWorld()->GetTimerManager().ClearTimer(EventTriggerHandle);
}

void UFryingTimerMinigame::HandlePlayerInput(const FString& InputType)
{
	if (bEventIsActive && InputType == CurrentEvent.RequiredInput)
	{
		EFryingTimerEventResult Result;
		if (EventTimer < CurrentEvent.Duration)
		{
			Result = EFryingTimerEventResult::TooEarly;
		}
		else
		{
			Result = EFryingTimerEventResult::Success;
			CurrentScore += 10;
		}

		if (FryingTimerWidget.IsValid())
		{
			FryingTimerWidget->OnEventResult(Result);
		}
		EndCurrentEvent();
	}
}

void UFryingTimerMinigame::SetUpNextEvent()
{
	CurrentEventIndex++;
	if (!Events.IsValidIndex(CurrentEventIndex))
	{
		// All events are done, end the minigame
		EndMinigame();
		return;
	}
	
	GetWorld()->GetTimerManager().SetTimer(EventTriggerHandle, this, &UFryingTimerMinigame::TriggerEvent, TimeBetweenEvents, false);
}

void UFryingTimerMinigame::TriggerEvent()
{
	if (!Events.IsValidIndex(CurrentEventIndex)) return;

	CurrentEvent = Events[CurrentEventIndex];
	bEventIsActive = true;
	EventTimer = 0.0f;
	
	if (FryingTimerWidget.IsValid())
	{
		FryingTimerWidget->OnEventStart(CurrentEvent);
	}
}

void UFryingTimerMinigame::EndCurrentEvent()
{
	bEventIsActive = false;
	if (FryingTimerWidget.IsValid())
	{
		FryingTimerWidget->OnEventEnd();
	}
	// Set up the next event after this one ends
	SetUpNextEvent();
} 