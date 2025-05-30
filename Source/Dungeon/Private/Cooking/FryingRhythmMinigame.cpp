#include "Cooking/FryingRhythmMinigame.h"
#include "UI/Inventory/CookingWidget.h"
#include "InteractablePot.h"
#include "Engine/Engine.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "Audio/CookingAudioManager.h"

UFryingRhythmMinigame::UFryingRhythmMinigame()
{
    // 튀기기 전용 설정
    GameSettings.GameDuration = 30.0f; // 20초에서 30초로 늘림 (더 여유롭게)
    GameSettings.SuccessThreshold = 60.0f;
    GameSettings.PerfectThreshold = 85.0f;
    
    // 점수 설정
    PerfectScore = 100.0f;
    GoodScore = 75.0f;
    HitScore = 50.0f;
    MissScore = -25.0f;
    
    // 콤보 설정
    ComboMultiplier = 1.2f;
    CurrentCombo = 0;
    MaxCombo = 0;
    
    // 요리 온도 설정
    CookingTemperature = 0.5f;
    OptimalTempMin = 0.4f;
    OptimalTempMax = 0.8f;
    TemperatureChangeRate = 0.1f;
    
    // 초기화
    CurrentNoteIndex = 0;
    TimeToNextNote = 0.0f;
    CurrentNoteTiming = 0.0f;
}

void UFryingRhythmMinigame::StartMinigame(UCookingWidget* InWidget, AInteractablePot* InPot)
{
    Super::StartMinigame(InWidget, InPot);
    
    // 리듬 노트 생성
    GenerateRhythmNotes();
    
    // 백그라운드 음악 재생
    if (BackgroundMusic.LoadSynchronous() && OwningPot.IsValid())
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), BackgroundMusic.Get(), OwningPot->GetActorLocation());
    }
    
    // 지글지글 사운드 재생
    if (SizzleSound.LoadSynchronous() && OwningPot.IsValid())
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), SizzleSound.Get(), OwningPot->GetActorLocation());
    }
    
    // 오디오 매니저에 배경음악 변경 알림
    if (OwningPot.IsValid())
    {
        if (UCookingAudioManager* AudioManager = OwningPot->GetAudioManager())
        {
            AudioManager->PlayCookingMusic(TEXT("Frying"));
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("UFryingRhythmMinigame::StartMinigame - Started with %d rhythm notes"), 
           RhythmNotes.Num());
}

void UFryingRhythmMinigame::UpdateMinigame(float DeltaTime)
{
    Super::UpdateMinigame(DeltaTime);
    
    if (!bIsGameActive)
    {
        return;
    }
    
    float ElapsedTime = GetWorld()->GetTimeSeconds() - GameStartTime;
    
    // 요리 온도 업데이트
    UpdateCookingTemperature(DeltaTime);
    
    // 현재 노트 업데이트
    if (CurrentNoteIndex < RhythmNotes.Num())
    {
        FRhythmNote& CurrentNote = RhythmNotes[CurrentNoteIndex];
        
        // 현재 노트가 활성화되었는지 확인
        if (ElapsedTime >= CurrentNote.TriggerTime && !CurrentNote.bCompleted)
        {
            // 리듬게임 시각적 노트 시작
            FString ActionType = GetActionTypeFromNoteType(CurrentNote.NoteType);
            if (OwningWidget.IsValid())
            {
                // 새로운 리듬게임 UI 시작
                OwningWidget->StartRhythmGameNote(ActionType, CurrentNote.HitWindow * 2.0f);
                UE_LOG(LogTemp, Log, TEXT("UFryingRhythmMinigame - Started visual rhythm note: %s"), *ActionType);
            }
            
            // 노트를 활성화 상태로 표시 (한 번만 시작되도록)
            CurrentNote.bCompleted = true; // 임시로 완료 표시해서 중복 시작 방지
        }
        
        // 진행도 계산 및 UI 업데이트
        if (OwningWidget.IsValid())
        {
            float NoteProgress = (ElapsedTime - CurrentNote.TriggerTime) / (CurrentNote.HitWindow * 2.0f);
            NoteProgress = FMath::Clamp(NoteProgress, 0.0f, 1.0f);
            
            // 리듬게임 원 애니메이션 업데이트
            OwningWidget->UpdateRhythmGameTiming(NoteProgress);
            
            // 노트 타임아웃 체크
            if (NoteProgress >= 1.0f && CurrentNote.bCompleted)
            {
                // 아직 입력이 없었다면 Miss 처리
                if (CurrentNote.Result == ERhythmEventResult::None)
                {
                    HandleMissedNote();
                }
            }
        }
    }
    
    // UI 업데이트
    if (OwningWidget.IsValid())
    {
        OwningWidget->OnMinigameUpdated(CurrentScore, (int32)CurrentPhase);
    }
}

