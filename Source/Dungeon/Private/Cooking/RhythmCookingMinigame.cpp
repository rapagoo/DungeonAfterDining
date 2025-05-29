#include "Cooking/RhythmCookingMinigame.h"
#include "UI/Inventory/CookingWidget.h"
#include "InteractablePot.h"
#include "Engine/Engine.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"

URhythmCookingMinigame::URhythmCookingMinigame()
{
    // 리듬 게임 전용 설정
    GameSettings.GameDuration = 12.0f;
    GameSettings.SuccessThreshold = 75.0f;
    GameSettings.PerfectThreshold = 95.0f;
    
    CurrentEventIndex = 0;
    ComboMultiplier = 1;
    CurrentCombo = 0;
    
    // 점수 설정
    PerfectScore = 100.0f;
    GoodScore = 75.0f;
    HitScore = 50.0f;
}

void URhythmCookingMinigame::StartMinigame(UCookingWidget* InWidget, AInteractablePot* InPot)
{
    Super::StartMinigame(InWidget, InPot);
    
    // 리듬 이벤트 생성 (임시로 간단한 패턴)
    GenerateRhythmEvents(TEXT("Default"), GameSettings.GameDuration);
    
    // 백그라운드 음악 재생
    if (BackgroundMusic.LoadSynchronous() && OwningPot.IsValid())
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), BackgroundMusic.Get(), OwningPot->GetActorLocation());
    }
    
    UE_LOG(LogTemp, Warning, TEXT("🎮 URhythmCookingMinigame::StartMinigame - Rhythm game started!"));
    UE_LOG(LogTemp, Warning, TEXT("🎮 Total events: %d, Game duration: %.2f seconds"), 
           RhythmEvents.Num(), GameSettings.GameDuration);
    UE_LOG(LogTemp, Warning, TEXT("🎮 First event at: %.2f seconds"), 
           RhythmEvents.Num() > 0 ? RhythmEvents[0].TriggerTime : -1.0f);
}

void URhythmCookingMinigame::UpdateMinigame(float DeltaTime)
{
    Super::UpdateMinigame(DeltaTime);
    
    if (!bIsGameActive)
    {
        return;
    }
    
    float CurrentTime = GetWorld()->GetTimeSeconds() - GameStartTime;
    
    // 현재 이벤트 체크
    if (CurrentEventIndex < RhythmEvents.Num())
    {
        FRhythmEvent& CurrentEvent = RhythmEvents[CurrentEventIndex];
        float TimeToEvent = CurrentEvent.TriggerTime - CurrentTime;
        
        // 이벤트 시간이 지났는데 아직 처리되지 않았다면 자동으로 Miss 처리
        if (TimeToEvent <= -CurrentEvent.SuccessWindow && !CurrentEvent.bProcessed)
        {
            UE_LOG(LogTemp, Warning, TEXT("🎮 URhythmCookingMinigame - Event %d AUTO-MISSED (timeout). Event time: %.2f, Current time: %.2f"), 
                   CurrentEventIndex, CurrentEvent.TriggerTime, CurrentTime);
            CurrentEvent.bProcessed = true;
            ResetCombo();
            // 자동 Miss에 대한 점수 차감
            AddScore(-15.0f);
            CurrentEventIndex++;
        }
        
        // UI 업데이트 - 타이밍 윈도우 내에 있는지 확인
        if (OwningWidget.IsValid())
        {
            int32 PhaseToReport = 1; // 기본값: Cooking phase
            
            // 타이밍 윈도우에 진입했는지 확인 (이벤트 시간 ±Success Window)
            if (TimeToEvent <= CurrentEvent.SuccessWindow && TimeToEvent >= -CurrentEvent.SuccessWindow && !CurrentEvent.bProcessed)
            {
                PhaseToReport = 2; // Timing phase - 이때만 입력 받기!
                UE_LOG(LogTemp, Warning, TEXT("🎮 URhythmCookingMinigame - TIMING WINDOW ACTIVE! Event %d (%.2fs to perfect, %.2fs to event)"), 
                       CurrentEventIndex, FMath::Abs(TimeToEvent), TimeToEvent);
            }
            else if (TimeToEvent > CurrentEvent.SuccessWindow)
            {
                PhaseToReport = 1; // 아직 타이밍이 아님
                // 주기적으로 다음 이벤트까지의 시간 로그 (매 2초마다)
                static float LastLogTime = 0.0f;
                if (CurrentTime - LastLogTime > 2.0f)
                {
                    UE_LOG(LogTemp, Log, TEXT("🎮 URhythmCookingMinigame - Next event %d in %.2f seconds"), 
                           CurrentEventIndex, TimeToEvent);
                    LastLogTime = CurrentTime;
                }
            }
            else if (CurrentEvent.bProcessed)
            {
                PhaseToReport = 3; // 이벤트 완료, 다음 대기
            }
            
            OwningWidget->OnMinigameUpdated(CurrentScore, PhaseToReport);
        }
    }
    else
    {
        // 모든 이벤트가 처리되었는지 체크
        UE_LOG(LogTemp, Warning, TEXT("🎮 URhythmCookingMinigame - All events processed, ending game"));
        EndMinigame();
    }
}

