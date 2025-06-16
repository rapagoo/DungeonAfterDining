#pragma once

#include "CoreMinimal.h"
#include "Cooking/CookingMinigameBase.h"
#include "Engine/DataTable.h"
#include "Sound/SoundBase.h"
#include "TimerBasedCookingMinigame.generated.h"

/**
 * 원형 타이밍 이벤트의 결과를 나타내는 열거형
 */
UENUM(BlueprintType)
enum class ECircularEventResult : uint8
{
    None,       // 아직 결과 없음
    Success,    // 성공
    Failed,     // 실패
    Timeout     // 시간 초과
};

/**
 * 원형 타이밍 이벤트 구조체
 */
USTRUCT(BlueprintType)
struct DUNGEON_API FCircularTimingEvent
{
    GENERATED_BODY()

    /** 성공 구간의 시작 각도 (0° = 12시, 시계방향) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Settings")
    float SuccessArcStartAngle = 0.0f;

    /** 성공 구간의 끝 각도 (0° = 12시, 시계방향) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Settings")
    float SuccessArcEndAngle = 60.0f;

    /** 화살표의 회전 속도 (도/초) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Settings")
    float ArrowRotationSpeed = 180.0f;

    /** 현재 화살표의 각도 (0° = 12시, 시계방향) */
    UPROPERTY(BlueprintReadOnly, Category = "Event State")
    float CurrentArrowAngle = 0.0f;

    /** 이벤트가 활성화되어 있는지 여부 */
    UPROPERTY(BlueprintReadOnly, Category = "Event State")
    bool bIsActive = false;

    /** 이벤트가 완료되었는지 여부 */
    UPROPERTY(BlueprintReadOnly, Category = "Event State")
    bool bIsCompleted = false;

    /** 이벤트 시작 시간 (게임 시작 기준 상대 시간) */
    UPROPERTY(BlueprintReadOnly, Category = "Event State")
    float EventStartTime = 0.0f;

    /** 이벤트 최대 지속 시간 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Settings")
    float MaxEventDuration = 3.0f;

    /** 이벤트 결과 */
    UPROPERTY(BlueprintReadOnly, Category = "Event State")
    ECircularEventResult Result = ECircularEventResult::None;

    FCircularTimingEvent()
    {
        SuccessArcStartAngle = 0.0f;
        SuccessArcEndAngle = 60.0f;
        ArrowRotationSpeed = 180.0f;
        CurrentArrowAngle = 0.0f;
        bIsActive = false;
        bIsCompleted = false;
        EventStartTime = 0.0f;
        MaxEventDuration = 3.0f;
        Result = ECircularEventResult::None;
    }
};

/**
 * 타이머 기반 요리 미니게임
 * 메인 타이머가 진행되는 동안 랜덤하게 원형 타이밍 이벤트가 발생
 */
UCLASS(Blueprintable, BlueprintType)
class DUNGEON_API UTimerBasedCookingMinigame : public UCookingMinigameBase
{
    GENERATED_BODY()

public:
    UTimerBasedCookingMinigame();

protected:
    /** 전체 요리 시간 (초) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timer Settings")
    float TotalCookingTime = 20.0f;

    /** 남은 요리 시간 (초) */
    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    float RemainingTime = 20.0f;

    /** 실패 시 추가되는 시간 페널티 (초) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timer Settings")
    float TimePenalty = 3.0f;

    /** 현재 활성화된 원형 이벤트 */
    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    FCircularTimingEvent CurrentEvent;

    /** 다음 이벤트 발생까지의 시간 (초) */
    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    float TimeToNextEvent = 0.0f;

    /** 이벤트 발생 간격의 최소값 (초) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Settings")
    float MinEventInterval = 1.0f;

    /** 이벤트 발생 간격의 최대값 (초) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Settings")
    float MaxEventInterval = 4.0f;

    /** 성공 구간 각도의 최소값 (도) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Settings")
    float MinSuccessArcSize = 30.0f;

    /** 성공 구간 각도의 최대값 (도) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Settings")
    float MaxSuccessArcSize = 90.0f;

    /** 화살표 회전 속도의 최소값 (도/초) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Settings")
    float MinArrowSpeed = 120.0f;

    /** 화살표 회전 속도의 최대값 (도/초) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Settings")
    float MaxArrowSpeed = 240.0f;

    /** UI 회전 보정 각도 (UI 표시용, 12시 방향 기준) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Settings", meta = (DisplayName = "UI Rotation Offset"))
    float UIRotationOffset = -90.0f;

    /** 성공 구간 판정 허용 오차 (도) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Settings", meta = (DisplayName = "Success Zone Tolerance"))
    float SuccessZoneTolerance = 5.0f;

    /** 이벤트 타임아웃 여유시간 (초) - 360도 완주 시간에 추가됨 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event Settings", meta = (DisplayName = "Event Timeout Buffer"))
    float EventTimeoutBuffer = 1.0f;

    /** 성공한 이벤트 수 */
    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    int32 SuccessfulEvents = 0;

