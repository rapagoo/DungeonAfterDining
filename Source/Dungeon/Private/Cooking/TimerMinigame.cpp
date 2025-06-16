#include "Cooking/TimerMinigame.h"
#include "UI/Inventory/CookingWidget.h"
#include "InteractablePot.h"
#include "Kismet/KismetMathLibrary.h"

UTimerMinigame::UTimerMinigame()
{
	// UObject does not have a tick function by default.
	// The owning widget will call UpdateMinigame manually.
}

void UTimerMinigame::StartMinigame(UCookingWidget* InWidget, AInteractablePot* InPot)
{
	Super::StartMinigame(InWidget, InPot);

	CurrentCookTime = 0.0f;
	TotalCookTime = BaseCookTime;
	CurrentEventIndex = -1;
	Events.Empty();
	bIsGameActive = true;

	GenerateEvents();

	// Move to the first event
	CurrentEventIndex = 0;
}

void UTimerMinigame::UpdateMinigame(float DeltaTime)
{
	Super::UpdateMinigame(DeltaTime);

	if (!bIsGameActive)
	{
		return;
	}

	CurrentCookTime += DeltaTime;

	if (CurrentEventIndex < Events.Num())
	{
		FTimerEvent& CurrentEvent = Events[CurrentEventIndex];

		if (!CurrentEvent.bIsActive && CurrentCookTime >= CurrentEvent.TriggerTime)
		{
			// Event becomes active
			CurrentEvent.bIsActive = true;
			CurrentArrowAngle = 0.0f;
			OnTimerEventSpawned.Broadcast(CurrentEvent);
		}

		if (CurrentEvent.bIsActive)
		{
			// Rotate arrow
			CurrentArrowAngle += ArrowRotationSpeed * DeltaTime;
			if (CurrentArrowAngle >= 360.0f)
			{
				// Full circle, player missed
				CompleteCurrentEvent(false);
			}
		}
	}
	
	if (CurrentCookTime >= TotalCookTime)
	{
		EndMinigame();
	}
}

void UTimerMinigame::EndMinigame()
{
	if(!bIsGameActive) return;

	bIsGameActive = false;
	
	// Ensure all remaining active events are marked as failed.
	if (CurrentEventIndex < Events.Num() && Events[CurrentEventIndex].bIsActive)
	{
		CompleteCurrentEvent(false);
	}

	// Calculate result
	int32 SucceededEvents = 0;
	for (const FTimerEvent& Event : Events)
	{
		if (Event.bSuccess)
		{
			SucceededEvents++;
		}
	}

	float SuccessRatio = (float)SucceededEvents / (float)NumberOfEvents;
	ECookingMinigameResult Result = ECookingMinigameResult::Failed;
	if (SuccessRatio >= 0.9f)
	{
		Result = ECookingMinigameResult::Perfect;
	}
	else if (SuccessRatio >= 0.7f)
	{
		Result = ECookingMinigameResult::Good;
	}
	else if (SuccessRatio >= 0.5f)
	{
		Result = ECookingMinigameResult::Average;
	}
	else if (SuccessRatio > 0.0f)
	{
		Result = ECookingMinigameResult::Poor;
	}

	NotifyGameEnd(Result);
}

void UTimerMinigame::HandlePlayerInput(const FString& InputType)
{
	Super::HandlePlayerInput(InputType);

	if (!bIsGameActive || CurrentEventIndex >= Events.Num() || !Events[CurrentEventIndex].bIsActive)
	{
		return;
	}

	FTimerEvent& CurrentEvent = Events[CurrentEventIndex];
	
	bool bSuccess = (CurrentArrowAngle >= CurrentEvent.SuccessStartAngle && CurrentArrowAngle <= CurrentEvent.SuccessEndAngle);
	CompleteCurrentEvent(bSuccess);
}

void UTimerMinigame::GenerateEvents()
{
	Events.SetNum(NumberOfEvents);
	if(NumberOfEvents == 0) return;

	float TimeBetweenEvents = BaseCookTime / (NumberOfEvents + 1);

	for (int32 i = 0; i < NumberOfEvents; ++i)
	{
		FTimerEvent& NewEvent = Events[i];
		NewEvent.TriggerTime = TimeBetweenEvents * (i + 1);
		
		float RandomStartAngle = FMath::FRandRange(0.0f, 360.0f - SuccessZoneSize);
		NewEvent.SuccessStartAngle = RandomStartAngle;
		NewEvent.SuccessEndAngle = RandomStartAngle + SuccessZoneSize;
		NewEvent.bIsActive = false;
		NewEvent.bIsCompleted = false;
	}
}

void UTimerMinigame::CompleteCurrentEvent(bool bSuccess)
{
	if (CurrentEventIndex >= Events.Num() || Events[CurrentEventIndex].bIsCompleted)
	{
		return;
	}

	FTimerEvent& CurrentEvent = Events[CurrentEventIndex];
	CurrentEvent.bIsActive = false;
	CurrentEvent.bIsCompleted = true;
	CurrentEvent.bSuccess = bSuccess;

	if (!bSuccess)
	{
		TotalCookTime += TimePenaltyOnFail;
	}
	
	OnTimerEventCompleted.Broadcast(bSuccess);

	// Move to next event
	CurrentEventIndex++;
} 