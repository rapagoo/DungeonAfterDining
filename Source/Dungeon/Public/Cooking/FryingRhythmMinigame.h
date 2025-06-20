#pragma once

#include "CoreMinimal.h"
#include "Cooking/CookingMinigameBase.h"
#include "Cooking/RhythmCookingMinigame.h"
#include "Engine/DataTable.h"
#include "Sound/SoundBase.h"
#include "FryingRhythmMinigame.generated.h"

/**
 * 리듬 노트 타입
 */
UENUM(BlueprintType)
enum class ERhythmNoteType : uint8
{
    Stir,       // 젓기
    Flip,       // 뒤집기
    Temp,       // 온도 확인
    Season      // 양념 추가
};

/**
 * 리듬 노트 구조체 - 화면에 표시되는 타이밍 큐
 */
USTRUCT(BlueprintType)
struct DUNGEON_API FRhythmNote
{
    GENERATED_BODY()

    /** 노트가 활성화되는 시간 (게임 시작 후 초) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing")
    float TriggerTime = 0.0f;

    /** 노트 타입 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Note")
    ERhythmNoteType NoteType = ERhythmNoteType::Stir;

    /** 완벽한 타이밍 윈도우 (초) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing")
    float PerfectWindow = 0.1f;

    /** 좋은 타이밍 윈도우 (초) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing")
    float GoodWindow = 0.25f;

    /** 일반 타이밍 윈도우 (초) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing")
    float HitWindow = 0.4f;

    /** 노트 시각화 시작 여부 */
    UPROPERTY(BlueprintReadOnly, Category = "State")
    bool bVisualsStarted = false;

    /** 노트가 실제로 시각화되기 시작한 게임 시간 (초) */
    UPROPERTY(BlueprintReadOnly, Category = "State")
    float VisualStartTime = 0.0f;

    /** 노트 완료 여부 */
    UPROPERTY(BlueprintReadOnly, Category = "State")
    bool bCompleted = false;

    /** 노트 결과 */
    UPROPERTY(BlueprintReadOnly, Category = "State")
    ERhythmEventResult Result = ERhythmEventResult::None;
};

/**
 * 리듬 기반 튀기기 미니게임
 * 플레이어가 화면에 나타나는 타이밍 큐에 맞춰 적절한 액션을 수행
 */
UCLASS(Blueprintable, BlueprintType)
class DUNGEON_API UFryingRhythmMinigame : public UCookingMinigameBase
{
    GENERATED_BODY()

public:
    UFryingRhythmMinigame();

protected:
    /** 리듬 노트 목록 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm Settings")
    TArray<FRhythmNote> RhythmNotes;

    /** 현재 활성화된 노트 인덱스 */
    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    int32 CurrentNoteIndex = 0;

    /** 다음 노트까지의 시간 표시용 */
    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    float TimeToNextNote = 0.0f;

    /** 현재 노트의 정확도 표시 (0.0 = 완벽, 1.0 = 최악) */
    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    float CurrentNoteTiming = 0.0f;

    /** 백그라운드 음악 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TSoftObjectPtr<USoundBase> BackgroundMusic;

    /** 지글지글 사운드 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TSoftObjectPtr<USoundBase> SizzleSound;

    /** 메트로놈 사운드 (박자 맞춤용) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    USoundBase* MetronomeSound;

    /** 메트로놈 음악 파일 (긴 루프용) - 선택사항 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TSoftObjectPtr<USoundBase> MetronomeMusicLoop;

    /** 메트로놈 음악 재생용 오디오 컴포넌트 */
    UPROPERTY()
    class UAudioComponent* MetronomeAudioComponent;

    /** 완벽한 타이밍 사운드 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TSoftObjectPtr<USoundBase> PerfectSound;

    /** 좋은 타이밍 사운드 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TSoftObjectPtr<USoundBase> GoodSound;

    /** 일반 성공 사운드 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TSoftObjectPtr<USoundBase> HitSound;

    /** 실패 사운드 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TSoftObjectPtr<USoundBase> MissSound;

    /** 점수 설정 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoring")
    float PerfectScore = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoring")
    float GoodScore = 75.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoring")
    float HitScore = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoring")
    float MissScore = -25.0f;

    /** 콤보 시스템 */
    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    int32 CurrentCombo = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    int32 MaxCombo = 0;

    /** 콤보 배수 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoring")
    float ComboMultiplier = 1.2f;

    /** 요리 온도 상태 (너무 낮으면 안 익고, 너무 높으면 탄다) */
    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    float CookingTemperature = 0.5f;

    /** 최적 온도 범위 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Settings")
    float OptimalTempMin = 0.4f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Settings")
    float OptimalTempMax = 0.8f;

    /** 온도 변화 속도 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Settings")
    float TemperatureChangeRate = 0.1f;

    /** 메트로놈 타이머 핸들 */
    UPROPERTY()
    FTimerHandle MetronomeTimerHandle;

    /** 현재 노트의 메트로놈 틱 횟수 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm Settings", meta = (ClampMin = "2", ClampMax = "8"))
    int32 TicksPerNote = 4; // 각 노트당 4번의 틱 (1, 2, 3, Perfect!)

    /** 현재 노트의 현재 틱 번호 */
    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    int32 CurrentTick = 0;