void URhythmCookingMinigame::EndMinigame()
{
    UE_LOG(LogTemp, Log, TEXT("URhythmCookingMinigame::EndMinigame - Final Score: %.2f, Combo: %d"), 
           CurrentScore, CurrentCombo);
    
    Super::EndMinigame();
}

void URhythmCookingMinigame::HandlePlayerInput(const FString& InputType)
{
    Super::HandlePlayerInput(InputType);
    
    if (!bIsGameActive)
    {
        return;
    }
    
    // 리듬 타이밍 체크
    if (InputType == TEXT("Flip"))
    {
        float CurrentTime = GetWorld()->GetTimeSeconds() - GameStartTime;
        bool bFoundValidEvent = false;
        
        // 현재 활성 이벤트가 있는지 확인
        if (CurrentEventIndex < RhythmEvents.Num())
        {
            FRhythmEvent& CurrentEvent = RhythmEvents[CurrentEventIndex];
            
            // 이벤트가 아직 처리되지 않았는지 확인
            if (!CurrentEvent.bProcessed)
            {
                ERhythmEventResult Result = JudgeEventTiming(CurrentEvent, CurrentTime);
                
                // 타이밍 윈도우 내에 있는지 확인
                if (Result != ERhythmEventResult::Miss)
                {
                    bFoundValidEvent = true;
                    float ScoreToAdd = CalculateEventScore(Result);
                    
                    // 결과에 따른 처리
                    switch (Result)
                    {
                    case ERhythmEventResult::Perfect:
                        UE_LOG(LogTemp, Log, TEXT("URhythmCookingMinigame - PERFECT HIT! Score: +%.0f"), ScoreToAdd);
                        IncrementCombo();
                        break;
                    case ERhythmEventResult::Good:
                        UE_LOG(LogTemp, Log, TEXT("URhythmCookingMinigame - GOOD HIT! Score: +%.0f"), ScoreToAdd);
                        IncrementCombo();
                        break;
                    case ERhythmEventResult::Hit:
                        UE_LOG(LogTemp, Log, TEXT("URhythmCookingMinigame - HIT! Score: +%.0f"), ScoreToAdd);
                        IncrementCombo();
                        break;
                    default:
                        break;
                    }
                    
                    AddScore(ScoreToAdd);
                    CurrentEvent.bProcessed = true;
                    CurrentEventIndex++;
                    
                    // 성공 사운드 재생
                    if (SuccessSound.LoadSynchronous() && OwningPot.IsValid())
                    {
                        UGameplayStatics::PlaySoundAtLocation(GetWorld(), SuccessSound.Get(), OwningPot->GetActorLocation());
                    }
                }
            }
        }
        
        // 유효한 타이밍 윈도우가 아닌 경우 (연타 방지 및 페널티)
        if (!bFoundValidEvent)
        {
            UE_LOG(LogTemp, Warning, TEXT("URhythmCookingMinigame - BAD TIMING! No active event or outside timing window"));
            
            // 점수 차감 (연타 방지)
            float PenaltyScore = 25.0f;
            AddScore(-PenaltyScore);
            ResetCombo();
            
            // 실패 사운드나 피드백 (있다면)
            UE_LOG(LogTemp, Warning, TEXT("URhythmCookingMinigame - Score penalty: -%.0f"), PenaltyScore);
        }
    }
}

