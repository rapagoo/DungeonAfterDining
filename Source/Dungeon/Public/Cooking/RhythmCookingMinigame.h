#pragma once

#include "CoreMinimal.h"
#include "Cooking/CookingMinigameBase.h"
#include "Engine/DataTable.h"
#include "RhythmCookingMinigame.generated.h"

// Forward declarations
class USoundBase;

/**
 * 리듬 이벤트 구조체
 */
USTRUCT(BlueprintType)
struct DUNGEON_API FRhythmEvent
{
    GENERATED_BODY()

    /** 이벤트가 발생할 시간 (초) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm")
    float TriggerTime = 0.0f;

    /** 이벤트 유형 (예: "Stir", "Flip", "Season") */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm")
    FString EventType = TEXT("Stir");

    /** 완벽한 타이밍 윈도우 (초) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm")
    float PerfectWindow = 0.2f;

    /** 좋은 타이밍 윈도우 (초) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm")
    float GoodWindow = 0.4f;

    /** 성공 타이밍 윈도우 (초) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm")
    float SuccessWindow = 0.6f;

    /** 이벤트가 처리되었는지 여부 */
    UPROPERTY(BlueprintReadOnly, Category = "Rhythm")
    bool bProcessed = false;

    FRhythmEvent()
    {
        TriggerTime = 0.0f;
        EventType = TEXT("Stir");
        PerfectWindow = 0.2f;
        GoodWindow = 0.4f;
        SuccessWindow = 0.6f;
        bProcessed = false;
    }
};

/**
 * 리듬 이벤트 판정 결과
 */
UENUM(BlueprintType)
enum class ERhythmEventResult : uint8
{
    None,
    Perfect,
    Good,
    Hit,
    Miss
};

/**
 * 리듬 기반 요리 미니게임
 * 플레이어가 비트에 맞춰 요리 동작을 수행하는 게임
 */
UCLASS(Blueprintable, BlueprintType)
class DUNGEON_API URhythmCookingMinigame : public UCookingMinigameBase
{
    GENERATED_BODY()

public:
    URhythmCookingMinigame();

protected:
    /** 리듬 이벤트 목록 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm Settings")
    TArray<FRhythmEvent> RhythmEvents;

    /** 현재 처리 중인 이벤트 인덱스 */
    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    int32 CurrentEventIndex = 0;

    /** 백그라운드 음악 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TSoftObjectPtr<USoundBase> BackgroundMusic;

    /** 성공 사운드 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TSoftObjectPtr<USoundBase> SuccessSound;

    /** 실패 사운드 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TSoftObjectPtr<USoundBase> FailSound;

    /** 완벽한 타이밍 보너스 점수 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoring")
    float PerfectScore = 100.0f;

    /** 좋은 타이밍 점수 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoring")
    float GoodScore = 75.0f;

    /** 일반 성공 점수 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoring")
    float HitScore = 50.0f;

    /** 연속 성공 배수 */
    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    int32 ComboMultiplier = 1;

    /** 현재 연속 성공 횟수 */
    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    int32 CurrentCombo = 0;

public:
    // UCookingMinigameBase 인터페이스 구현
    virtual void StartMinigame(UCookingWidget* InWidget, AInteractablePot* InPot) override;
    virtual void UpdateMinigame(float DeltaTime) override;
    virtual void EndMinigame() override;
    virtual void HandlePlayerInput(const FString& InputType) override;
    virtual ECookingMinigameResult CalculateResult() const override;

    /**
     * 특정 요리법에 맞는 리듬 이벤트를 생성합니다
     * @param CookingMethodName 요리법 이름
     * @param GameDuration 게임 지속 시간
     */
    UFUNCTION(BlueprintCallable, Category = "Rhythm Cooking")
    void GenerateRhythmEvents(const FString& CookingMethodName, float GameDuration);

    /**
     * 현재 활성화된 리듬 이벤트를 반환합니다
     */
    UFUNCTION(BlueprintPure, Category = "Rhythm Cooking")
    FRhythmEvent GetCurrentEvent() const;

    /**
     * 다음 이벤트까지의 시간을 반환합니다
     */
    UFUNCTION(BlueprintPure, Category = "Rhythm Cooking")
    float GetTimeToNextEvent() const;

    /**
     * 현재 콤보 횟수를 반환합니다
     */
    UFUNCTION(BlueprintPure, Category = "Rhythm Cooking")
    int32 GetCurrentCombo() const { return CurrentCombo; }

protected:
    /**
     * 점수를 추가합니다 (음수 방지 오버라이드)
     * @param Points 추가할 점수
     */
    virtual void AddScore(float Points) override;

    /**
     * 이벤트 타이밍을 판정합니다
     * @param Event 판정할 이벤트
     * @param InputTime 입력 시간
     * @return 판정 결과
     */
    UFUNCTION(BlueprintCallable, Category = "Rhythm Cooking")
    ERhythmEventResult JudgeEventTiming(const FRhythmEvent& Event, float InputTime);

    /**
     * 콤보를 증가시킵니다
     */
    UFUNCTION(BlueprintCallable, Category = "Rhythm Cooking")
    void IncrementCombo();

    /**
     * 콤보를 리셋합니다
     */
    UFUNCTION(BlueprintCallable, Category = "Rhythm Cooking")
    void ResetCombo();

    /**
     * 결과에 따라 점수를 계산합니다
     * @param Result 이벤트 판정 결과
     * @return 획득 점수
     */
    UFUNCTION(BlueprintPure, Category = "Rhythm Cooking")
    float CalculateEventScore(ERhythmEventResult Result) const;

    /**
     * 요리법에 따라 기본 리듬 패턴을 생성합니다
     * @param CookingMethod 요리법
     * @param Duration 총 시간
     */
    UFUNCTION(BlueprintCallable, Category = "Rhythm Cooking")
    void CreateRhythmPattern(const FString& CookingMethod, float Duration);
}; 