void UFryingRhythmMinigame::EndMinigame()
{
    // 마지막 콤보 기록
    MaxCombo = FMath::Max(MaxCombo, CurrentCombo);
    
    // 오디오 매니저에 배경음악 복원 알림
    if (OwningPot.IsValid())
    {
        if (UCookingAudioManager* AudioManager = OwningPot->GetAudioManager())
        {
            AudioManager->RestoreBackgroundMusic();
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("UFryingRhythmMinigame::EndMinigame - Final Score: %.2f, Max Combo: %d"), 
           CurrentScore, MaxCombo);
    
    Super::EndMinigame();
}

void UFryingRhythmMinigame::HandlePlayerInput(const FString& InputType)
{
    if (!bIsGameActive || CurrentNoteIndex >= RhythmNotes.Num())
    {
        return;
    }
    
    FRhythmNote& CurrentNote = RhythmNotes[CurrentNoteIndex];
    if (CurrentNote.bCompleted)
    {
        return;
    }
    
    // 현재 노트를 처리
    ProcessCurrentNote(InputType);
}

void UFryingRhythmMinigame::ProcessCurrentNote(const FString& InputType)
{
    if (CurrentNoteIndex >= RhythmNotes.Num())
    {
        return;
    }
    
    FRhythmNote& CurrentNote = RhythmNotes[CurrentNoteIndex];
    float ElapsedTime = GetWorld()->GetTimeSeconds() - GameStartTime;
    
    // 입력 타입이 노트 타입과 맞는지 확인
    FString ExpectedAction = GetActionTypeFromNoteType(CurrentNote.NoteType);
    if (InputType != ExpectedAction)
    {
        // 잘못된 입력
        UE_LOG(LogTemp, Warning, TEXT("UFryingRhythmMinigame::ProcessCurrentNote - Wrong input. Expected: %s, Got: %s"), 
               *ExpectedAction, *InputType);
        return;
    }
    
    // 리듬게임 타이밍 계산 - 노트의 중간 지점을 Perfect 타이밍으로 설정
    float NoteDuration = CurrentNote.HitWindow * 2.0f; // 전체 노트 지속 시간
    float PerfectTiming = CurrentNote.TriggerTime + (NoteDuration * 0.5f); // 중간 지점이 Perfect
    float TimingDifference = FMath::Abs(ElapsedTime - PerfectTiming);
    ERhythmEventResult Result = CalculateTimingResult(TimingDifference, CurrentNote);
    
    // 노트 완료 처리 (실제 완료로 업데이트)
    CurrentNote.bCompleted = true;
    CurrentNote.Result = Result;
    
    // 결과 문자열 생성
    FString ResultString;
    switch (Result)
    {
    case ERhythmEventResult::Perfect:
        ResultString = TEXT("Perfect");
        break;
    case ERhythmEventResult::Good:
        ResultString = TEXT("Good");
        break;
    case ERhythmEventResult::Hit:
        ResultString = TEXT("Hit");
        break;
    case ERhythmEventResult::Miss:
        ResultString = TEXT("Miss");
        break;
    default:
        ResultString = TEXT("Unknown");
        break;
    }
    
    // 시각적 결과 표시
    if (OwningWidget.IsValid())
    {
        OwningWidget->ShowRhythmGameResult(ResultString);
        OwningWidget->EndRhythmGameNote();
    }
    
    // 노트 타입에 따른 온도 변화
    if (Result != ERhythmEventResult::Miss)
    {
        switch (CurrentNote.NoteType)
        {
        case ERhythmNoteType::Stir:
            // 저으면 온도가 적당히 올라감 (고르게 섞어서 열전달)
            CookingTemperature = FMath::Clamp(CookingTemperature + 0.12f, 0.0f, 1.0f);
            break;
        case ERhythmNoteType::Temp:
            // 온도 체크는 온도를 적정선으로 조절
            if (CookingTemperature > OptimalTempMax)
            {
                CookingTemperature = FMath::Clamp(CookingTemperature - 0.2f, 0.0f, 1.0f);
            }
            else if (CookingTemperature < OptimalTempMin)
            {
                CookingTemperature = FMath::Clamp(CookingTemperature + 0.2f, 0.0f, 1.0f);
            }
            break;
        case ERhythmNoteType::Season:
            // 양념 추가는 온도를 약간 낮춤 (차가운 양념 추가)
            CookingTemperature = FMath::Clamp(CookingTemperature - 0.05f, 0.0f, 1.0f);
            break;
        }
    }
    
    // 점수 계산 및 추가
    float BaseScore = 0.0f;
    switch (Result)
    {
    case ERhythmEventResult::Perfect:
        BaseScore = PerfectScore;
        IncreaseCombo();
        break;
    case ERhythmEventResult::Good:
        BaseScore = GoodScore;
        IncreaseCombo();
        break;
    case ERhythmEventResult::Hit:
        BaseScore = HitScore;
        IncreaseCombo();
        break;
    case ERhythmEventResult::Miss:
        BaseScore = MissScore;
        ResetCombo();
        break;
    }
    
    // 콤보 보너스 적용
    float FinalScore = BaseScore;
    if (Result != ERhythmEventResult::Miss && CurrentCombo > 1)
    {
        FinalScore *= FMath::Pow(ComboMultiplier, FMath::Min(CurrentCombo / 5, 3)); // 최대 3단계 콤보 보너스
    }
    
    // 온도 보너스 적용
    if (IsTemperatureOptimal())
    {
        FinalScore *= 1.2f;
    }
    
    AddScore(FinalScore);
    
    // 사운드 재생
    PlayResultSound(Result);
    
    // 다음 노트로 이동
    CurrentNoteIndex++;
    
    UE_LOG(LogTemp, Log, TEXT("UFryingRhythmMinigame::ProcessCurrentNote - Result: %s, Score: %.2f, Combo: %d, Temp: %.2f"), 
           *ResultString, FinalScore, CurrentCombo, CookingTemperature);
}

void UFryingRhythmMinigame::HandleMissedNote()
{
    if (CurrentNoteIndex >= RhythmNotes.Num())
    {
        return;
    }
    
    FRhythmNote& CurrentNote = RhythmNotes[CurrentNoteIndex];
    
    // Miss 처리
    CurrentNote.bCompleted = true;
    CurrentNote.Result = ERhythmEventResult::Miss;
    
    // 시각적 Miss 표시
    if (OwningWidget.IsValid())
    {
        OwningWidget->ShowRhythmGameResult(TEXT("Miss"));
        OwningWidget->EndRhythmGameNote();
    }
    
    AddScore(MissScore);
    ResetCombo();
    
    // 사운드 재생
    PlayResultSound(ERhythmEventResult::Miss);
    
    // 다음 노트로 이동
    CurrentNoteIndex++;
    
    UE_LOG(LogTemp, Log, TEXT("UFryingRhythmMinigame::HandleMissedNote - Note missed, Combo reset"));
}

ERhythmEventResult UFryingRhythmMinigame::CalculateTimingResult(float TimingDifference, const FRhythmNote& Note)
{
    if (TimingDifference <= Note.PerfectWindow)
    {
        return ERhythmEventResult::Perfect;
    }
    else if (TimingDifference <= Note.GoodWindow)
    {
        return ERhythmEventResult::Good;
    }
    else if (TimingDifference <= Note.HitWindow)
    {
        return ERhythmEventResult::Hit;
    }
    else
    {
        return ERhythmEventResult::Miss;
    }
}

void UFryingRhythmMinigame::GenerateRhythmNotes()
{
    RhythmNotes.Empty();
    
    // 게임 시간 동안 적절한 간격으로 노트 생성
    float NoteInterval = 3.0f; // 3초마다 노트 (2초에서 3초로 늘림)
    int32 TotalNotes = FMath::FloorToInt(GameSettings.GameDuration / NoteInterval);
    
    for (int32 i = 0; i < TotalNotes; i++)
    {
        FRhythmNote NewNote;
        NewNote.TriggerTime = (i + 1) * NoteInterval;
        
        // 튀기기에 적합한 노트 타입만 사용 (Stir와 Temp만 사용)
        if (i % 2 == 0)  // 2가지 타입만 번갈아 사용
        {
            NewNote.NoteType = ERhythmNoteType::Stir;     // 흔들기/저어주기
        }
        else
        {
            NewNote.NoteType = ERhythmNoteType::Temp;     // 온도 확인
        }
        
        // 튀기기에 맞는 더 관대한 타이밍 윈도우 설정
        NewNote.PerfectWindow = 0.5f;  // 0.3초 -> 0.5초로 늘림 (더 관대하게)
        NewNote.GoodWindow = 0.8f;     // 0.6초 -> 0.8초로 늘림  
        NewNote.HitWindow = 1.2f;      // 1.0초 -> 1.2초로 늘림
        
        // 게임이 진행될수록 타이밍 윈도우를 약간 줄여서 난이도 증가 (더욱 완화)
        float DifficultyMultiplier = 1.0f - (i / float(TotalNotes)) * 0.1f; // 최대 10% 감소 (20%에서 10%로 더 완화)
        NewNote.PerfectWindow *= DifficultyMultiplier;
        NewNote.GoodWindow *= DifficultyMultiplier;
        NewNote.HitWindow *= DifficultyMultiplier;
        
        RhythmNotes.Add(NewNote);
    }
    
    UE_LOG(LogTemp, Log, TEXT("UFryingRhythmMinigame::GenerateRhythmNotes - Generated %d frying-specific notes (Stir/Temp only) with generous timing"), 
           RhythmNotes.Num());
}

void UFryingRhythmMinigame::UpdateCookingTemperature(float DeltaTime)
{
    // 온도는 자연적으로 중간값으로 수렴
    float TargetTemp = 0.5f;
    float TempDelta = (TargetTemp - CookingTemperature) * TemperatureChangeRate * DeltaTime;
    
    // 플레이어의 액션에 따른 온도 변화는 각 노트 처리에서 구현
    CookingTemperature = FMath::Clamp(CookingTemperature + TempDelta, 0.0f, 1.0f);
}

bool UFryingRhythmMinigame::IsTemperatureOptimal() const
{
    return CookingTemperature >= OptimalTempMin && CookingTemperature <= OptimalTempMax;
}

void UFryingRhythmMinigame::IncreaseCombo()
{
    CurrentCombo++;
    MaxCombo = FMath::Max(MaxCombo, CurrentCombo);
}

void UFryingRhythmMinigame::ResetCombo()
{
    CurrentCombo = 0;
}

void UFryingRhythmMinigame::PlayResultSound(ERhythmEventResult Result)
{
    USoundBase* SoundToPlay = nullptr;
    
    switch (Result)
    {
    case ERhythmEventResult::Perfect:
        SoundToPlay = PerfectSound.LoadSynchronous();
        break;
    case ERhythmEventResult::Good:
        SoundToPlay = GoodSound.LoadSynchronous();
        break;
    case ERhythmEventResult::Hit:
        SoundToPlay = HitSound.LoadSynchronous();
        break;
    case ERhythmEventResult::Miss:
        SoundToPlay = MissSound.LoadSynchronous();
        break;
    }
    
    if (SoundToPlay && OwningPot.IsValid())
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), SoundToPlay, OwningPot->GetActorLocation());
    }
}

