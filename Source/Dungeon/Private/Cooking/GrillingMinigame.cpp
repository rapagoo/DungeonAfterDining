#include "Cooking/GrillingMinigame.h"
#include "UI/Inventory/CookingWidget.h"
#include "InteractablePot.h"
#include "Engine/Engine.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"

UGrillingMinigame::UGrillingMinigame()
{
    // 굽기 전용 설정
    GameSettings.GameDuration = 15.0f; // 굽기는 조금 더 긴 시간
    GameSettings.SuccessThreshold = 70.0f;
    GameSettings.PerfectThreshold = 90.0f;
    
    // 굽기 상태 초기화
    CurrentSide = EGrillingSide::FirstSide;
    CurrentDoneness = EGrillingDoneness::Raw;
    HeatLevel = 0.5f;
    OptimalHeatLevel = 0.7f; // 최적 화력 레벨
    
    // 두 면의 진행도 초기화
    SideCookingProgress.SetNum(2);
    SideCookingProgress[0] = 0.0f; // 첫 번째 면
    SideCookingProgress[1] = 0.0f; // 두 번째 면
    
    CurrentEventIndex = 0;
    
    // 굽기 설정값
    BaseCookingSpeed = 0.08f; // 초당 8% 진행
    HeatMultiplier = 1.5f;
    
    // 점수 설정
    PerfectFlipScore = 50.0f;
    GoodFlipScore = 30.0f;
    HeatControlScore = 20.0f;
}

void UGrillingMinigame::StartMinigame(UCookingWidget* InWidget, AInteractablePot* InPot)
{
    Super::StartMinigame(InWidget, InPot);
    
    // 굽기 이벤트 생성
    GenerateGrillingEvents();
    
    // 지글지글 사운드 재생
    if (SizzleSound.LoadSynchronous() && OwningPot.IsValid())
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), SizzleSound.Get(), OwningPot->GetActorLocation());
    }
    
    UE_LOG(LogTemp, Log, TEXT("UGrillingMinigame::StartMinigame - Grilling started with %d events"), 
           GrillingEvents.Num());
}

void UGrillingMinigame::UpdateMinigame(float DeltaTime)
{
    Super::UpdateMinigame(DeltaTime);
    
    if (!bIsGameActive)
    {
        return;
    }
    
    // 굽기 진행도 업데이트
    UpdateCookingProgress(DeltaTime);
    
    // 굽기 정도 계산
    CalculateDoneness();
    
    // 현재 이벤트 체크
    float CurrentTime = GetWorld()->GetTimeSeconds() - GameStartTime;
    
    if (CurrentEventIndex < GrillingEvents.Num())
    {
        FGrillingEvent& CurrentEvent = GrillingEvents[CurrentEventIndex];
        
        // 이벤트 시작 시점이 되었고 아직 처리되지 않았다면
        if (CurrentTime >= CurrentEvent.StartTime && !CurrentEvent.bProcessed)
        {
            // UI에 이벤트 알림
            if (OwningWidget.IsValid())
            {
                FString ActionType;
                switch (CurrentEvent.EventType)
                {
                case EGrillingEventType::Flip:
                    ActionType = TEXT("Flip");
                    break;
                case EGrillingEventType::AdjustHeat:
                    // 현재 열 레벨에 따라 HeatUp 또는 HeatDown 결정
                    ActionType = (HeatLevel < OptimalHeatLevel) ? TEXT("HeatUp") : TEXT("HeatDown");
                    break;
                case EGrillingEventType::CheckDoneness:
                    ActionType = TEXT("Check");
                    break;
                default:
                    ActionType = TEXT("Unknown");
                    break;
                }
                
                // 위젯에 필요한 액션 알림
                OwningWidget->UpdateRequiredAction(ActionType, true);
                
                UE_LOG(LogTemp, Warning, TEXT("🍖 UGrillingMinigame - Event %d started, action required: %s"), 
                       CurrentEventIndex, *ActionType);
                UE_LOG(LogTemp, Warning, TEXT("🍖 UGrillingMinigame - Current heat level: %.2f, Optimal: %.2f"), 
                       HeatLevel, OptimalHeatLevel);
                
                // 위젯에 미니게임 업데이트 알림
                OwningWidget->OnMinigameUpdated(CurrentScore, (int32)CurrentPhase);
            }
        }
        
        // 이벤트 타임아웃 체크
        if (CurrentTime >= CurrentEvent.StartTime + CurrentEvent.Duration && !CurrentEvent.bProcessed)
        {
            // 타임아웃된 이벤트는 실패 처리
            UE_LOG(LogTemp, Warning, TEXT("UGrillingMinigame - Event %d TIMED OUT"), CurrentEventIndex);
            CurrentEvent.bProcessed = true;
            CurrentEventIndex++;
            
            // 페널티 적용
            AddScore(-20.0f);
            
            // 위젯에 액션 비활성화 알림
            if (OwningWidget.IsValid())
            {
                OwningWidget->UpdateRequiredAction(TEXT(""), false);
            }
            
            // 다음 이벤트로 이동하거나 게임 종료 체크
            if (CurrentEventIndex >= GrillingEvents.Num())
            {
                EndMinigame();
                return;
            }
        }
    }
    else
    {
        // 모든 이벤트 완료
        EndMinigame();
    }
    
    // 너무 타버렸다면 게임 종료
    if (CurrentDoneness == EGrillingDoneness::Burnt)
    {
        // 점수 페널티
        AddScore(-50.0f);
        EndMinigame();
    }
}

