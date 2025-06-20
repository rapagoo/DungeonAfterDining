#pragma once

#include "CoreMinimal.h"
#include "Cooking/CookingMinigameBase.h"
#include "GameplayTagContainer.h"
#include "Sound/SoundBase.h"
#include "BoilingClickMinigame.generated.h"

/**
 * 클릭 타겟 구조체
 */
USTRUCT(BlueprintType)
struct DUNGEON_API FClickTarget
{
    GENERATED_BODY()

    /** 타겟이 나타나는 시간 (게임 시작 후 초) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing")
    float SpawnTime = 0.0f;

    /** 타겟의 화면 위치 (0.0~1.0 비율) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Position")
    FVector2D ScreenPosition = FVector2D(0.5f, 0.5f);

    /** 타겟을 클릭해야 하는 제한 시간 (초) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timing")
    float TimeLimit = 3.0f;

    /** 타겟이 활성화되었는지 */
    UPROPERTY(BlueprintReadOnly, Category = "State")
    bool bIsActive = false;

    /** 타겟이 클릭되었는지 */
    UPROPERTY(BlueprintReadOnly, Category = "State")
    bool bWasClicked = false;

    /** 타겟이 시간 초과되었는지 */
    UPROPERTY(BlueprintReadOnly, Category = "State")
    bool bTimedOut = false;

    FClickTarget()
    {
        SpawnTime = 0.0f;
        ScreenPosition = FVector2D(0.5f, 0.5f);
        TimeLimit = 3.0f;
        bIsActive = false;
        bWasClicked = false;
        bTimedOut = false;
    }
};

/**
 * 끓이기 반응속도 클릭 미니게임
 * 화면에 나타나는 원을 빠르게 클릭하는 게임
 */
UCLASS(Blueprintable, BlueprintType)
class DUNGEON_API UBoilingClickMinigame : public UCookingMinigameBase
{
    GENERATED_BODY()

public:
    UBoilingClickMinigame();

    // UCookingMinigameBase 인터페이스 구현
    virtual void StartMinigame(UCookingWidget* InWidget, AInteractablePot* InPot) override;
    virtual void UpdateMinigame(float DeltaTime) override;
    virtual void EndMinigame() override;
    virtual void HandlePlayerInput(const FString& InputType) override;

    /** 마우스 클릭 처리 (화면 좌표) */
    UFUNCTION(BlueprintCallable, Category = "Click Game")
    void HandleMouseClick(FVector2D ScreenPosition);

protected:
    /** 클릭 타겟 목록 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Click Game")
    TArray<FClickTarget> ClickTargets;

    /** 현재 활성화된 타겟 인덱스 */
    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    int32 CurrentTargetIndex = 0;

    /** 끓이기 총 시간 (초) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boiling Settings")
    float TotalBoilingTime = 30.0f;

    /** 타겟 간격 (초) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boiling Settings")
    float TargetInterval = 4.0f;

    /** 타겟 클릭 제한 시간 (초) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boiling Settings")
    float ClickTimeLimit = 3.0f;

    /** 실패 시 추가되는 요리 시간 (초) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Penalty")
    float FailurePenaltyTime = 5.0f;

    /** 타겟 원의 크기 (화면 비율) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual Settings")
    float TargetSize = 0.08f;

    /** 클릭 허용 범위 (타겟 크기의 배수) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual Settings")
    float ClickTolerance = 1.2f;

    /** 성공 사운드 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TSoftObjectPtr<USoundBase> SuccessSound;

    /** 실패 사운드 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TSoftObjectPtr<USoundBase> FailSound;

    /** 타겟 나타나는 사운드 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TSoftObjectPtr<USoundBase> SpawnSound;

    /** 끓는 배경 사운드 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TSoftObjectPtr<USoundBase> BoilingSound;

    /** 성공한 클릭 수 */
    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    int32 SuccessfulClicks = 0;

    /** 실패한 클릭 수 */
    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    int32 FailedClicks = 0;

    /** 추가된 요리 시간 (패널티) */
    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    float AddedCookingTime = 0.0f;

private:
    // 내부 함수들
    void GenerateClickTargets();
    void SpawnTarget(int32 TargetIndex);
    void HandleTargetTimeout(int32 TargetIndex);
    void HandleSuccessfulClick(int32 TargetIndex);
    void HandleFailedClick();
    FVector2D GenerateRandomScreenPosition() const;
    bool IsClickWithinTarget(FVector2D ClickPos, FVector2D TargetPos) const;
    void PlayResultSound(bool bSuccess);
    void UpdateUI();
}; 