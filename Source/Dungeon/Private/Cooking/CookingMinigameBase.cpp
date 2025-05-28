#include "Cooking/CookingMinigameBase.h"
#include "UI/Inventory/CookingWidget.h"
#include "InteractablePot.h"
#include "Engine/Engine.h"

UCookingMinigameBase::UCookingMinigameBase()
{
    // 기본 설정
    GameSettings.GameDuration = 10.0f;
    GameSettings.SuccessThreshold = 60.0f;
    GameSettings.PerfectThreshold = 90.0f;
    GameSettings.EventInterval = 2.0f;
    GameSettings.ReactionTimeLimit = 1.5f;
}

void UCookingMinigameBase::StartMinigame(UCookingWidget* InWidget, AInteractablePot* InPot)
{
    if (!InWidget || !InPot)
    {
        UE_LOG(LogTemp, Error, TEXT("UCookingMinigameBase::StartMinigame - Invalid widget or pot"));
        return;
    }

    OwningWidget = InWidget;
    OwningPot = InPot;
    
    CurrentScore = 0.0f;
    CurrentPhase = ECookingMinigamePhase::Preparation;
    bIsGameActive = true;
    GameStartTime = GetWorld()->GetTimeSeconds();

    UE_LOG(LogTemp, Log, TEXT("UCookingMinigameBase::StartMinigame - Game started"));
}

void UCookingMinigameBase::UpdateMinigame(float DeltaTime)
{
    if (!bIsGameActive)
    {
        return;
    }

    float ElapsedTime = GetWorld()->GetTimeSeconds() - GameStartTime;
    
    // 게임 시간 초과 체크
    if (ElapsedTime >= GameSettings.GameDuration)
    {
        EndMinigame();
        return;
    }

    // 하위 클래스에서 구체적인 업데이트 로직 구현
}

void UCookingMinigameBase::EndMinigame()
{
    if (!bIsGameActive)
    {
        return;
    }

    bIsGameActive = false;
    ECookingMinigameResult Result = CalculateResult();
    
    UE_LOG(LogTemp, Log, TEXT("UCookingMinigameBase::EndMinigame - Final Score: %.2f, Result: %d"), 
           CurrentScore, (int32)Result);

    NotifyGameEnd(Result);
}

void UCookingMinigameBase::HandlePlayerInput(const FString& InputType)
{
    if (!bIsGameActive)
    {
        return;
    }

    // 하위 클래스에서 구체적인 입력 처리 구현
    UE_LOG(LogTemp, Log, TEXT("UCookingMinigameBase::HandlePlayerInput - Input: %s"), *InputType);
}

ECookingMinigameResult UCookingMinigameBase::CalculateResult() const
{
    if (CurrentScore >= GameSettings.PerfectThreshold)
    {
        return ECookingMinigameResult::Perfect;
    }
    else if (CurrentScore >= GameSettings.SuccessThreshold + 20.0f)
    {
        return ECookingMinigameResult::Good;
    }
    else if (CurrentScore >= GameSettings.SuccessThreshold)
    {
        return ECookingMinigameResult::Average;
    }
    else if (CurrentScore >= GameSettings.SuccessThreshold * 0.5f)
    {
        return ECookingMinigameResult::Poor;
    }
    else
    {
        return ECookingMinigameResult::Failed;
    }
}

void UCookingMinigameBase::AddScore(float Points)
{
    CurrentScore = FMath::Max(0.0f, CurrentScore + Points);
    UE_LOG(LogTemp, Log, TEXT("UCookingMinigameBase::AddScore - Added %.2f points, Total: %.2f"), 
           Points, CurrentScore);
}

void UCookingMinigameBase::AdvancePhase()
{
    switch (CurrentPhase)
    {
    case ECookingMinigamePhase::Preparation:
        CurrentPhase = ECookingMinigamePhase::Cooking;
        break;
    case ECookingMinigamePhase::Cooking:
        CurrentPhase = ECookingMinigamePhase::Timing;
        break;
    case ECookingMinigamePhase::Timing:
        CurrentPhase = ECookingMinigamePhase::Finishing;
        break;
    case ECookingMinigamePhase::Finishing:
        EndMinigame();
        break;
    }

    UE_LOG(LogTemp, Log, TEXT("UCookingMinigameBase::AdvancePhase - Phase: %d"), (int32)CurrentPhase);
}

void UCookingMinigameBase::NotifyGameEnd(ECookingMinigameResult Result)
{
    if (OwningPot.IsValid())
    {
        OwningPot->EndCookingMinigame(Result);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UCookingMinigameBase::NotifyGameEnd - OwningPot is invalid"));
    }
} 