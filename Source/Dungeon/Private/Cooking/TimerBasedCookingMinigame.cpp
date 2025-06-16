#include "Cooking/TimerBasedCookingMinigame.h"
#include "UI/Inventory/CookingWidget.h"
#include "InteractablePot.h"
#include "Engine/Engine.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Kismet/KismetMathLibrary.h"

UTimerBasedCookingMinigame::UTimerBasedCookingMinigame()
{
    // 기본 설정
    TotalCookingTime = 20.0f;
    RemainingTime = 20.0f;
    TimePenalty = 3.0f;
    
    // 이벤트 발생 간격 설정
    MinEventInterval = 1.0f;
    MaxEventInterval = 4.0f;
    
    // 성공 구간 크기 설정
    MinSuccessArcSize = 30.0f;
    MaxSuccessArcSize = 90.0f;
    
    // 화살표 속도 설정
    MinArrowSpeed = 120.0f;
    MaxArrowSpeed = 240.0f;
    
    // UI 설정
    UIRotationOffset = -90.0f;
    SuccessZoneTolerance = 5.0f;
    EventTimeoutBuffer = 1.0f;
    
    // 게임 상태 초기화
    SuccessfulEvents = 0;
    FailedEvents = 0;
    TimeToNextEvent = 0.0f;
    
    // 현재 이벤트 초기화
    CurrentEvent = FCircularTimingEvent();
    
    // 베이스 클래스 설정
    GameSettings.GameDuration = TotalCookingTime;
    GameSettings.SuccessThreshold = 60.0f;
    GameSettings.PerfectThreshold = 90.0f;
}

void UTimerBasedCookingMinigame::StartMinigame(UCookingWidget* InWidget, AInteractablePot* InPot)
{
    Super::StartMinigame(InWidget, InPot);
    
    // 게임 상태 초기화
    RemainingTime = TotalCookingTime;
    SuccessfulEvents = 0;
    FailedEvents = 0;
    CurrentScore = 0.0f;
    
    // 첫 번째 이벤트 스케줄링
    ScheduleNextEvent();
    
    // UI 초기화
    if (OwningWidget.IsValid())
    {
        OwningWidget->StartTimerBasedMinigame(TotalCookingTime);
    }
    
    // 백그라운드 음악 재생
    if (BackgroundMusic.LoadSynchronous() && OwningPot.IsValid())
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), BackgroundMusic.Get(), OwningPot->GetActorLocation());
    }
    
    UE_LOG(LogTemp, Log, TEXT("UTimerBasedCookingMinigame::StartMinigame - Started with %.1f seconds"), TotalCookingTime);
}

void UTimerBasedCookingMinigame::UpdateMinigame(float DeltaTime)
{
    Super::UpdateMinigame(DeltaTime);
    
    if (!bIsGameActive)
    {
        return;
    }
    
    // 메인 타이머 업데이트
    UpdateMainTimer(DeltaTime);
    
    // 현재 이벤트 업데이트
    UpdateCurrentEvent(DeltaTime);
    
    // 다음 이벤트 스케줄링 확인
    if (!CurrentEvent.bIsActive && TimeToNextEvent > 0.0f)
    {
        TimeToNextEvent -= DeltaTime;
        if (TimeToNextEvent <= 0.0f)
        {
            GenerateRandomEvent();
        }
    }
    
    // UI 업데이트
    if (OwningWidget.IsValid())
    {
        // 타이머 진행률 업데이트
        float TimerProgress = GetTimerProgress();
        OwningWidget->OnMinigameUpdated(CurrentScore, (int32)CurrentPhase);
        OwningWidget->UpdateMainTimer(TimerProgress, RemainingTime);
        
        // 원형 이벤트 UI 업데이트
        if (CurrentEvent.bIsActive)
        {
            OwningWidget->UpdateCircularEvent(CurrentEvent.CurrentArrowAngle);
        }
    }
    
    // 게임 종료 조건 확인
    if (RemainingTime <= 0.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("UTimerBasedCookingMinigame::UpdateMinigame - GAME ENDING! RemainingTime: %.2f"), RemainingTime);
        EndMinigame();
    }
}

