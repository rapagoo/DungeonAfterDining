// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Cooking/CookingMinigameBase.h"
#include "GrillingMinigame.generated.h"

class UGrillingMinigameWidget;

UENUM(BlueprintType)
enum class EGrillingDoneness : uint8
{
	Raw,
	Rare,
	Medium,
	WellDone,
	Burnt
};

USTRUCT(BlueprintType)
struct FSideCookingProgress
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Grilling")
	float CookedAmount = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Grilling")
	EGrillingDoneness Doneness = EGrillingDoneness::Raw;
};

UCLASS(Blueprintable, BlueprintType)
class DUNGEON_API UGrillingMinigame : public UCookingMinigameBase
{
	GENERATED_BODY()

public:
	UGrillingMinigame();
	
	virtual void StartMinigame(TWeakObjectPtr<UUserWidget> InMinigameWidget, AInteractablePot* InPot) override;
	virtual void UpdateMinigame(float DeltaTime) override;
	virtual void EndMinigame() override;
	virtual void HandlePlayerInput(const FString& InputType) override;
	virtual ECookingMinigameResult CalculateResult() const override;

	// Getters for UI to pull data
	UFUNCTION(BlueprintPure, Category = "Grilling")
	float GetCurrentTemperature() const { return CurrentTemperature; }
	
	UFUNCTION(BlueprintPure, Category = "Grilling")
	bool IsFlipping() const { return bIsFlipping; }

	UFUNCTION(BlueprintPure, Category = "Grilling")
	const TArray<FSideCookingProgress>& GetSideProgress() const { return SideCookingProgress; }
	
	UFUNCTION(BlueprintPure, Category = "Grilling")
	int32 GetCurrentSideIndex() const { return CurrentSideIndex; }

private:
	EGrillingDoneness CalculateFinalDoneness() const;
	void UpdateSideDoneness(FSideCookingProgress& Side);
	void FlipItem();
	void StopFlip();

	UPROPERTY()
	TWeakObjectPtr<UGrillingMinigameWidget> GrillingWidget;
	
	UPROPERTY(EditDefaultsOnly, Category = "Minigame Settings")
	float HeatIncreaseAmount = 10.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Minigame Settings")
	float PassiveHeatLoss = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Minigame Settings")
	float OptimalTempMin = 50.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Minigame Settings")
	float OptimalTempMax = 80.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Minigame Settings")
	float FlipDuration = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Minigame Settings")
	float OptimalTempScorePerSecond = 5.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Minigame Settings")
	int32 NumberOfSides = 2;
	
	// State variables
	UPROPERTY(BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	float CurrentTemperature = 0.0f;
	
	UPROPERTY(BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	TArray<FSideCookingProgress> SideCookingProgress;
	
	UPROPERTY(BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	int32 CurrentSideIndex = 0;
	
	UPROPERTY(BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	bool bIsFlipping = false;
	
	UPROPERTY()
	FTimerHandle FlipTimerHandle;
}; 