void URhythmCookingMinigame::AddScore(float Points)
{
    // 점수가 음수로 가지 않도록 제한
    float NewScore = CurrentScore + Points;
    CurrentScore = FMath::Max(0.0f, NewScore);
    
    UE_LOG(LogTemp, Log, TEXT("URhythmCookingMinigame::AddScore - Added: %.2f, Total: %.2f"), Points, CurrentScore);
}

ECookingMinigameResult URhythmCookingMinigame::CalculateResult() const
{
    // 완료한 이벤트 수와 전체 이벤트 수 고려
    int32 TotalEvents = RhythmEvents.Num();
    int32 CompletedEvents = CurrentEventIndex; // 처리된 이벤트 수
    
    // 완료율 계산
    float CompletionRate = TotalEvents > 0 ? (float)CompletedEvents / (float)TotalEvents : 0.0f;
    
    // 콤보 보너스를 포함한 최종 점수 계산
    float FinalScore = CurrentScore + (CurrentCombo * 5.0f); // 콤보 보너스 감소
    
    // 완료율이 낮으면 등급 하락
    if (CompletionRate < 0.5f)
    {
        UE_LOG(LogTemp, Warning, TEXT("URhythmCookingMinigame::CalculateResult - Low completion rate: %.2f"), CompletionRate);
        return ECookingMinigameResult::Failed;
    }
    
    // 더 엄격한 기준 적용
    if (FinalScore >= 400.0f && CompletionRate >= 0.9f) // Perfect: 높은 점수 + 높은 완료율
        return ECookingMinigameResult::Perfect;
    else if (FinalScore >= 250.0f && CompletionRate >= 0.8f) // Good: 괜찮은 점수 + 괜찮은 완료율
        return ECookingMinigameResult::Good;
    else if (FinalScore >= 150.0f && CompletionRate >= 0.6f) // Average: 기본 점수 + 기본 완료율
        return ECookingMinigameResult::Average;
    else if (FinalScore >= 50.0f && CompletionRate >= 0.5f) // Poor: 낮은 점수지만 최소 완료율
        return ECookingMinigameResult::Poor;
    else
        return ECookingMinigameResult::Failed;
}

