#include "Cooking/BoilingClickMinigame.h"
#include "UI/Inventory/CookingWidget.h"
#include "InteractablePot.h"
#include "Engine/Engine.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Math/UnrealMathUtility.h"

UBoilingClickMinigame::UBoilingClickMinigame()
{
    // 끓이기 클릭 게임 설정
    GameSettings.GameDuration = 30.0f;
    GameSettings.SuccessThreshold = 70.0f;
    GameSettings.PerfectThreshold = 90.0f;
    
    // 클릭 게임 설정
    TotalBoilingTime = 30.0f;
    TargetInterval = 4.0f; // 4초마다 타겟 생성
    ClickTimeLimit = 3.0f; // 3초 안에 클릭해야 함
    FailurePenaltyTime = 5.0f; // 실패 시 5초 추가
    
    // 시각적 설정
    TargetSize = 0.12f; // 화면의 12% 크기
    ClickTolerance = 1.0f; // 타겟 이미지 크기와 정확히 일치하는 판정 범위
    
    // 게임 상태 초기화
    CurrentTargetIndex = 0;
    SuccessfulClicks = 0;
    FailedClicks = 0;
    AddedCookingTime = 0.0f;
}

void UBoilingClickMinigame::StartMinigame(UCookingWidget* InWidget, AInteractablePot* InPot)
{
    Super::StartMinigame(InWidget, InPot);
    
    // 클릭 타겟 생성
    GenerateClickTargets();
    
    // 끓는 소리 재생
    if (BoilingSound.LoadSynchronous() && OwningPot.IsValid())
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), BoilingSound.Get(), OwningPot->GetActorLocation());
    }
    
    // UI 초기화
    UpdateUI();
    
    UE_LOG(LogTemp, Log, TEXT("UBoilingClickMinigame::StartMinigame - Started with %d click targets"), 
           ClickTargets.Num());
}

void UBoilingClickMinigame::UpdateMinigame(float DeltaTime)
{
    Super::UpdateMinigame(DeltaTime);
    
    if (!bIsGameActive)
    {
        return;
    }
    
    float ElapsedTime = GetWorld()->GetTimeSeconds() - GameStartTime;
    
    // 현재 타겟 확인 및 스폰
    if (CurrentTargetIndex < ClickTargets.Num())
    {
        FClickTarget& CurrentTarget = ClickTargets[CurrentTargetIndex];
        
        // 타겟을 스폰해야 하는 시간인지 확인
        if (ElapsedTime >= CurrentTarget.SpawnTime && !CurrentTarget.bIsActive)
        {
            SpawnTarget(CurrentTargetIndex);
        }
        
        // 활성화된 타겟의 시간 초과 확인
        if (CurrentTarget.bIsActive && !CurrentTarget.bWasClicked && !CurrentTarget.bTimedOut)
        {
            float TargetElapsedTime = ElapsedTime - CurrentTarget.SpawnTime;
            if (TargetElapsedTime >= CurrentTarget.TimeLimit)
            {
                HandleTargetTimeout(CurrentTargetIndex);
            }
        }
    }
    
    // 게임 종료 조건 확인
    if (ElapsedTime >= TotalBoilingTime + AddedCookingTime)
    {
        EndMinigame();
    }
}

void UBoilingClickMinigame::EndMinigame()
{
    // 최종 성과 계산
    float SuccessRate = (ClickTargets.Num() > 0) ? 
        (float(SuccessfulClicks) / float(ClickTargets.Num())) * 100.0f : 0.0f;
    
    CurrentScore = SuccessRate;
    
    UE_LOG(LogTemp, Log, TEXT("UBoilingClickMinigame::EndMinigame - Success Rate: %.1f%%, Added Time: %.1f seconds"), 
           SuccessRate, AddedCookingTime);
    
    // UI 업데이트
    if (OwningWidget.IsValid())
    {
        OwningWidget->OnMinigameEnded(FMath::RoundToInt(SuccessRate));
    }
    
    Super::EndMinigame();
}

void UBoilingClickMinigame::HandlePlayerInput(const FString& InputType)
{
    // 이 게임은 마우스 클릭만 사용, 키보드 입력은 무시
    return;
}

