// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/SlotStruct.h" // For FSlotStruct
#include "ActionMenuWidget.generated.h"

// Forward Declarations
class UButton;
class UImage;
class ACharacter;
class UInventoryComponent;
class USlotWidget; // Forward declare SlotWidget
class AInventoryItemActor; // Forward declare InventoryItemActor

/**
 * Widget that appears when right-clicking an inventory item.
 */
UCLASS()
class DUNGEON_API UActionMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	// --- Bound Widgets --- 
	// Names must match the widget names in the UMG designer exactly (Is Variable=true)

	UPROPERTY(meta = (BindWidget))
	UButton* UseButton;

	UPROPERTY(meta = (BindWidget))
	UButton* DropButton;

	UPROPERTY(meta = (BindWidget))
	UButton* CancelButton;

	UPROPERTY(meta = (BindWidget))
	UImage* UseButtonHoverImage;

	UPROPERTY(meta = (BindWidget))
	UImage* DropButtonHoverImage;

	UPROPERTY(meta = (BindWidget))
	UImage* CancelButtonHoverImage;

	// Bind to the TextBlock inside Drop Button (set Is Variable=true in WBP_ActionMenu)
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* DropButtonText;

	// --- Data --- 

	// The item slot this menu is acting upon
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Action Menu Data")
	FSlotStruct ItemToActOn;

	// The index of the item slot in the inventory
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Action Menu Data")
	int32 ItemSlotIndex = -1;

	// Reference to the owning character
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Action Menu Data")
	ACharacter* OwnerCharacter;

	// Reference to the owning character's inventory component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Action Menu Data")
	UInventoryComponent* OwnerInventory;

	// Reference to the slot widget that created this menu
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Action Menu Data")
	USlotWidget* OwningSlotWidget;

protected:
	// The class of the item actor to spawn when dropping
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action Menu Config")
	TSubclassOf<AInventoryItemActor> ItemActorClass;

protected:
	// Called when the widget is constructed (after properties are initialized)
	virtual void NativeConstruct() override;

	// Called when the mouse leaves the widget area
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	// --- Button Callbacks --- 

	UFUNCTION()
	void OnUseButtonClicked();

	UFUNCTION()
	void OnDropButtonClicked();

	UFUNCTION()
	void OnPlaceButtonClicked();

	UFUNCTION()
	void OnCancelButtonClicked();

	// --- Hover Callbacks --- 

	UFUNCTION()
	void OnUseButtonHovered();
	UFUNCTION()
	void OnUseButtonUnhovered();

	UFUNCTION()
	void OnDropButtonHovered();
	UFUNCTION()
	void OnDropButtonUnhovered();

	UFUNCTION()
	void OnCancelButtonHovered();
	UFUNCTION()
	void OnCancelButtonUnhovered();

public:
	// Call this after creating the widget to set its context
	UFUNCTION(BlueprintCallable, Category = "Action Menu")
	void InitializeMenu(const FSlotStruct& InItem, int32 InSlotIndex, ACharacter* InOwnerCharacter, UInventoryComponent* InOwnerInventory, USlotWidget* InOwningSlotWidget);

}; 