void UGrillingMinigame::EndMinigame()
{
    // 최종 품질 계산
    float QualityBonus = CalculateGrillingQuality();
    AddScore(QualityBonus);
    
    UE_LOG(LogTemp, Log, TEXT("UGrillingMinigame::EndMinigame - Final doneness: %d, Quality bonus: %.2f"), 
           (int32)CurrentDoneness, QualityBonus);
    
    Super::EndMinigame();
}

void UGrillingMinigame::HandlePlayerInput(const FString& InputType)
{
    Super::HandlePlayerInput(InputType);
    
    if (!bIsGameActive)
    {
        return;
    }
    
    // 현재 활성 이벤트가 있는지 확인
    if (CurrentEventIndex >= GrillingEvents.Num())
    {
        return;
    }
    
    FGrillingEvent& CurrentEvent = GrillingEvents[CurrentEventIndex];
    if (CurrentEvent.bProcessed)
    {
        return;
    }
    
    float CurrentTime = GetWorld()->GetTimeSeconds() - GameStartTime;
    
    // 입력 타입에 따른 처리
    if (InputType == TEXT("Flip") && CurrentEvent.EventType == EGrillingEventType::Flip)
    {
        bool bSuccess = JudgeFlipTiming(CurrentEvent);
        if (bSuccess)
        {
            FlipFood();
        }
        CurrentEvent.bProcessed = true;
        CurrentEventIndex++;
        
        // 위젯에 액션 비활성화 알림
        if (OwningWidget.IsValid())
        {
            OwningWidget->UpdateRequiredAction(TEXT(""), false);
        }
    }
    else if (InputType == TEXT("HeatUp") && CurrentEvent.EventType == EGrillingEventType::AdjustHeat)
    {
        AdjustHeat(0.2f);
        AddScore(HeatControlScore);
        CurrentEvent.bProcessed = true;
        CurrentEventIndex++;
        
        // 위젯에 액션 비활성화 알림
        if (OwningWidget.IsValid())
        {
            OwningWidget->UpdateRequiredAction(TEXT(""), false);
        }
    }
    else if (InputType == TEXT("HeatDown") && CurrentEvent.EventType == EGrillingEventType::AdjustHeat)
    {
        AdjustHeat(-0.2f);
        AddScore(HeatControlScore);
        CurrentEvent.bProcessed = true;
        CurrentEventIndex++;
        
        // 위젯에 액션 비활성화 알림
        if (OwningWidget.IsValid())
        {
            OwningWidget->UpdateRequiredAction(TEXT(""), false);
        }
    }
    else if (InputType == TEXT("Check") && CurrentEvent.EventType == EGrillingEventType::CheckDoneness)
    {
        // 익힘 정도 확인 점수
        AddScore(15.0f);
        CurrentEvent.bProcessed = true;
        CurrentEventIndex++;
        
        // 위젯에 액션 비활성화 알림
        if (OwningWidget.IsValid())
        {
            OwningWidget->UpdateRequiredAction(TEXT(""), false);
        }
    }
}