    /** 현재 노트가 메트로놈을 재생 중인지 */
    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    bool bNoteMetronomeActive = false;

    /** 메트로놈 일반 틱 볼륨 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm Settings", meta = (ClampMin = "0.1", ClampMax = "2.0"))
    float MetronomeTickVolume = 0.7f; // 기본 70%

    /** 메트로놈 마지막 틱 볼륨 (Perfect 타이밍 강조) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm Settings", meta = (ClampMin = "0.1", ClampMax = "2.0"))
    float MetronomeLastTickVolume = 1.0f; // 기본 100%

public:
    // UCookingMinigameBase 인터페이스 구현
    virtual void StartMinigame(UCookingWidget* InWidget, AInteractablePot* InPot) override;
    virtual void UpdateMinigame(float DeltaTime) override;
    virtual void EndMinigame() override;
    virtual void HandlePlayerInput(const FString& InputType) override;
    virtual ECookingMinigameResult CalculateResult() const override;

    /**
     * 현재 활성화된 노트를 반환합니다
     */
    UFUNCTION(BlueprintPure, Category = "Rhythm")
    FRhythmNote GetCurrentNote() const;

    /**
     * 다음 노트까지의 시간을 반환합니다
     */
    UFUNCTION(BlueprintPure, Category = "Rhythm")
    float GetTimeToNextNote() const { return TimeToNextNote; }

    /**
     * 현재 콤보를 반환합니다
     */
    UFUNCTION(BlueprintPure, Category = "Rhythm")
    int32 GetCurrentCombo() const { return CurrentCombo; }

    /**
     * 현재 요리 온도를 반환합니다
     */
    UFUNCTION(BlueprintPure, Category = "Cooking")
    float GetCookingTemperature() const { return CookingTemperature; }

    /**
     * 온도가 최적 범위에 있는지 확인합니다
     */
    UFUNCTION(BlueprintPure, Category = "Cooking")
    bool IsTemperatureOptimal() const;

    /**
     * 현재 노트의 타이밍 정확도를 반환합니다 (0.0 = 완벽, 1.0 = 최악)
     */
    UFUNCTION(BlueprintPure, Category = "Rhythm")
    float GetCurrentNoteTiming() const { return CurrentNoteTiming; }

    /**
     * 메트로놈 사운드를 설정합니다 (InteractablePot에서 호출)
     */
    UFUNCTION(BlueprintCallable, Category = "Audio")
    void SetMetronomeSound(USoundBase* NewMetronomeSound);

    /**
     * 메트로놈 볼륨을 설정합니다 (InteractablePot에서 호출)
     */
    UFUNCTION(BlueprintCallable, Category = "Audio")
    void SetMetronomeVolume(float TickVolume, float LastTickVolume);

protected:
    /**
     * 리듬 노트를 생성합니다
     */
    UFUNCTION(BlueprintCallable, Category = "Rhythm")
    void GenerateRhythmNotes();

    /**
     * 현재 노트를 처리합니다
     */
    UFUNCTION(BlueprintCallable, Category = "Rhythm")
    void ProcessCurrentNote(const FString& InputType);

    /**
     * 콤보를 증가시킵니다
     */
    UFUNCTION(BlueprintCallable, Category = "Scoring")
    void IncreaseCombo();

    /**
     * 콤보를 리셋합니다
     */
    UFUNCTION(BlueprintCallable, Category = "Scoring")
    void ResetCombo();

    /**
     * 타이밍에 따른 결과를 계산합니다
     */
    UFUNCTION(BlueprintCallable, Category = "Rhythm")
    ERhythmEventResult CalculateTimingResult(float TimingDifference, const FRhythmNote& Note);

    /**
     * 요리 온도를 업데이트합니다
     */
    UFUNCTION(BlueprintCallable, Category = "Cooking")
    void UpdateCookingTemperature(float DeltaTime);

    /**
     * 적절한 사운드를 재생합니다
     */
    UFUNCTION(BlueprintCallable, Category = "Audio")
    void PlayResultSound(ERhythmEventResult Result);

    /**
     * 미스된 노트를 처리합니다
     */
    UFUNCTION(BlueprintCallable, Category = "Rhythm")
    void HandleMissedNote();

    /**
     * 노트 타입에서 액션 타입 문자열로 변환
     */
    UFUNCTION(BlueprintPure, Category = "Rhythm")
    FString GetActionTypeFromNoteType(ERhythmNoteType NoteType) const;

    /**
     * 현재 노트에 대한 메트로놈 시작
     */
    UFUNCTION(BlueprintCallable, Category = "Rhythm")
    void StartNoteMetronome(float NoteDuration);

    /**
     * 현재 노트의 메트로놈 정지
     */
    UFUNCTION(BlueprintCallable, Category = "Rhythm")
    void StopNoteMetronome();

    /**
     * 노트 메트로놈 틱 (실제 사운드 재생)
     */
    UFUNCTION()
    void PlayNoteMetronomeTick();

    /**
     * 다음 노트로 이동합니다.
     */
    void MoveToNextNote();
}; 