void UTimerBasedCookingMinigame::EndMinigame()
{
    // 타이머 완료 사운드 재생
    if (TimerCompleteSound.LoadSynchronous() && OwningPot.IsValid())
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), TimerCompleteSound.Get(), OwningPot->GetActorLocation());
    }
    
    // 현재 이벤트 정리
    if (CurrentEvent.bIsActive)
    {
        CompleteCurrentEvent(ECircularEventResult::Timeout);
    }
    
    // 최종 점수 계산
    float SuccessRate = (SuccessfulEvents > 0) ? (float)SuccessfulEvents / (float)(SuccessfulEvents + FailedEvents) : 0.0f;
    CurrentScore = SuccessRate * 100.0f;
    
    UE_LOG(LogTemp, Log, TEXT("UTimerBasedCookingMinigame::EndMinigame - Final Score: %.2f, Success: %d, Failed: %d"), 
           CurrentScore, SuccessfulEvents, FailedEvents);
    
    Super::EndMinigame();
}

void UTimerBasedCookingMinigame::HandlePlayerInput(const FString& InputType)
{
    if (!bIsGameActive || !CurrentEvent.bIsActive)
    {
        return;
    }
    
    // 입력 키 확인 (예: "ActionButton", "SpaceBar" 등)
    if (InputType == TEXT("ActionButton") || InputType == TEXT("SpaceBar"))
    {
        // 화살표가 성공 구간에 있는지 확인
        if (IsArrowInSuccessZone())
        {
            CompleteCurrentEvent(ECircularEventResult::Success);
        }
        else
        {
            CompleteCurrentEvent(ECircularEventResult::Failed);
        }
    }
}

float UTimerBasedCookingMinigame::GetTimerProgress() const
{
    if (TotalCookingTime <= 0.0f)
    {
        return 1.0f;
    }
    
    return FMath::Clamp(1.0f - (RemainingTime / TotalCookingTime), 0.0f, 1.0f);
}

void UTimerBasedCookingMinigame::GenerateRandomEvent()
{
    // 이전 이벤트가 아직 활성화되어 있다면 무시
    if (CurrentEvent.bIsActive)
    {
        return;
    }
    
    // 새로운 이벤트 생성
    CurrentEvent = FCircularTimingEvent();
    
    // 성공 구간 크기 랜덤 결정
    float ArcSize = FMath::RandRange(MinSuccessArcSize, MaxSuccessArcSize);
    
    // 성공 구간 시작 위치 랜덤 결정 (0-360도 범위)
    float StartAngle = FMath::RandRange(0.0f, 360.0f - ArcSize);
    
    // 이벤트 설정
    CurrentEvent.SuccessArcStartAngle = StartAngle;
    CurrentEvent.SuccessArcEndAngle = StartAngle + ArcSize;
    CurrentEvent.ArrowRotationSpeed = FMath::RandRange(MinArrowSpeed, MaxArrowSpeed);
    CurrentEvent.CurrentArrowAngle = 0.0f; // 12시에서 시작
    CurrentEvent.bIsActive = true;
    CurrentEvent.bIsCompleted = false;
    CurrentEvent.EventStartTime = GetWorld()->GetTimeSeconds() - GameStartTime;
    CurrentEvent.Result = ECircularEventResult::None;
    
    // 화살표 속도에 따라 동적으로 타임아웃 설정 (360도 완주 시간 + 여유시간)
    float TimeFor360Degrees = 360.0f / CurrentEvent.ArrowRotationSpeed;
    CurrentEvent.MaxEventDuration = TimeFor360Degrees + EventTimeoutBuffer;
    
    // 이벤트 시작 사운드 재생
    PlayEventSound(EventStartSound);
    
    // UI에 원형 이벤트 시작 알림
    if (OwningWidget.IsValid())
    {
        OwningWidget->StartCircularEvent(StartAngle, StartAngle + ArcSize, CurrentEvent.ArrowRotationSpeed);
    }
    
    UE_LOG(LogTemp, Log, TEXT("UTimerBasedCookingMinigame::GenerateRandomEvent - Arc: %.1f-%.1f degrees, Speed: %.1f deg/s, Timeout: %.1f s"), 
           StartAngle, StartAngle + ArcSize, CurrentEvent.ArrowRotationSpeed, CurrentEvent.MaxEventDuration);
}