ECookingMinigameResult UGrillingMinigame::CalculateResult() const
{
    // 굽기 정도에 따른 추가 보정
    float FinalScore = CurrentScore;
    
    switch (CurrentDoneness)
    {
    case EGrillingDoneness::Medium:
    case EGrillingDoneness::MediumRare:
    case EGrillingDoneness::MediumWell:
        FinalScore += 20.0f; // 적절한 굽기 보너스
        break;
    case EGrillingDoneness::WellDone:
        FinalScore += 10.0f; // 약간 익은 것도 괜찮음
        break;
    case EGrillingDoneness::Burnt:
        FinalScore -= 30.0f; // 탄 것은 큰 페널티
        break;
    case EGrillingDoneness::Raw:
    case EGrillingDoneness::Rare:
        FinalScore -= 15.0f; // 덜 익은 것도 페널티
        break;
    }
    
    // 베이스 클래스의 결과 계산 로직 사용하되, 최종 점수로 계산
    if (FinalScore >= GameSettings.PerfectThreshold)
        return ECookingMinigameResult::Perfect;
    else if (FinalScore >= GameSettings.SuccessThreshold + 20.0f)
        return ECookingMinigameResult::Good;
    else if (FinalScore >= GameSettings.SuccessThreshold)
        return ECookingMinigameResult::Average;
    else if (FinalScore >= GameSettings.SuccessThreshold * 0.5f)
        return ECookingMinigameResult::Poor;
    else
        return ECookingMinigameResult::Failed;
}

float UGrillingMinigame::GetCurrentSideProgress() const
{
    if (CurrentSide == EGrillingSide::FirstSide && SideCookingProgress.Num() > 0)
    {
        return SideCookingProgress[0];
    }
    else if (CurrentSide == EGrillingSide::SecondSide && SideCookingProgress.Num() > 1)
    {
        return SideCookingProgress[1];
    }
    return 0.0f;
}

FGrillingEvent UGrillingMinigame::GetCurrentEvent() const
{
    if (CurrentEventIndex < GrillingEvents.Num())
    {
        return GrillingEvents[CurrentEventIndex];
    }
    return FGrillingEvent();
}

