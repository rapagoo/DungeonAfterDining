// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/InvenItemEnum.h" // For EInventoryItemType
#include "Inventory/SlotStruct.h" // For FSlotStruct
#include "Components/WidgetSwitcher.h" // Include WidgetSwitcher
#include "InventoryWidget.generated.h"

// Forward declarations
class UButton;
class UImage;
class UWidgetSwitcher;
class UWrapBox;
class UItemInfoWidget;
class USlotWidget;
class AWarriorHeroCharacter; // Forward declare character
class UInventoryComponent; // Forward declare inventory component

/**
 * C++ base class for the WBP_Inventory widget.
 */
UCLASS()
class DUNGEON_API UInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	// --- Bound Widgets --- (Names must match the widget names in the UMG designer)
	UPROPERTY(meta = (BindWidget))
	UButton* EatableButton;
	// Add other tab buttons if they exist (e.g., WeaponButton, ArmorButton)

	UPROPERTY(meta = (BindWidget))
	UImage* EatableTabImage;
	// Add other tab images
	// Assuming corresponding Image widgets exist in UMG
	UPROPERTY(meta = (BindWidget))
	UImage* FoodTabImage;

	UPROPERTY(meta = (BindWidget))
	UImage* RecipesTabImage;

	UPROPERTY(meta = (BindWidget))
	UWidgetSwitcher* TabWidgetSwitcher;

	UPROPERTY(meta = (BindWidget))
	UWrapBox* EatablesWrapBox;
	// Add other WrapBoxes for different item types (e.g., WeaponsWrapBox)

	UPROPERTY(meta = (BindWidget))
	UWrapBox* FoodWrapBox;

	UPROPERTY(meta = (BindWidget))
	UWrapBox* RecipesWrapBox;

	UPROPERTY(meta = (BindWidget))
	UItemInfoWidget* WBP_ItemInfo; // Instance of our C++ Item Info Widget

	// --- Tab Styling --- 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tab Styling")
	FLinearColor HoveredColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tab Styling")
	FLinearColor ActivatedColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tab Styling")
	FLinearColor NotActivatedColor;

	// --- Slot Widget Class --- 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	TSubclassOf<UUserWidget> SlotWidgetClass;

	// --- References --- 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	AWarriorHeroCharacter* OwnerCharacter = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	UInventoryComponent* OwnerInventory = nullptr;

	// Bind the buttons for tab switching
	UPROPERTY(meta = (BindWidget))
	UButton* FoodButton;

	UPROPERTY(meta = (BindWidget))
	UButton* RecipesButton;

public:
	// Constructor
	UInventoryWidget(const FObjectInitializer& ObjectInitializer);

	// Function to set owner references (Call this after creating the widget)
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetOwnerReferences(AWarriorHeroCharacter* InOwnerCharacter, UInventoryComponent* InOwnerInventory);

	// Function called by InventoryComponent to update the UI
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void UpdateItemsInInventoryUI(const TArray<FSlotStruct>& AllItems);

	/** Returns the Item Info Widget subobject **/
	UFUNCTION(BlueprintPure, Category = "Inventory")
	UItemInfoWidget* GetItemInfoWidget() const { return WBP_ItemInfo; }

protected:
	// Native overrides
	virtual bool Initialize() override;
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	// Tab switching logic
	UFUNCTION()
	void SelectTab(int32 TabIndex);

	// Button Callbacks
	UFUNCTION()
	void OnEatableButtonClicked();

	UFUNCTION()
	void OnFoodButtonClicked();

	UFUNCTION()
	void OnRecipesButtonClicked();

	// Hover Callbacks
	UFUNCTION()
	void OnEatableButtonHovered();
	UFUNCTION()
	void OnEatableButtonUnhovered();
	// Add hover callbacks for other tab buttons
	UFUNCTION()
	void OnFoodButtonHovered();
	UFUNCTION()
	void OnFoodButtonUnhovered();
	UFUNCTION()
	void OnRecipesButtonHovered();
	UFUNCTION()
	void OnRecipesButtonUnhovered();
}; 