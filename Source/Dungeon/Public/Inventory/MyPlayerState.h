#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Inventory/SlotStruct.h" // Corrected include for FSlotStruct
#include "MyPlayerState.generated.h"

UCLASS()
class DUNGEON_API AMyPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AMyPlayerState();

	// Removed PersistentInventorySlots - We will use GameInstance for transfer
	// UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Inventory", meta=(SaveGame))
	// TArray<FSlotStruct> PersistentInventorySlots;
}; 