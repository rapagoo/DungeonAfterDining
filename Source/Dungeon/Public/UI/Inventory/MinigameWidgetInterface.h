// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MinigameWidgetInterface.generated.h"

class UCookingMinigameBase;

// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable)
class UMinigameWidgetInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * An interface for minigame widgets to allow the host widget (CookingWidget)
 * to communicate with them in a generic way.
 */
class DUNGEON_API IMinigameWidgetInterface
{
	GENERATED_BODY()

public:
	/** Called on a minigame widget right after it's created and added to the viewport. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Minigame Interface")
	void InitializeMinigameWidget(UCookingMinigameBase* Minigame);
	
	/** Called every time the minigame state is updated. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Minigame Interface")
	void UpdateMinigameWidget(float Score, int32 Phase);

	/** Called when a new action is required from the player. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Minigame Interface")
	void UpdateRequiredAction(const FString& ActionType, bool bIsRequired);

	/** Called by a minigame event to show a result like "Perfect", "Good", "Miss". */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Minigame Interface")
	void ShowEventResult(const FString& ResultText);
}; 