bool UTimerBasedCookingMinigame::IsArrowInSuccessZone() const
{
    if (!CurrentEvent.bIsActive)
    {
        return false;
    }
    
    float NormalizedArrowAngle = NormalizeAngle(CurrentEvent.CurrentArrowAngle);
    float NormalizedStartAngle = NormalizeAngle(CurrentEvent.SuccessArcStartAngle);
    float NormalizedEndAngle = NormalizeAngle(CurrentEvent.SuccessArcEndAngle);
    
    // 허용 오차를 적용하여 성공 구간을 확장
    float ExpandedStartAngle = NormalizeAngle(NormalizedStartAngle - SuccessZoneTolerance);
    float ExpandedEndAngle = NormalizeAngle(NormalizedEndAngle + SuccessZoneTolerance);
    
    UE_LOG(LogTemp, Log, TEXT("UTimerBasedCookingMinigame::IsArrowInSuccessZone - Arrow: %.1f, Zone: %.1f-%.1f (Expanded: %.1f-%.1f)"), 
           NormalizedArrowAngle, NormalizedStartAngle, NormalizedEndAngle, ExpandedStartAngle, ExpandedEndAngle);
    
    // 성공 구간이 0도를 넘나드는 경우 처리
    if (ExpandedEndAngle < ExpandedStartAngle)
    {
        bool bInZone = (NormalizedArrowAngle >= ExpandedStartAngle) || (NormalizedArrowAngle <= ExpandedEndAngle);
        UE_LOG(LogTemp, Log, TEXT("UTimerBasedCookingMinigame::IsArrowInSuccessZone - Cross-zero case: %s"), bInZone ? TEXT("SUCCESS") : TEXT("FAIL"));
        return bInZone;
    }
    else
    {
        bool bInZone = (NormalizedArrowAngle >= ExpandedStartAngle) && (NormalizedArrowAngle <= ExpandedEndAngle);
        UE_LOG(LogTemp, Log, TEXT("UTimerBasedCookingMinigame::IsArrowInSuccessZone - Normal case: %s"), bInZone ? TEXT("SUCCESS") : TEXT("FAIL"));
        return bInZone;
    }
}

void UTimerBasedCookingMinigame::CompleteCurrentEvent(ECircularEventResult Result)
{
    if (!CurrentEvent.bIsActive || CurrentEvent.bIsCompleted)
    {
        return;
    }
    
    CurrentEvent.bIsCompleted = true;
    CurrentEvent.Result = Result;
    CurrentEvent.bIsActive = false;
    
    // UI에 이벤트 완료 알림
    if (OwningWidget.IsValid())
    {
        FString ResultString;
        switch (Result)
        {
            case ECircularEventResult::Success:
                ResultString = TEXT("Success");
                break;
            case ECircularEventResult::Failed:
                ResultString = TEXT("Failed");
                break;
            case ECircularEventResult::Timeout:
                ResultString = TEXT("Timeout");
                break;
        }
        OwningWidget->EndCircularEvent(ResultString);
    }
    
    switch (Result)
    {
        case ECircularEventResult::Success:
            SuccessfulEvents++;
            AddScore(100.0f);
            PlayEventSound(SuccessSound);
            UE_LOG(LogTemp, Log, TEXT("UTimerBasedCookingMinigame::CompleteCurrentEvent - SUCCESS! Total: %d"), SuccessfulEvents);
            break;
            
        case ECircularEventResult::Failed:
            FailedEvents++;
            AddScore(-50.0f);
            ApplyTimePenalty();
            PlayEventSound(FailSound);
            UE_LOG(LogTemp, Log, TEXT("UTimerBasedCookingMinigame::CompleteCurrentEvent - FAILED! Total: %d, Time penalty applied"), FailedEvents);
            break;
            
        case ECircularEventResult::Timeout:
            FailedEvents++;
            AddScore(-25.0f);
            ApplyTimePenalty();
            PlayEventSound(FailSound);
            UE_LOG(LogTemp, Log, TEXT("UTimerBasedCookingMinigame::CompleteCurrentEvent - TIMEOUT! Total: %d"), FailedEvents);
            break;
    }
    
    // 다음 이벤트 스케줄링
    ScheduleNextEvent();
}

