// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/AllItemStruct.h" // Include FAllItemStruct definition
#include "InventoryComponent.generated.h"

// Forward declarations
class UUserWidget;
class UInputAction;
class UEnhancedInputComponent;
class AInventoryItemActor;
struct FSlotStruct;

// Declare the delegate
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryUpdated);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class DUNGEON_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UInventoryComponent();

	// Delegate to broadcast when inventory changes
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryUpdated OnInventoryUpdated;

	// --- Inventory Data ---

	// Holds all inventory slots using an array
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Inventory", meta=(SaveGame))
	TArray<FSlotStruct> InventorySlots;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	// --- Inventory UI ---

	// The class of the inventory widget to create (e.g., WBP_Inventory)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> InventoryWidgetClass;

	// The instance of the inventory widget
	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category = "UI")
	UUserWidget* InventoryWidgetInstance;

	// The class of the interact widget to create (e.g., WBP_Interact)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> InteractWidgetClass;

	// The instance of the interact widget
	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category = "UI")
	UUserWidget* InteractWidgetInstance;

	// --- Data --- // Renamed Category for clarity

	// The Data Table containing item definitions (e.g., DT_InvenItem)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Data")
	TSoftObjectPtr<UDataTable> ItemDataTable;

	// --- Input ---

	// Input Action for toggling the inventory UI
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* InventoryToggleAction;

	// Input Action for picking up items
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* PickupAction;

public:
	// Called by the owning actor to bind input actions
	void SetupInputBinding(UEnhancedInputComponent* PlayerInputComponent);

	// --- Functions ---

	// Toggles the inventory widget visibility and input mode
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ToggleInventory();

	// Attempts to pick up an item in front of the player
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void HandlePickup();

	// Adds an item to the inventory (specifically to the Eatables slot for now)
	// Returns true if the item was successfully added or stacked
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItem(const FSlotStruct& ItemToAdd);

	// Removes a specified quantity of an item from a specific slot
	// Returns true if removal was successful
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItemFromSlot(int32 SlotIndex, int32 QuantityToRemove = 1);

	// Updates the inventory UI widget with the current items
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void UpdateInventoryUI();

	/** Returns the created inventory widget instance (may be null) */
	UFUNCTION(BlueprintPure, Category = "UI")
	UUserWidget* GetInventoryWidgetInstance() const { return InventoryWidgetInstance; }

protected:
	// Helper function to trace for an item actor
	// Returns true if found, provides the item data and the actor itself
	bool TraceForItem(FSlotStruct& OutItem, AInventoryItemActor*& OutItemActor);
	
	// Called every frame - Moved to protected as it's unlikely needed publicly
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Removed BlueprintImplementableEvent - Will handle UI update differently
	// UFUNCTION(BlueprintImplementableEvent, Category = "UI", meta=(DisplayName="Update Interact Widget"))
	// void NotifyUpdateInteractWidget(bool bFoundItem, const FSlotStruct& ItemData);

};