    /** 실패한 이벤트 수 */
    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    int32 FailedEvents = 0;

    /** 백그라운드 음악 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TSoftObjectPtr<USoundBase> BackgroundMusic;

    /** 이벤트 시작 사운드 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TSoftObjectPtr<USoundBase> EventStartSound;

    /** 성공 사운드 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TSoftObjectPtr<USoundBase> SuccessSound;

    /** 실패 사운드 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TSoftObjectPtr<USoundBase> FailSound;

    /** 타이머 완료 사운드 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TSoftObjectPtr<USoundBase> TimerCompleteSound;

public:
    // UCookingMinigameBase 인터페이스 구현
    virtual void StartMinigame(UCookingWidget* InWidget, AInteractablePot* InPot) override;
    virtual void UpdateMinigame(float DeltaTime) override;
    virtual void EndMinigame() override;
    virtual void HandlePlayerInput(const FString& InputType) override;

    /**
     * 현재 메인 타이머의 진행률을 반환합니다 (0.0 ~ 1.0)
     */
    UFUNCTION(BlueprintPure, Category = "Timer Game")
    float GetTimerProgress() const;

    /**
     * 현재 원형 이벤트 정보를 반환합니다
     */
    UFUNCTION(BlueprintPure, Category = "Timer Game")
    FCircularTimingEvent GetCurrentEvent() const { return CurrentEvent; }

    /**
     * 원형 이벤트가 활성화되어 있는지 확인합니다
     */
    UFUNCTION(BlueprintPure, Category = "Timer Game")
    bool IsEventActive() const { return CurrentEvent.bIsActive; }

    /**
     * 성공한 이벤트 수를 반환합니다
     */
    UFUNCTION(BlueprintPure, Category = "Timer Game")
    int32 GetSuccessfulEventCount() const { return SuccessfulEvents; }

    /**
     * 실패한 이벤트 수를 반환합니다
     */
    UFUNCTION(BlueprintPure, Category = "Timer Game")
    int32 GetFailedEventCount() const { return FailedEvents; }

    /**
     * 남은 시간을 반환합니다
     */
    UFUNCTION(BlueprintPure, Category = "Timer Game")
    float GetRemainingTime() const { return RemainingTime; }

protected:
    /**
     * 새로운 랜덤 이벤트를 생성합니다
     */
    UFUNCTION(BlueprintCallable, Category = "Timer Game")
    void GenerateRandomEvent();

    /**
     * 현재 화살표가 성공 구간에 있는지 확인합니다
     */
    UFUNCTION(BlueprintPure, Category = "Timer Game")
    bool IsArrowInSuccessZone() const;

    /**
     * 현재 이벤트를 완료 처리합니다
     */
    UFUNCTION(BlueprintCallable, Category = "Timer Game")
    void CompleteCurrentEvent(ECircularEventResult Result);

    /**
     * 시간 페널티를 적용합니다
     */
    UFUNCTION(BlueprintCallable, Category = "Timer Game")
    void ApplyTimePenalty();

    /**
     * 메인 타이머를 업데이트합니다
     */
    UFUNCTION(BlueprintCallable, Category = "Timer Game")
    void UpdateMainTimer(float DeltaTime);

    /**
     * 현재 이벤트를 업데이트합니다
     */
    UFUNCTION(BlueprintCallable, Category = "Timer Game")
    void UpdateCurrentEvent(float DeltaTime);

    /**
     * 다음 이벤트 발생 시간을 계산합니다
     */
    UFUNCTION(BlueprintCallable, Category = "Timer Game")
    void ScheduleNextEvent();

    /**
     * 사운드를 재생합니다
     */
    UFUNCTION(BlueprintCallable, Category = "Audio")
    void PlayEventSound(TSoftObjectPtr<USoundBase> Sound);

    /**
     * 각도를 0-360 범위로 정규화합니다
     */
    UFUNCTION(BlueprintPure, Category = "Timer Game")
    float NormalizeAngle(float Angle) const;
}; 