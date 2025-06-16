#pragma once

#include "CoreMinimal.h"
#include "Cooking/CookingMinigameBase.h"
#include "TimerMinigame.generated.h"

class UCookingWidget;
class AInteractablePot;

USTRUCT(BlueprintType)
struct DUNGEON_API FTimerEvent
{
	GENERATED_BODY()

	// 이벤트가 활성화되는 시간 (게임 시작 후 초)
	UPROPERTY(BlueprintReadOnly, Category = "TimerEvent")
	float TriggerTime = 0.0f;

	// 성공 구간 시작 각도 (0-360)
	UPROPERTY(BlueprintReadOnly, Category = "TimerEvent")
	float SuccessStartAngle = 0.0f;

	// 성공 구간 끝 각도 (0-360)
	UPROPERTY(BlueprintReadOnly, Category = "TimerEvent")
	float SuccessEndAngle = 0.0f;

	// 이 이벤트가 현재 활성화되었는지 여부
	UPROPERTY(BlueprintReadOnly, Category = "TimerEvent")
	bool bIsActive = false;

	// 이 이벤트가 완료되었는지 여부
	UPROPERTY(BlueprintReadOnly, Category = "TimerEvent")
	bool bIsCompleted = false;

	// 이벤트 처리 결과
	UPROPERTY(BlueprintReadOnly, Category = "TimerEvent")
	bool bSuccess = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimerEventSpawned, const FTimerEvent&, EventData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimerEventCompleted, bool, bSuccess);


UCLASS(Blueprintable, BlueprintType)
class DUNGEON_API UTimerMinigame : public UCookingMinigameBase
{
	GENERATED_BODY()

public:
	UTimerMinigame();

	// UCookingMinigameBase interface
	virtual void StartMinigame(UCookingWidget* InWidget, AInteractablePot* InPot) override;
	virtual void UpdateMinigame(float DeltaTime) override;
	virtual void EndMinigame() override;
	virtual void HandlePlayerInput(const FString& InputType) override;
	// ~UCookingMinigameBase interface

	UPROPERTY(BlueprintAssignable, Category = "TimerMinigame|Delegates")
	FOnTimerEventSpawned OnTimerEventSpawned;
	
	UPROPERTY(BlueprintAssignable, Category = "TimerMinigame|Delegates")
	FOnTimerEventCompleted OnTimerEventCompleted;

protected:
	// 전체 요리 기본 시간
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimerMinigame|Settings")
	float BaseCookTime = 20.0f;

	// 실패 시 추가될 시간
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimerMinigame|Settings")
	float TimePenaltyOnFail = 3.0f;

	// 화살표 회전 속도 (degrees per second)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimerMinigame|Settings")
	float ArrowRotationSpeed = 180.0f;

	// 이벤트 성공 구간의 크기 (각도)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimerMinigame|Settings", meta = (ClampMin = "10.0", ClampMax = "90.0"))
	float SuccessZoneSize = 45.0f;

	// 총 이벤트 발생 횟수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimerMinigame|Settings", meta = (ClampMin = "1", ClampMax = "10"))
	int32 NumberOfEvents = 5;

private:
	// 현재 누적된 요리 시간
	UPROPERTY(BlueprintReadOnly, Category = "TimerMinigame|State", meta = (AllowPrivateAccess = "true"))
	float CurrentCookTime = 0.0f;

	// 패널티를 포함한 전체 요리 시간
	UPROPERTY(BlueprintReadOnly, Category = "TimerMinigame|State", meta = (AllowPrivateAccess = "true"))
	float TotalCookTime = 0.0f;

	// 현재 화살표 각도
	UPROPERTY(BlueprintReadOnly, Category = "TimerMinigame|State", meta = (AllowPrivateAccess = "true"))
	float CurrentArrowAngle = 0.0f;

	// 생성된 이벤트 목록
	UPROPERTY()
	TArray<FTimerEvent> Events;

	// 현재 처리 중인 이벤트 인덱스
	int32 CurrentEventIndex = -1;

	void GenerateEvents();
	void CompleteCurrentEvent(bool bSuccess);
}; 