void URhythmCookingMinigame::GenerateRhythmEvents(const FString& CookingMethodName, float GameDuration)
{
    RhythmEvents.Empty();
    
    // 난이도에 따른 리듬 패턴 생성
    TArray<float> BeatPattern;
    
    if (CookingMethodName == TEXT("Default") || CookingMethodName == TEXT("Rhythm"))
    {
        // 기본 리듬 패턴: 간단한 4/4 박자
        BeatPattern = {1.5f, 3.0f, 4.5f, 6.0f, 7.5f, 9.0f, 10.5f};
    }
    else if (CookingMethodName == TEXT("Frying"))
    {
        // 튀김용 패턴: 빠른 연속 비트
        BeatPattern = {2.0f, 2.8f, 3.6f, 5.0f, 5.8f, 6.6f, 8.0f, 9.0f, 10.0f};
    }
    else if (CookingMethodName == TEXT("Boiling"))
    {
        // 끓이기용 패턴: 느린 안정적인 비트
        BeatPattern = {2.0f, 4.5f, 7.0f, 9.5f};
    }
    else
    {
        // 기본 패턴 사용
        BeatPattern = {1.5f, 3.0f, 4.5f, 6.0f, 7.5f, 9.0f, 10.5f};
    }
    
    // 게임 시간에 맞춰 패턴 조정
    for (float BeatTime : BeatPattern)
    {
        if (BeatTime < GameDuration - 1.0f) // 마지막 1초 전까지만
        {
            FRhythmEvent NewEvent;
            NewEvent.TriggerTime = BeatTime;
            NewEvent.EventType = TEXT("Beat");
            
            // 더 엄격한 타이밍 윈도우
            NewEvent.PerfectWindow = 0.15f;  // 0.3초 -> 0.15초로 축소
            NewEvent.GoodWindow = 0.25f;     // 0.5초 -> 0.25초로 축소
            NewEvent.SuccessWindow = 0.4f;   // 0.8초 -> 0.4초로 축소
            NewEvent.bProcessed = false;
            
            RhythmEvents.Add(NewEvent);
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("URhythmCookingMinigame::GenerateRhythmEvents - Generated %d events for %s with stricter timing"), 
           RhythmEvents.Num(), *CookingMethodName);
           
    // 각 이벤트 타이밍 로그 출력
    for (int32 i = 0; i < RhythmEvents.Num(); i++)
    {
        UE_LOG(LogTemp, Log, TEXT("  Event %d: Time %.2fs (Perfect: ±%.2fs, Good: ±%.2fs, Hit: ±%.2fs)"), 
               i, RhythmEvents[i].TriggerTime, RhythmEvents[i].PerfectWindow, 
               RhythmEvents[i].GoodWindow, RhythmEvents[i].SuccessWindow);
    }
}

FRhythmEvent URhythmCookingMinigame::GetCurrentEvent() const
{
    if (CurrentEventIndex < RhythmEvents.Num())
    {
        return RhythmEvents[CurrentEventIndex];
    }
    return FRhythmEvent();
}

float URhythmCookingMinigame::GetTimeToNextEvent() const
{
    if (CurrentEventIndex < RhythmEvents.Num())
    {
        float CurrentTime = GetWorld()->GetTimeSeconds() - GameStartTime;
        return RhythmEvents[CurrentEventIndex].TriggerTime - CurrentTime;
    }
    return -1.0f;
}

ERhythmEventResult URhythmCookingMinigame::JudgeEventTiming(const FRhythmEvent& Event, float InputTime)
{
    float EventTime = Event.TriggerTime;
    float TimeDifference = FMath::Abs(InputTime - EventTime);
    
    if (TimeDifference <= Event.PerfectWindow)
    {
        return ERhythmEventResult::Perfect;
    }
    else if (TimeDifference <= Event.GoodWindow)
    {
        return ERhythmEventResult::Good;
    }
    else if (TimeDifference <= Event.SuccessWindow)
    {
        return ERhythmEventResult::Hit;
    }
    else
    {
        return ERhythmEventResult::Miss;
    }
}

void URhythmCookingMinigame::IncrementCombo()
{
    CurrentCombo++;
    ComboMultiplier = FMath::Min(CurrentCombo / 5 + 1, 4); // 최대 4배
    UE_LOG(LogTemp, Log, TEXT("URhythmCookingMinigame::IncrementCombo - Combo: %d, Multiplier: %d"), 
           CurrentCombo, ComboMultiplier);
}

void URhythmCookingMinigame::ResetCombo()
{
    CurrentCombo = 0;
    ComboMultiplier = 1;
    UE_LOG(LogTemp, Log, TEXT("URhythmCookingMinigame::ResetCombo - Combo reset"));
}

float URhythmCookingMinigame::CalculateEventScore(ERhythmEventResult Result) const
{
    float BaseScore = 0.0f;
    
    switch (Result)
    {
    case ERhythmEventResult::Perfect:
        BaseScore = PerfectScore;
        break;
    case ERhythmEventResult::Good:
        BaseScore = GoodScore;
        break;
    case ERhythmEventResult::Hit:
        BaseScore = HitScore;
        break;
    case ERhythmEventResult::Miss:
        BaseScore = 0.0f;
        break;
    default:
        BaseScore = 0.0f;
        break;
    }
    
    return BaseScore * ComboMultiplier;
}

void URhythmCookingMinigame::CreateRhythmPattern(const FString& CookingMethod, float Duration)
{
    // 요리법에 따른 패턴 생성 (향후 확장)
    UE_LOG(LogTemp, Log, TEXT("URhythmCookingMinigame::CreateRhythmPattern - Creating pattern for %s"), 
           *CookingMethod);
    
    GenerateRhythmEvents(CookingMethod, Duration);
} 