void UGrillingMinigame::GenerateGrillingEvents()
{
    GrillingEvents.Empty();
    
    float GameDuration = GameSettings.GameDuration;
    
    // 첫 번째 면 굽기 (게임 시작 ~ 중간)
    FGrillingEvent FirstFlip;
    FirstFlip.EventType = EGrillingEventType::Flip;
    FirstFlip.StartTime = GameDuration * 0.4f; // 40% 지점에서 뒤집기
    FirstFlip.Duration = 3.0f;
    FirstFlip.OptimalWindow = 1.0f;
    FirstFlip.AcceptableWindow = 2.0f;
    GrillingEvents.Add(FirstFlip);
    
    // 화력 조절 이벤트
    FGrillingEvent HeatAdjust1;
    HeatAdjust1.EventType = EGrillingEventType::AdjustHeat;
    HeatAdjust1.StartTime = GameDuration * 0.2f; // 20% 지점
    HeatAdjust1.Duration = 2.0f;
    GrillingEvents.Add(HeatAdjust1);
    
    // 두 번째 면 굽기 중 화력 조절
    FGrillingEvent HeatAdjust2;
    HeatAdjust2.EventType = EGrillingEventType::AdjustHeat;
    HeatAdjust2.StartTime = GameDuration * 0.7f; // 70% 지점
    HeatAdjust2.Duration = 2.0f;
    GrillingEvents.Add(HeatAdjust2);
    
    // 마지막 익힘 정도 확인
    FGrillingEvent FinalCheck;
    FinalCheck.EventType = EGrillingEventType::CheckDoneness;
    FinalCheck.StartTime = GameDuration * 0.9f; // 90% 지점
    FinalCheck.Duration = 2.0f;
    GrillingEvents.Add(FinalCheck);
    
    // 시간순으로 정렬
    GrillingEvents.Sort([](const FGrillingEvent& A, const FGrillingEvent& B) {
        return A.StartTime < B.StartTime;
    });
    
    UE_LOG(LogTemp, Log, TEXT("UGrillingMinigame::GenerateGrillingEvents - Generated %d events"), 
           GrillingEvents.Num());
}

void UGrillingMinigame::FlipFood()
{
    if (CurrentSide == EGrillingSide::FirstSide)
    {
        CurrentSide = EGrillingSide::SecondSide;
        UE_LOG(LogTemp, Log, TEXT("UGrillingMinigame::FlipFood - Flipped to second side"));
    }
    else if (CurrentSide == EGrillingSide::SecondSide)
    {
        CurrentSide = EGrillingSide::Completed;
        UE_LOG(LogTemp, Log, TEXT("UGrillingMinigame::FlipFood - Cooking completed"));
    }
    
    // 뒤집기 사운드 재생
    if (FlipSound.LoadSynchronous() && OwningPot.IsValid())
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), FlipSound.Get(), OwningPot->GetActorLocation());
    }
}

void UGrillingMinigame::AdjustHeat(float Delta)
{
    HeatLevel = FMath::Clamp(HeatLevel + Delta, 0.0f, 1.0f);
    UE_LOG(LogTemp, Log, TEXT("UGrillingMinigame::AdjustHeat - Heat level: %.2f"), HeatLevel);
}

void UGrillingMinigame::UpdateCookingProgress(float DeltaTime)
{
    if (CurrentSide == EGrillingSide::Completed)
    {
        return;
    }
    
    // 화력에 따른 굽기 속도 계산
    float CookingSpeed = BaseCookingSpeed * (1.0f + HeatLevel * HeatMultiplier);
    float ProgressIncrement = CookingSpeed * DeltaTime;
    
    // 현재 면의 진행도 업데이트
    int32 SideIndex = (CurrentSide == EGrillingSide::FirstSide) ? 0 : 1;
    if (SideCookingProgress.IsValidIndex(SideIndex))
    {
        SideCookingProgress[SideIndex] = FMath::Clamp(
            SideCookingProgress[SideIndex] + ProgressIncrement, 0.0f, 1.5f); // 1.5까지 가능 (타버림)
    }
}

void UGrillingMinigame::CalculateDoneness()
{
    // 두 면의 평균 진행도 계산
    float AverageProgress = (SideCookingProgress[0] + SideCookingProgress[1]) * 0.5f;
    
    if (AverageProgress >= 1.3f)
    {
        CurrentDoneness = EGrillingDoneness::Burnt;
    }
    else if (AverageProgress >= 1.1f)
    {
        CurrentDoneness = EGrillingDoneness::WellDone;
    }
    else if (AverageProgress >= 0.9f)
    {
        CurrentDoneness = EGrillingDoneness::MediumWell;
    }
    else if (AverageProgress >= 0.7f)
    {
        CurrentDoneness = EGrillingDoneness::Medium;
    }
    else if (AverageProgress >= 0.5f)
    {
        CurrentDoneness = EGrillingDoneness::MediumRare;
    }
    else if (AverageProgress >= 0.3f)
    {
        CurrentDoneness = EGrillingDoneness::Rare;
    }
    else
    {
        CurrentDoneness = EGrillingDoneness::Raw;
    }
}