FRhythmNote UFryingRhythmMinigame::GetCurrentNote() const
{
    if (CurrentNoteIndex < RhythmNotes.Num())
    {
        return RhythmNotes[CurrentNoteIndex];
    }
    
    return FRhythmNote();
}

ECookingMinigameResult UFryingRhythmMinigame::CalculateResult() const
{
    // 기본 결과 계산
    ECookingMinigameResult BaseResult = Super::CalculateResult();
    
    // 콤보와 온도 상태에 따른 보너스/페널티 적용
    float ComboBonus = MaxCombo / 10.0f; // 최대 콤보 10마다 10% 보너스
    float TemperatureAdjustment = IsTemperatureOptimal() ? 0.1f : -0.1f;
    
    float AdjustedScore = CurrentScore * (1.0f + ComboBonus + TemperatureAdjustment);
    
    if (AdjustedScore >= GameSettings.PerfectThreshold)
    {
        return ECookingMinigameResult::Perfect;
    }
    else if (AdjustedScore >= GameSettings.SuccessThreshold + 20.0f)
    {
        return ECookingMinigameResult::Good;
    }
    else if (AdjustedScore >= GameSettings.SuccessThreshold)
    {
        return ECookingMinigameResult::Average;
    }
    else if (AdjustedScore >= GameSettings.SuccessThreshold * 0.5f)
    {
        return ECookingMinigameResult::Poor;
    }
    else
    {
        return ECookingMinigameResult::Failed;
    }
}

FString UFryingRhythmMinigame::GetActionTypeFromNoteType(ERhythmNoteType NoteType) const
{
    switch (NoteType)
    {
    case ERhythmNoteType::Stir:
        return TEXT("Stir"); // 튀기기에서는 실제로 흔들기/저어주기 액션
    case ERhythmNoteType::Temp:
        return TEXT("Check"); // 온도 확인
    case ERhythmNoteType::Season:
        return TEXT("Stir"); // 양념도 흔들어서 섞기
    default:
        return TEXT("Stir");
    }
} 