#pragma once

#include "CoreMinimal.h"
#include "Cooking/CookingMinigameBase.h"
#include "Engine/DataTable.h"
#include "GrillingMinigame.generated.h"

// Forward declarations
class USoundBase;

/**
 * 굽기 이벤트 타입
 */
UENUM(BlueprintType)
enum class EGrillingEventType : uint8
{
    None,
    Flip,           // 뒤집기
    AdjustHeat,     // 화력 조절
    CheckDoneness   // 익힘 정도 확인
};

/**
 * 굽기 면 상태
 */
UENUM(BlueprintType)
enum class EGrillingSide : uint8
{
    FirstSide,      // 첫 번째 면
    SecondSide,     // 두 번째 면
    Completed       // 완료
};

/**
 * 굽기 정도
 */
UENUM(BlueprintType)
enum class EGrillingDoneness : uint8
{
    Raw,            // 생것
    Rare,           // 레어
    MediumRare,     // 미디움 레어
    Medium,         // 미디움
    MediumWell,     // 미디움 웰
    WellDone,       // 웰던
    Burnt           // 탄 것
};

/**
 * 굽기 이벤트 구조체
 */
USTRUCT(BlueprintType)
struct DUNGEON_API FGrillingEvent
{
    GENERATED_BODY()

    /** 이벤트 타입 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grilling")
    EGrillingEventType EventType = EGrillingEventType::None;

    /** 이벤트 시작 시간 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grilling")
    float StartTime = 0.0f;

    /** 이벤트 지속 시간 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grilling")
    float Duration = 2.0f;

    /** 최적 타이밍 윈도우 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grilling")
    float OptimalWindow = 0.5f;

    /** 허용 가능한 타이밍 윈도우 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grilling")
    float AcceptableWindow = 1.0f;

    /** 이벤트가 처리되었는지 여부 */
    UPROPERTY(BlueprintReadOnly, Category = "Grilling")
    bool bProcessed = false;

    FGrillingEvent()
    {
        EventType = EGrillingEventType::None;
        StartTime = 0.0f;
        Duration = 2.0f;
        OptimalWindow = 0.5f;
        AcceptableWindow = 1.0f;
        bProcessed = false;
    }
};

/**
 * 굽기 미니게임
 * 플레이어가 적절한 타이밍에 뒤집고 화력을 조절하는 게임
 */
UCLASS(Blueprintable, BlueprintType)
class DUNGEON_API UGrillingMinigame : public UCookingMinigameBase
{
    GENERATED_BODY()

public:
    UGrillingMinigame();

protected:
    /** 현재 굽고 있는 면 */
    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    EGrillingSide CurrentSide = EGrillingSide::FirstSide;

    /** 현재 굽기 정도 */
    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    EGrillingDoneness CurrentDoneness = EGrillingDoneness::Raw;

    /** 현재 화력 레벨 (0.0 ~ 1.0) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grilling State")
    float HeatLevel;

    /** Optimal heat level for cooking */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grilling Settings")
    float OptimalHeatLevel;

    /** 각 면의 굽기 진행도 (0.0 ~ 1.0) */
    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    TArray<float> SideCookingProgress;

    /** 굽기 이벤트 목록 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grilling Settings")
    TArray<FGrillingEvent> GrillingEvents;

    /** 현재 처리 중인 이벤트 인덱스 */
    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    int32 CurrentEventIndex = 0;

    /** 뒤집기 사운드 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TSoftObjectPtr<USoundBase> FlipSound;

    /** 지글지글 사운드 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TSoftObjectPtr<USoundBase> SizzleSound;

    /** 탄 냄새 사운드 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TSoftObjectPtr<USoundBase> BurningSound;

    /** 기본 굽기 속도 (초당 진행도) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grilling Settings")
    float BaseCookingSpeed = 0.1f;

    /** 화력에 따른 굽기 속도 배수 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grilling Settings")
    float HeatMultiplier = 2.0f;

    /** 완벽한 뒤집기 보너스 점수 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoring")
    float PerfectFlipScore = 50.0f;

    /** 좋은 뒤집기 점수 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoring")
    float GoodFlipScore = 30.0f;

    /** 화력 조절 보너스 점수 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoring")
    float HeatControlScore = 20.0f;

public:
    // UCookingMinigameBase 인터페이스 구현
    virtual void StartMinigame(UCookingWidget* InWidget, AInteractablePot* InPot) override;
    virtual void UpdateMinigame(float DeltaTime) override;
    virtual void EndMinigame() override;
    virtual void HandlePlayerInput(const FString& InputType) override;
    virtual ECookingMinigameResult CalculateResult() const override;

    /**
     * 현재 굽고 있는 면을 반환합니다
     */
    UFUNCTION(BlueprintPure, Category = "Grilling")
    EGrillingSide GetCurrentSide() const { return CurrentSide; }

    /**
     * 현재 굽기 정도를 반환합니다
     */
    UFUNCTION(BlueprintPure, Category = "Grilling")
    EGrillingDoneness GetCurrentDoneness() const { return CurrentDoneness; }

    /**
     * 현재 화력 레벨을 반환합니다
     */
    UFUNCTION(BlueprintPure, Category = "Grilling")
    float GetHeatLevel() const { return HeatLevel; }

    /**
     * 현재 면의 굽기 진행도를 반환합니다
     */
    UFUNCTION(BlueprintPure, Category = "Grilling")
    float GetCurrentSideProgress() const;

    /**
     * 현재 활성화된 이벤트를 반환합니다
     */
    UFUNCTION(BlueprintPure, Category = "Grilling")
    FGrillingEvent GetCurrentEvent() const;

protected:
    /**
     * 굽기 이벤트를 생성합니다
     */
    UFUNCTION(BlueprintCallable, Category = "Grilling")
    void GenerateGrillingEvents();

    /**
     * 음식을 뒤집습니다
     */
    UFUNCTION(BlueprintCallable, Category = "Grilling")
    void FlipFood();

    /**
     * 화력을 조절합니다
     * @param Delta 화력 변화량 (-1.0 ~ 1.0)
     */
    UFUNCTION(BlueprintCallable, Category = "Grilling")
    void AdjustHeat(float Delta);

    /**
     * 굽기 진행도를 업데이트합니다
     * @param DeltaTime 프레임 간 시간
     */
    UFUNCTION(BlueprintCallable, Category = "Grilling")
    void UpdateCookingProgress(float DeltaTime);

    /**
     * 굽기 정도를 계산합니다
     */
    UFUNCTION(BlueprintCallable, Category = "Grilling")
    void CalculateDoneness();

    /**
     * 뒤집기 타이밍을 판정합니다
     * @param Event 판정할 이벤트
     * @return 성공 여부
     */
    UFUNCTION(BlueprintCallable, Category = "Grilling")
    bool JudgeFlipTiming(const FGrillingEvent& Event);

    /**
     * 최종 굽기 품질을 계산합니다
     */
    UFUNCTION(BlueprintPure, Category = "Grilling")
    float CalculateGrillingQuality() const;

private:
    /**
     * 이벤트 타임아웃 처리
     */
    void OnEventTimeout(const FGrillingEvent& Event);
}; 