bool UGrillingMinigame::JudgeFlipTiming(const FGrillingEvent& Event)
{
    float CurrentTime = GetWorld()->GetTimeSeconds() - GameStartTime;
    float EventTime = CurrentTime - Event.StartTime;
    
    bool bSuccess = false;
    float ScoreToAdd = 0.0f;
    
    if (EventTime <= Event.OptimalWindow)
    {
        // 완벽한 타이밍
        ScoreToAdd = PerfectFlipScore;
        bSuccess = true;
        UE_LOG(LogTemp, Log, TEXT("UGrillingMinigame::JudgeFlipTiming - Perfect flip!"));
    }
    else if (EventTime <= Event.AcceptableWindow)
    {
        // 괜찮은 타이밍
        ScoreToAdd = GoodFlipScore;
        bSuccess = true;
        UE_LOG(LogTemp, Log, TEXT("UGrillingMinigame::JudgeFlipTiming - Good flip!"));
    }
    else
    {
        // 타이밍 실패
        ScoreToAdd = -10.0f;
        bSuccess = false;
        UE_LOG(LogTemp, Log, TEXT("UGrillingMinigame::JudgeFlipTiming - Failed flip timing"));
    }
    
    AddScore(ScoreToAdd);
    return bSuccess;
}

float UGrillingMinigame::CalculateGrillingQuality() const
{
    float QualityScore = 0.0f;
    
    // 굽기 정도에 따른 품질 점수
    switch (CurrentDoneness)
    {
    case EGrillingDoneness::Medium:
        QualityScore += 50.0f; // 최고 품질
        break;
    case EGrillingDoneness::MediumRare:
    case EGrillingDoneness::MediumWell:
        QualityScore += 35.0f; // 좋은 품질
        break;
    case EGrillingDoneness::WellDone:
        QualityScore += 20.0f; // 괜찮은 품질
        break;
    case EGrillingDoneness::Rare:
        QualityScore += 10.0f; // 아쉬운 품질
        break;
    case EGrillingDoneness::Raw:
        QualityScore -= 20.0f; // 실패
        break;
    case EGrillingDoneness::Burnt:
        QualityScore -= 40.0f; // 큰 실패
        break;
    }
    
    // 양쪽 면의 균등한 굽기 보너스
    float ProgressDifference = FMath::Abs(SideCookingProgress[0] - SideCookingProgress[1]);
    if (ProgressDifference < 0.1f)
    {
        QualityScore += 15.0f; // 균등하게 구웠음
    }
    else if (ProgressDifference < 0.2f)
    {
        QualityScore += 5.0f; // 어느 정도 균등함
    }
    
    return QualityScore;
}

void UGrillingMinigame::OnEventTimeout(const FGrillingEvent& Event)
{
    // 이벤트 타임아웃 처리
    switch (Event.EventType)
    {
    case EGrillingEventType::Flip:
        UE_LOG(LogTemp, Warning, TEXT("UGrillingMinigame::OnEventTimeout - Missed flip timing!"));
        AddScore(-20.0f); // 뒤집기 실패 페널티
        break;
    case EGrillingEventType::AdjustHeat:
        UE_LOG(LogTemp, Warning, TEXT("UGrillingMinigame::OnEventTimeout - Missed heat adjustment!"));
        AddScore(-10.0f); // 화력 조절 실패 페널티
        break;
    case EGrillingEventType::CheckDoneness:
        UE_LOG(LogTemp, Warning, TEXT("UGrillingMinigame::OnEventTimeout - Missed doneness check!"));
        AddScore(-5.0f); // 확인 실패 페널티
        break;
    }
} 