void UBoilingClickMinigame::HandleMouseClick(FVector2D ScreenPosition)
{
    UE_LOG(LogTemp, Warning, TEXT("UBoilingClickMinigame::HandleMouseClick - RECEIVED click at (%.3f, %.3f)"), 
           ScreenPosition.X, ScreenPosition.Y);

    if (!bIsGameActive)
    {
        UE_LOG(LogTemp, Warning, TEXT("UBoilingClickMinigame::HandleMouseClick - Game not active"));
        return;
    }
    
    if (CurrentTargetIndex >= ClickTargets.Num())
    {
        UE_LOG(LogTemp, Warning, TEXT("UBoilingClickMinigame::HandleMouseClick - No more targets (index %d >= %d)"), 
               CurrentTargetIndex, ClickTargets.Num());
        return;
    }
    
    FClickTarget& CurrentTarget = ClickTargets[CurrentTargetIndex];
    
    UE_LOG(LogTemp, Warning, TEXT("UBoilingClickMinigame::HandleMouseClick - Target %d: Active=%s, Clicked=%s, TimedOut=%s, Position=(%.3f, %.3f)"), 
           CurrentTargetIndex, 
           CurrentTarget.bIsActive ? TEXT("true") : TEXT("false"),
           CurrentTarget.bWasClicked ? TEXT("true") : TEXT("false"), 
           CurrentTarget.bTimedOut ? TEXT("true") : TEXT("false"),
           CurrentTarget.ScreenPosition.X, CurrentTarget.ScreenPosition.Y);
    
    // 현재 활성화된 타겟이 있는지 확인
    if (!CurrentTarget.bIsActive || CurrentTarget.bWasClicked || CurrentTarget.bTimedOut)
    {
        UE_LOG(LogTemp, Warning, TEXT("UBoilingClickMinigame::HandleMouseClick - Target not available"));
        return;
    }
    
    // 클릭 위치가 타겟 범위 내에 있는지 확인
    bool bWithinTarget = IsClickWithinTarget(ScreenPosition, CurrentTarget.ScreenPosition);
    UE_LOG(LogTemp, Warning, TEXT("UBoilingClickMinigame::HandleMouseClick - Within target: %s"), 
           bWithinTarget ? TEXT("YES") : TEXT("NO"));
    
    if (bWithinTarget)
    {
        UE_LOG(LogTemp, Warning, TEXT("UBoilingClickMinigame::HandleMouseClick - SUCCESS! Processing immediately"));
        HandleSuccessfulClick(CurrentTargetIndex);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UBoilingClickMinigame::HandleMouseClick - MISS! Processing immediately"));
        HandleFailedClick();
    }
}

void UBoilingClickMinigame::GenerateClickTargets()
{
    ClickTargets.Empty();
    
    // 게임 시간 동안 적절한 간격으로 타겟 생성
    int32 NumTargets = FMath::FloorToInt(TotalBoilingTime / TargetInterval);
    
    for (int32 i = 0; i < NumTargets; i++)
    {
        FClickTarget NewTarget;
        NewTarget.SpawnTime = (i + 1) * TargetInterval;
        NewTarget.ScreenPosition = GenerateRandomScreenPosition();
        NewTarget.TimeLimit = ClickTimeLimit;
        
        // 게임이 진행될수록 약간 빨라짐
        float DifficultyMultiplier = 1.0f - (i / float(NumTargets)) * 0.3f; // 최대 30% 빨라짐
        NewTarget.TimeLimit *= DifficultyMultiplier;
        
        ClickTargets.Add(NewTarget);
    }
    
    UE_LOG(LogTemp, Log, TEXT("UBoilingClickMinigame::GenerateClickTargets - Generated %d targets"), 
           ClickTargets.Num());
}

void UBoilingClickMinigame::SpawnTarget(int32 TargetIndex)
{
    if (TargetIndex >= ClickTargets.Num())
    {
        return;
    }
    
    FClickTarget& Target = ClickTargets[TargetIndex];
    Target.bIsActive = true;
    
    // 타겟 스폰 사운드 재생
    if (SpawnSound.LoadSynchronous() && OwningPot.IsValid())
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), SpawnSound.Get(), OwningPot->GetActorLocation());
    }
    
    // UI에 타겟 표시
    if (OwningWidget.IsValid())
    {
        OwningWidget->ShowClickTarget(Target.ScreenPosition, TargetSize, Target.TimeLimit);
    }
    
    UE_LOG(LogTemp, Log, TEXT("UBoilingClickMinigame::SpawnTarget - Spawned target %d at (%.2f, %.2f)"), 
           TargetIndex, Target.ScreenPosition.X, Target.ScreenPosition.Y);
}

void UBoilingClickMinigame::HandleTargetTimeout(int32 TargetIndex)
{
    if (TargetIndex >= ClickTargets.Num())
    {
        return;
    }
    
    FClickTarget& Target = ClickTargets[TargetIndex];
    Target.bTimedOut = true;
    Target.bIsActive = false;
    
    FailedClicks++;
    AddedCookingTime += FailurePenaltyTime;
    
    // 실패 사운드 재생
    PlayResultSound(false);
    
    // UI 업데이트
    if (OwningWidget.IsValid())
    {
        OwningWidget->HideClickTarget();
        OwningWidget->ShowClickResult(false, AddedCookingTime);
    }
    
    UpdateUI();
    
    UE_LOG(LogTemp, Warning, TEXT("UBoilingClickMinigame::HandleTargetTimeout - Target %d timed out! Added %.1f seconds"), 
           TargetIndex, FailurePenaltyTime);
    
    // 다음 타겟으로 이동
    CurrentTargetIndex++;
}

