#include "Cooking/FryingRhythmMinigame.h"
#include "UI/Minigame/FryingRhythmMinigameWidget.h"
#include "Actors/InteractablePot.h"
#include "TimerManager.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Audio/CookingAudioManager.h"

UFryingRhythmMinigame::UFryingRhythmMinigame()
{
	PrimaryComponentTick.bCanEverTick = true;
	bIsGameActive = false;
	ComboCounter = 0;
	CookingTemperature = 0.5f;
}

void UFryingRhythmMinigame::StartMinigame(TWeakObjectPtr<UUserWidget> InWidget, AInteractablePot* InPot)
{
	Super::StartMinigame(InWidget, InPot);
	FryingRhythmWidget = Cast<UFryingRhythmMinigameWidget>(InWidget.Get());
	if (!FryingRhythmWidget.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("FryingRhythmMinigame received an incompatible widget!"));
		return;
	}
	
	bIsGameActive = true;
	ComboCounter = 0;
	CurrentScore = 0;
	CookingTemperature = 0.5f;
	CurrentNoteIndex = -1;

	GenerateRhythmNotes();
	MoveToNextNote();
}

void UFryingRhythmMinigame::EndMinigame()
{
	Super::EndMinigame();
	bIsGameActive = false;
	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
}

void UFryingRhythmMinigame::UpdateMinigame(float DeltaTime)
{
    Super::UpdateMinigame(DeltaTime);
	if (!bIsGameActive || !bNoteIsActive) return;

	float NoteDuration = RhythmNotes[CurrentNoteIndex].HitWindow * 2.0f;
	if (NoteDuration > 0)
	{
		CurrentNoteTiming += DeltaTime / NoteDuration;
		CurrentNoteTiming = FMath::Clamp(CurrentNoteTiming, 0.0f, 1.0f);

		if (FryingRhythmWidget.IsValid())
		{
			FryingRhythmWidget->UpdateRhythmTiming(CurrentNoteTiming);
		}

		if (CurrentNoteTiming >= 1.0f)
		{
			HandleMissedNote();
		}
	}
}

void UFryingRhythmMinigame::HandlePlayerInput(const FString& InputType)
{
    if (!bNoteIsActive) return;

    FRhythmNote& CurrentNote = RhythmNotes[CurrentNoteIndex];
    FString ExpectedInput = GetActionTypeFromNoteType(CurrentNote.NoteType);

    if (InputType == ExpectedInput)
    {
        float TimingDifference = FMath::Abs(CurrentNoteTiming - 0.5f); // Example timing
        ERhythmEventResult Result = CalculateTimingResult(TimingDifference, CurrentNote);
        
        if (FryingRhythmWidget.IsValid())
        {
            FString ResultString = UEnum::GetValueAsString(Result);
            FryingRhythmWidget->ShowRhythmResult(ResultString);
        }
        PlayResultSound(Result);
        MoveToNextNote();
    }
}

void UFryingRhythmMinigame::MoveToNextNote()
{
	bNoteIsActive = false;
	if (FryingRhythmWidget.IsValid())
	{
		FryingRhythmWidget->EndRhythmNote();
	}
    
    CurrentNoteIndex++;
    if (RhythmNotes.IsValidIndex(CurrentNoteIndex))
    {
        SpawnNewNote();
    }
    else
    {
        EndMinigame();
    }
}

void UFryingRhythmMinigame::SpawnNewNote()
{
	if (!RhythmNotes.IsValidIndex(CurrentNoteIndex)) return;

	bNoteIsActive = true;
	CurrentNoteTiming = 0.0f;
	
	if (FryingRhythmWidget.IsValid())
	{
		const FString ActionType = GetActionTypeFromNoteType(RhythmNotes[CurrentNoteIndex].NoteType);
		float NoteDuration = RhythmNotes[CurrentNoteIndex].HitWindow * 2.0f;
		FryingRhythmWidget->StartRhythmNote(ActionType, NoteDuration);
	}
}

