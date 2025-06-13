#include "Cooking/GrillingMinigame.h"
#include "UI/Minigame/GrillingMinigameWidget.h"
#include "Actors/InteractablePot.h"
#include "TimerManager.h"

UGrillingMinigame::UGrillingMinigame()
{
	PrimaryComponentTick.bCanEverTick = true;
	bIsGameActive = false;
}

void UGrillingMinigame::StartMinigame(TWeakObjectPtr<UUserWidget> InMinigameWidget, AInteractablePot* InPot)
{
	Super::StartMinigame(InMinigameWidget, InPot);
	GrillingWidget = Cast<UGrillingMinigameWidget>(InMinigameWidget.Get());
	if (!GrillingWidget.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("GrillingMinigame received an incompatible widget!"));
		return;
	}

	bIsGameActive = true;
	CurrentTemperature = 0.0f;
	CurrentScore = 0;
	bIsFlipping = false;

	SideCookingProgress.Init(FSideCookingProgress(), NumberOfSides);
	CurrentSideIndex = 0;
	
	if (GrillingWidget.IsValid())
	{
		GrillingWidget->UpdateRequiredAction(TEXT("HeatUp"), true);
		GrillingWidget->UpdateRequiredAction(TEXT("Flip"), true);
	}
}

void UGrillingMinigame::UpdateMinigame(float DeltaTime)
{
	Super::UpdateMinigame(DeltaTime);
	if (!bIsGameActive || bIsFlipping) return;

	CurrentTemperature -= PassiveHeatLoss * DeltaTime;
	CurrentTemperature = FMath::Max(0.0f, CurrentTemperature);

	if (CurrentTemperature >= OptimalTempMin && CurrentTemperature <= OptimalTempMax)
	{
		SideCookingProgress[CurrentSideIndex].CookedAmount += DeltaTime;
		UpdateSideDoneness(SideCookingProgress[CurrentSideIndex]);
		CurrentScore += OptimalTempScorePerSecond * DeltaTime;
	}

	if (GrillingWidget.IsValid())
	{
		GrillingWidget->UpdateMinigameWidget(CurrentScore, 0); 
	}
}

void UGrillingMinigame::EndMinigame()
{
	if (!bIsGameActive) return;

	bIsGameActive = false;
	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
	Super::EndMinigame();
}

void UGrillingMinigame::HandlePlayerInput(const FString& InputType)
{
	if (!bIsGameActive || bIsFlipping) return;

	if (InputType == TEXT("HeatUp"))
	{
		CurrentTemperature += HeatIncreaseAmount;
	}
	else if (InputType == TEXT("Flip"))
	{
		FlipItem();
	}
}

ECookingMinigameResult UGrillingMinigame::CalculateResult() const
{
	EGrillingDoneness FinalDoneness = CalculateFinalDoneness();
	switch(FinalDoneness)
	{
		case EGrillingDoneness::Medium:
		case EGrillingDoneness::WellDone:
			return ECookingMinigameResult::Good;
		case EGrillingDoneness::Rare:
			return ECookingMinigameResult::Average;
		case EGrillingDoneness::Burnt:
		case EGrillingDoneness::Raw:
		default:
			return ECookingMinigameResult::Failed;
	}
}

void UGrillingMinigame::FlipItem()
{
	if (bIsFlipping) return;

	bIsFlipping = true;
	if (GrillingWidget.IsValid())
	{
		GrillingWidget->ShowEventResult(TEXT("뒤집는 중..."));
	}
	
	GetWorld()->GetTimerManager().SetTimer(FlipTimerHandle, this, &UGrillingMinigame::StopFlip, FlipDuration, false);
}

void UGrillingMinigame::StopFlip()
{
	bIsFlipping = false;
	CurrentSideIndex = (CurrentSideIndex + 1) % NumberOfSides;
	
	if (GrillingWidget.IsValid())
	{
		GrillingWidget->ShowEventResult(FString::Printf(TEXT("%d면 익히는 중"), CurrentSideIndex + 1));
	}
}

void UGrillingMinigame::UpdateSideDoneness(FSideCookingProgress& Side)
{
    if (Side.CookedAmount > 15.f) Side.Doneness = EGrillingDoneness::Burnt;
    else if (Side.CookedAmount > 10.f) Side.Doneness = EGrillingDoneness::WellDone;
    else if (Side.CookedAmount > 6.f) Side.Doneness = EGrillingDoneness::Medium;
    else if (Side.CookedAmount > 3.f) Side.Doneness = EGrillingDoneness::Rare;
    else Side.Doneness = EGrillingDoneness::Raw;
}

EGrillingDoneness UGrillingMinigame::CalculateFinalDoneness() const
{
	float TotalCookedAmount = 0.f;
	for(const auto& Side : SideCookingProgress)
	{
		if(Side.Doneness == EGrillingDoneness::Burnt) return EGrillingDoneness::Burnt;
		TotalCookedAmount += Side.CookedAmount;
	}
	
	float AverageCookAmount = TotalCookedAmount / SideCookingProgress.Num();
	
	FSideCookingProgress TempSide;
	TempSide.CookedAmount = AverageCookAmount;
	
    if (TempSide.CookedAmount > 15.f) return EGrillingDoneness::Burnt;
    if (TempSide.CookedAmount > 10.f) return EGrillingDoneness::WellDone;
    if (TempSide.CookedAmount > 6.f) return EGrillingDoneness::Medium;
    if (TempSide.CookedAmount > 3.f) return EGrillingDoneness::Rare;
    
	return EGrillingDoneness::Raw;
} 