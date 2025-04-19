// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/SlotStruct.h" // For FSlotStruct
#include "GameplayTagContainer.h" // For Gameplay Tags
#include "SlotWidget.generated.h"

// Forward Declarations
class UImage;
class UTextBlock;
class UButton;
class UDataTable;
class AWarriorHeroCharacter;
class UInventoryComponent;
class UActionMenuWidget;
class UGameplayEffect;
class UItemInfoWidget;

/**
 * Represents a single slot in the inventory UI.
 */
UCLASS()
class DUNGEON_API USlotWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	// --- Bound Widgets --- 
	// Names must match the widget names in the UMG designer exactly (Is Variable=true)

	UPROPERTY(meta = (BindWidget))
	UImage* ItemImage;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* QuantityText;

	// Assuming the root or a main clickable element is a Button named SlotButton
	UPROPERTY(meta = (BindWidget))
	UButton* ItemButton;

	// Background image shown on hover (Ensure this matches the UMG variable name)
	UPROPERTY(meta = (BindWidget))
	UImage* HoverBorderImage;

	// --- Slot Data --- 

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Slot Data")
	FSlotStruct ItemData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Slot Data")
	int32 SlotIndex = -1;

	// --- References & Config --- 

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Slot Data")
	AWarriorHeroCharacter* OwnerCharacter;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Slot Data")
	UInventoryComponent* OwnerInventory;

	// Data table containing item definitions (passed during initialization or set as default)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Slot Config")
	TSoftObjectPtr<UDataTable> ItemDataTable;

	// The Action Menu widget to spawn on right-click
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Slot Config")
	TSubclassOf<UActionMenuWidget> ActionMenuClass;

	// Gameplay Effect to apply when using Eatables
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Slot Config | Effects")
	TSubclassOf<UGameplayEffect> HealEffectClass;

	// Gameplay Tag for HealAmount magnitude
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Slot Config | Effects")
	FGameplayTag HealAmountTag;

private:
	// Reference to the Item Info widget managed by the parent Inventory Widget
	UPROPERTY() // No need for Edit/Visible flags, just keep the reference
	UItemInfoWidget* ItemInfoWidgetRef = nullptr;

protected:
	// Called when the widget is constructed
	virtual void NativeConstruct() override;

	// Handles mouse button down events, especially right-click for action menu
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// Function to update the slot's visual appearance based on ItemData
	virtual void UpdateSlotDisplay();

	// Handle mouse entering the widget bounds
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// Handle mouse leaving the widget bounds
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

public:
	// Call this to set the data for this slot widget
	UFUNCTION(BlueprintCallable, Category = "Slot Functions")
	void InitializeSlot(const FSlotStruct& InItemData, int32 InSlotIndex, AWarriorHeroCharacter* InOwnerCharacter, UInventoryComponent* InOwnerInventory, UItemInfoWidget* InItemInfoWidget);

	// Handles the logic for using the item in this slot
	UFUNCTION(BlueprintCallable, Category = "Slot Functions") // Allow BP override/call if needed
	void UseItemInSlot();

}; 