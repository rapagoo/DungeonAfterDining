#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Inventory/SlotStruct.h"
#include "MyGameInstance.generated.h"

UCLASS()
class DUNGEON_API UMyGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	UFUNCTION() // Delegate binding requires UFUNCTION()
	virtual void HandlePreLoadMap(const FString& MapName);

	// Temporary storage for inventory items during level transition
	UPROPERTY(VisibleAnywhere, Category = "Inventory Transfer")
	TArray<FSlotStruct> TempInventoryToTransfer;
}; 