void UTimerBasedCookingMinigame::ApplyTimePenalty()
{
    // 페널티: 총 요리 시간을 늘려서 더 오래 기다리게 만듦
    TotalCookingTime += TimePenalty;
    RemainingTime += TimePenalty;
    
    UE_LOG(LogTemp, Warning, TEXT("UTimerBasedCookingMinigame::ApplyTimePenalty - Added %.1f seconds penalty, Remaining: %.1f, Total: %.1f"), 
           TimePenalty, RemainingTime, TotalCookingTime);
}

void UTimerBasedCookingMinigame::UpdateMainTimer(float DeltaTime)
{
    float PreviousTime = RemainingTime;
    RemainingTime -= DeltaTime;
    RemainingTime = FMath::Max(RemainingTime, 0.0f);
    
    // 시간 변화 로그 (1초마다)
    static float LastLogTime = 0.0f;
    static float LogTimer = 0.0f;
    LogTimer += DeltaTime;
    if (LogTimer >= 1.0f)
    {
        UE_LOG(LogTemp, Log, TEXT("UTimerBasedCookingMinigame::UpdateMainTimer - Remaining: %.2f / %.2f (%.1f%%)"), 
               RemainingTime, TotalCookingTime, (RemainingTime / TotalCookingTime) * 100.0f);
        LogTimer = 0.0f;
    }
}

void UTimerBasedCookingMinigame::UpdateCurrentEvent(float DeltaTime)
{
    if (!CurrentEvent.bIsActive || CurrentEvent.bIsCompleted)
    {
        return;
    }
    
    // 화살표 회전 업데이트
    CurrentEvent.CurrentArrowAngle += CurrentEvent.ArrowRotationSpeed * DeltaTime;
    CurrentEvent.CurrentArrowAngle = NormalizeAngle(CurrentEvent.CurrentArrowAngle);
    
    // 이벤트 타임아웃 확인 (게임 시작 기준 상대 시간 사용)
    float CurrentGameTime = GetWorld()->GetTimeSeconds() - GameStartTime;
    float ElapsedTime = CurrentGameTime - CurrentEvent.EventStartTime;
    
    // 디버그 로그 추가
    static float LastLogTime = 0.0f;
    if (ElapsedTime - LastLogTime >= 0.5f) // 0.5초마다 로그
    {
        UE_LOG(LogTemp, Log, TEXT("UTimerBasedCookingMinigame::UpdateCurrentEvent - Elapsed: %.2f / %.2f, Arrow: %.1f"), 
               ElapsedTime, CurrentEvent.MaxEventDuration, CurrentEvent.CurrentArrowAngle);
        LastLogTime = ElapsedTime;
    }
    
    if (ElapsedTime >= CurrentEvent.MaxEventDuration)
    {
        UE_LOG(LogTemp, Warning, TEXT("UTimerBasedCookingMinigame::UpdateCurrentEvent - EVENT TIMEOUT! Elapsed: %.2f >= Max: %.2f"), 
               ElapsedTime, CurrentEvent.MaxEventDuration);
        CompleteCurrentEvent(ECircularEventResult::Timeout);
    }
}

void UTimerBasedCookingMinigame::ScheduleNextEvent()
{
    // 게임이 종료되었으면 더 이상 이벤트 생성하지 않음
    if (RemainingTime <= 0.0f)
    {
        return;
    }
    
    TimeToNextEvent = FMath::RandRange(MinEventInterval, MaxEventInterval);
    UE_LOG(LogTemp, Log, TEXT("UTimerBasedCookingMinigame::ScheduleNextEvent - Next event in %.1f seconds"), TimeToNextEvent);
}

void UTimerBasedCookingMinigame::PlayEventSound(TSoftObjectPtr<USoundBase> Sound)
{
    if (Sound.LoadSynchronous() && OwningPot.IsValid())
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), Sound.Get(), OwningPot->GetActorLocation());
    }
}

float UTimerBasedCookingMinigame::NormalizeAngle(float Angle) const
{
    while (Angle < 0.0f)
    {
        Angle += 360.0f;
    }
    while (Angle >= 360.0f)
    {
        Angle -= 360.0f;
    }
    return Angle;
} 