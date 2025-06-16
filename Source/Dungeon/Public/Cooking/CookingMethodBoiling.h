#pragma once

#include "CoreMinimal.h"
#include "Cooking/CookingMethodBase.h"
#include "GameplayTagContainer.h"
#include "CookingMethodBoiling.generated.h"

/**
 * 끓이기 요리법 클래스 - 타이머 기반 미니게임 사용
 */
UCLASS(Blueprintable, BlueprintType)
class DUNGEON_API UCookingMethodBoiling : public UCookingMethodBase
{
	GENERATED_BODY()

public:
	UCookingMethodBoiling();

	// UCookingMethodBase 인터페이스 구현
	virtual bool ProcessIngredients_Implementation(const TArray<FName>& Ingredients, UDataTable* RecipeDataTable, UDataTable* ItemDataTable, FName& OutCookedItemID, float& OutCookingDuration) override;
	virtual FText GetCookingMethodName_Implementation() const override;

protected:
	/** 기본 끓이기 시간 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boiling Settings")
	float BaseBoilingTime = 20.0f;

	/** 재료 개수에 따른 시간 추가 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boiling Settings")
	float TimePerIngredient = 3.0f;

	/** 최대 끓이기 시간 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boiling Settings")
	float MaxBoilingTime = 35.0f;

	/** 끓이기 온도 */
	UPROPERTY(EditDefaultsOnly, Category = "Boiling")
	float BoilingTemperature = 100.0f;

	// 끓이기 관련 사운드와 이펙트
	UPROPERTY(EditDefaultsOnly, Category = "Boiling|Visuals")
	TObjectPtr<UParticleSystem> BoilingBubbleEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Boiling|Sound")
	TObjectPtr<USoundBase> BoilingBubbleSound;
}; 