void UFryingRhythmMinigame::HandleMissedNote()
{
    ResetCombo();
    PlayResultSound(ERhythmEventResult::Miss);
    if (FryingRhythmWidget.IsValid())
    {
        FryingRhythmWidget->ShowRhythmResult(TEXT("Miss"));
    }
    MoveToNextNote();
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

void UFryingRhythmMinigame::StartNoteMetronome(float NoteDuration)
{
    if (!bIsGameActive)
    {
        return;
    }

    // 이전 메트로놈이 있다면 정리
    StopNoteMetronome();
    
    // 메트로놈 상태 초기화
    CurrentTick = 0;
    bNoteMetronomeActive = true;
    
    // 노트 지속시간을 틱 수로 나누어 각 틱 간격 계산
    float TickInterval = NoteDuration / float(TicksPerNote);
    
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(
            MetronomeTimerHandle, 
            this, 
            &UFryingRhythmMinigame::PlayNoteMetronomeTick, 
            TickInterval, 
            true  // 반복
        );
        
        // 즉시 첫 번째 틱 재생
        PlayNoteMetronomeTick();
        
        UE_LOG(LogTemp, Log, TEXT("UFryingRhythmMinigame::StartNoteMetronome - Started for %.2f seconds, %d ticks, %.2f interval"), 
               NoteDuration, TicksPerNote, TickInterval);
    }
}

void UFryingRhythmMinigame::StopNoteMetronome()
{
    // 타이머 정리
    if (GetWorld() && MetronomeTimerHandle.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(MetronomeTimerHandle);
    }
    
    // 음악 루프 정리
    if (MetronomeAudioComponent && IsValid(MetronomeAudioComponent))
    {
        MetronomeAudioComponent->Stop();
        MetronomeAudioComponent->DestroyComponent();
        MetronomeAudioComponent = nullptr;
    }
    
    // 상태 리셋
    bNoteMetronomeActive = false;
    CurrentTick = 0;
    
    UE_LOG(LogTemp, Log, TEXT("UFryingRhythmMinigame::StopNoteMetronome - Stopped"));
}

void UFryingRhythmMinigame::PlayNoteMetronomeTick()
{
    if (!bIsGameActive || !bNoteMetronomeActive || !OwningPot.IsValid())
    {
        return;
    }

    CurrentTick++;
    
    // 메트로놈 사운드 재생
    if (MetronomeSound)
    {
        // 4번째 틱(마지막 틱)은 더 크게 재생 (Perfect 타이밍 강조)
        float Volume = (CurrentTick == TicksPerNote) ? MetronomeLastTickVolume : MetronomeTickVolume;
        
        UGameplayStatics::PlaySoundAtLocation(
            GetWorld(), 
            MetronomeSound, 
            OwningPot->GetActorLocation(),
            Volume
        );
        
        UE_LOG(LogTemp, Log, TEXT("UFryingRhythmMinigame::PlayNoteMetronomeTick - Tick %d/%d (Volume: %.1f) %s"), 
               CurrentTick, TicksPerNote, Volume, (CurrentTick == TicksPerNote) ? TEXT("[PERFECT TIMING]") : TEXT(""));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UFryingRhythmMinigame::PlayNoteMetronomeTick - MetronomeSound is not set"));
    }
    
    // 마지막 틱 후 메트로놈 정지
    if (CurrentTick >= TicksPerNote)
    {
        StopNoteMetronome();
    }
}

void UFryingRhythmMinigame::SetMetronomeSound(USoundBase* NewMetronomeSound)
{
    if (NewMetronomeSound)
    {
        MetronomeSound = NewMetronomeSound;
        UE_LOG(LogTemp, Log, TEXT("UFryingRhythmMinigame::SetMetronomeSound - Set to: %s"), *NewMetronomeSound->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UFryingRhythmMinigame::SetMetronomeSound - Tried to set null sound"));
    }
}

void UFryingRhythmMinigame::SetMetronomeVolume(float TickVolume, float LastTickVolume)
{
    MetronomeTickVolume = FMath::Clamp(TickVolume, 0.1f, 2.0f);
    MetronomeLastTickVolume = FMath::Clamp(LastTickVolume, 0.1f, 2.0f);
    UE_LOG(LogTemp, Log, TEXT("UFryingRhythmMinigame::SetMetronomeVolume - Tick: %.1f, LastTick: %.1f"), 
           MetronomeTickVolume, MetronomeLastTickVolume);
}

int32 UFryingRhythmMinigame::GetCurrentCombo() const
{
	return ComboCounter;
} 