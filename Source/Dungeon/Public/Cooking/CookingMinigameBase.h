#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/DataTable.h"
#include "CookingMinigameBase.generated.h"

// Forward declarations
class UCookingWidget;
class AInteractablePot;

/**
 * 요리 미니게임의 결과를 나타내는 열거형
 */
UENUM(BlueprintType)
enum class ECookingMinigameResult : uint8
{
    None,
    Perfect,    // 완벽한 성공
    Good,       // 좋은 성공
    Average,    // 평균적 성공
    Poor,       // 아쉬운 성공
    Failed      // 실패
};

/**
 * 요리 미니게임 단계를 나타내는 열거형
 */
UENUM(BlueprintType)
enum class ECookingMinigamePhase : uint8
{
    Preparation,    // 재료 준비 단계
    Cooking,        // 실제 조리 단계
    Timing,         // 타이밍 조절 단계
    Finishing       // 마무리 단계
};

/**
 * 미니게임 설정 구조체
 */
USTRUCT(BlueprintType)
struct DUNGEON_API FCookingMinigameSettings
{
    GENERATED_BODY()

    /** 미니게임의 총 지속 시간 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    float GameDuration = 10.0f;

    /** 성공 판정 기준 점수 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    float SuccessThreshold = 60.0f;

    /** Perfect 판정 기준 점수 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    float PerfectThreshold = 90.0f;

    /** 이벤트 발생 간격 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    float EventInterval = 2.0f;

    /** 반응 시간 한계 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    float ReactionTimeLimit = 1.5f;
};

/**
 * 요리 미니게임의 추상 베이스 클래스
 */
UCLASS(Blueprintable, BlueprintType, Abstract)
class DUNGEON_API UCookingMinigameBase : public UObject
{
    GENERATED_BODY()

public:
    UCookingMinigameBase();

protected:
    /** 현재 게임의 점수 */
    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    float CurrentScore = 0.0f;

    /** 현재 게임 단계 */
    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    ECookingMinigamePhase CurrentPhase = ECookingMinigamePhase::Preparation;

    /** 게임이 진행 중인지 여부 */
    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    bool bIsGameActive = false;

    /** 게임 시작 시간 */
    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    float GameStartTime = 0.0f;

    /** 미니게임 설정 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    FCookingMinigameSettings GameSettings;

    /** 연결된 요리 위젯 */
    UPROPERTY()
    TWeakObjectPtr<UCookingWidget> OwningWidget;

    /** 연결된 요리 냄비 */
    UPROPERTY()
    TWeakObjectPtr<AInteractablePot> OwningPot;

public:
    /**
     * 미니게임을 시작합니다
     * @param InWidget 소유하는 요리 위젯
     * @param InPot 소유하는 요리 냄비
     */
    UFUNCTION(BlueprintCallable, Category = "Cooking Minigame")
    virtual void StartMinigame(UCookingWidget* InWidget, AInteractablePot* InPot);

    /**
     * 미니게임을 업데이트합니다 (매 틱마다 호출)
     * @param DeltaTime 프레임 간 시간
     */
    UFUNCTION(BlueprintCallable, Category = "Cooking Minigame")
    virtual void UpdateMinigame(float DeltaTime);

    /**
     * 미니게임을 종료합니다
     */
    UFUNCTION(BlueprintCallable, Category = "Cooking Minigame")
    virtual void EndMinigame();

    /**
     * 플레이어 입력을 처리합니다
     * @param InputType 입력 유형 (버튼 이름 등)
     */
    UFUNCTION(BlueprintCallable, Category = "Cooking Minigame")
    virtual void HandlePlayerInput(const FString& InputType);

    /**
     * 현재 점수를 반환합니다
     */
    UFUNCTION(BlueprintPure, Category = "Cooking Minigame")
    float GetCurrentScore() const { return CurrentScore; }

    /**
     * 게임 결과를 계산합니다
     */
    UFUNCTION(BlueprintPure, Category = "Cooking Minigame")
    virtual ECookingMinigameResult CalculateResult() const;

    /**
     * 게임이 활성화되어 있는지 확인합니다
     */
    UFUNCTION(BlueprintPure, Category = "Cooking Minigame")
    bool IsGameActive() const { return bIsGameActive; }

    /**
     * 현재 게임 단계를 반환합니다
     */
    UFUNCTION(BlueprintPure, Category = "Cooking Minigame")
    ECookingMinigamePhase GetCurrentPhase() const { return CurrentPhase; }

protected:
    /**
     * 점수를 추가합니다
     * @param Points 추가할 점수
     */
    UFUNCTION(BlueprintCallable, Category = "Cooking Minigame")
    virtual void AddScore(float Points);

    /**
     * 다음 단계로 이동합니다
     */
    UFUNCTION(BlueprintCallable, Category = "Cooking Minigame")
    virtual void AdvancePhase();

    /**
     * 게임 종료를 위젯에 알립니다
     */
    UFUNCTION(BlueprintCallable, Category = "Cooking Minigame")
    virtual void NotifyGameEnd(ECookingMinigameResult Result);
}; 