void UBoilingClickMinigame::HandleSuccessfulClick(int32 TargetIndex)
{
    if (TargetIndex >= ClickTargets.Num())
    {
        return;
    }
    
    FClickTarget& Target = ClickTargets[TargetIndex];
    Target.bWasClicked = true;
    Target.bIsActive = false;
    
    SuccessfulClicks++;
    
    // 성공 사운드 재생
    PlayResultSound(true);
    
    // UI 업데이트
    if (OwningWidget.IsValid())
    {
        OwningWidget->HideClickTarget();
        OwningWidget->ShowClickResult(true, 0.0f);
    }
    
    UpdateUI();
    
    UE_LOG(LogTemp, Log, TEXT("UBoilingClickMinigame::HandleSuccessfulClick - Target %d clicked successfully!"), 
           TargetIndex);
    
    // 다음 타겟으로 이동
    CurrentTargetIndex++;
}

void UBoilingClickMinigame::HandleFailedClick()
{
    UE_LOG(LogTemp, Warning, TEXT("UBoilingClickMinigame::HandleFailedClick - Missed click - showing immediate feedback"));
    
    // 빗나간 클릭에 대한 즉시 피드백 제공
    if (OwningWidget.IsValid())
    {
        // 현재 타겟은 그대로 두고, 빗나간 클릭에 대한 피드백만 표시
        OwningWidget->ShowClickResult(false, 0.0f); // 빗나간 클릭은 시간 추가 없음
    }
    
    // 빗나간 클릭은 특별한 패널티 없음 (타겟을 놓치는 것보다는 덜 심각)
    // 타겟은 계속 활성 상태로 유지되어 다시 클릭할 수 있음
}

FVector2D UBoilingClickMinigame::GenerateRandomScreenPosition() const
{
    // 화면 가장자리에서 충분히 떨어진 위치에 생성 (타겟 크기 고려)
    float Margin = TargetSize * 3.0f; // 더 큰 마진으로 설정
    
    // 0.2 ~ 0.8 범위로 제한하여 안전한 영역에만 생성
    float X = FMath::FRandRange(FMath::Max(0.2f, Margin), FMath::Min(0.8f, 1.0f - Margin));
    float Y = FMath::FRandRange(FMath::Max(0.2f, Margin), FMath::Min(0.8f, 1.0f - Margin));
    
    UE_LOG(LogTemp, Log, TEXT("GenerateRandomScreenPosition - Generated position: (%.3f, %.3f), Margin: %.3f"), 
           X, Y, Margin);
    
    return FVector2D(X, Y);
}

bool UBoilingClickMinigame::IsClickWithinTarget(FVector2D ClickPos, FVector2D TargetPos) const
{
    float Distance = FVector2D::Distance(ClickPos, TargetPos);
    
    // UI에서 표시되는 크기와 일치하도록 계산
    // UI DisplayScale이 6.0이므로, 실제 판정 범위도 그에 맞춰 조정
    float DisplayScale = 6.0f; // CookingWidget의 DisplayScale과 동일
    float AllowedDistance = (TargetSize * DisplayScale * ClickTolerance) / 2.0f / DisplayScale; // 반지름
    // 간단히 하면: AllowedDistance = (TargetSize * ClickTolerance) / 2.0f
    // 하지만 실제 화면 표시 크기를 고려하여 더 관대하게 설정
    AllowedDistance = TargetSize * 1.2f; // 타겟 크기보다 20% 큰 반지름 (관대한 판정)
    
    UE_LOG(LogTemp, Warning, TEXT("IsClickWithinTarget - Click: (%.3f, %.3f), Target: (%.3f, %.3f), Distance: %.3f, Allowed: %.3f"), 
           ClickPos.X, ClickPos.Y, TargetPos.X, TargetPos.Y, Distance, AllowedDistance);
    
    bool bWithinRange = Distance <= AllowedDistance;
    UE_LOG(LogTemp, Warning, TEXT("IsClickWithinTarget - Result: %s (TargetSize: %.3f, DisplayScale: %.1f)"), 
           bWithinRange ? TEXT("WITHIN") : TEXT("OUTSIDE"), TargetSize, DisplayScale);
    
    return bWithinRange;
}

void UBoilingClickMinigame::PlayResultSound(bool bSuccess)
{
    USoundBase* SoundToPlay = nullptr;
    
    if (bSuccess)
    {
        SoundToPlay = SuccessSound.LoadSynchronous();
    }
    else
    {
        SoundToPlay = FailSound.LoadSynchronous();
    }
    
    if (SoundToPlay && OwningPot.IsValid())
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), SoundToPlay, OwningPot->GetActorLocation());
    }
}

void UBoilingClickMinigame::UpdateUI()
{
    if (!OwningWidget.IsValid())
    {
        return;
    }
    
    // 게임 진행 상황 업데이트
    float Progress = (ClickTargets.Num() > 0) ? 
        (float(CurrentTargetIndex) / float(ClickTargets.Num())) * 100.0f : 0.0f;
    
    OwningWidget->UpdateClickGameStats(SuccessfulClicks, FailedClicks, AddedCookingTime, Progress);
} 