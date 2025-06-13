#pragma once

#include "CoreMinimal.h"
#include "Cooking/CookingMinigameBase.h"
#include "FryingTimerMinigame.generated.h"

class UFryingTimerMinigameWidget; // Forward declaration

/**
 * 타이밍 이벤트의 결과
 */
UENUM(BlueprintType)
enum class EFryingTimerEventResult : uint8
{
	None,
	Success,
	Fail,
	TooEarly
};

/**
 * 튀기기 타이머 미니게임에서 발생하는 랜덤 이벤트의 구조체
 */
USTRUCT(BlueprintType)
struct FFryingTimerEvent
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event")
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event")
	float Duration = 0.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event")
	float SuccessWindow = 0.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event")
	FString RequiredInput;
};

/**
 * 새로운 튀기기 미니게임 클래스.
 * 정해진 시간 동안 진행되며, 중간에 랜덤 타이밍 이벤트가 발생합니다.
 */
UCLASS()
class DUNGEON_API UFryingTimerMinigame : public UCookingMinigameBase
{
	GENERATED_BODY()

public:
	UFryingTimerMinigame();

	//~ Begin UCookingMinigameBase Interface
	virtual void StartMinigame(TWeakObjectPtr<UUserWidget> InWidget, AInteractablePot* InPot) override;
	virtual void UpdateMinigame(float DeltaTime) override;
	virtual void EndMinigame() override;
	virtual void HandlePlayerInput(const FString& InputType) override;
	//~ End UCookingMinigameBase Interface

protected:
	/** All the possible events for this minigame. */
	UPROPERTY(EditDefaultsOnly, Category = "Minigame Settings")
	TArray<FFryingTimerEvent> Events;

	/** Time between the end of one event and the start of the next. */
	UPROPERTY(EditDefaultsOnly, Category = "Minigame Settings")
	float TimeBetweenEvents = 2.0f;
	
	/** Total duration of the cooking process. */
	UPROPERTY(EditDefaultsOnly, Category = "Minigame Settings")
	float TotalCookTime = 30.0f;

private:
	/** 다음 이벤트 시간을 계산하고 설정합니다. */
	void SetUpNextEvent();

	/** 새로운 타이밍 이벤트를 시작합니다. */
	void TriggerEvent();

	/** 현재 진행중인 이벤트를 종료합니다. */
	void EndCurrentEvent();

private:
	UPROPERTY()
	TWeakObjectPtr<UFryingTimerMinigameWidget> FryingTimerWidget;
	
	UPROPERTY()
	int32 CurrentEventIndex = -1;

	UPROPERTY()
	FFryingTimerEvent CurrentEvent;

	UPROPERTY()
	bool bEventIsActive = false;
	
	UPROPERTY()
	float EventTimer = 0.0f;

	UPROPERTY()
	FTimerHandle EventTriggerHandle;
}; 