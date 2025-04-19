// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/SlotStruct.h" // Include FSlotStruct
#include "Inventory/InvenItemStruct.h" // Include FInventoryItemStruct for DT Row
#include "Engine/DataTable.h"     // Include UDataTable
#include "ItemInfoWidget.generated.h"

// Forward Declarations
class UTextBlock;
class UImage;
class UTexture2D;

/**
 * C++ base class for the WBP_ItemInfo widget.
 */
UCLASS()
class DUNGEON_API UItemInfoWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	// --- Bound Widgets --- 
	// Names must match the widget names in the UMG designer exactly (Is Variable=true)

	UPROPERTY(meta = (BindWidget))
	UTextBlock* ItemName;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* ItemDescriptionText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* PowerText;
	
	UPROPERTY(meta = (BindWidget))
	UImage* ItemTypeImage;

	// Note: PowerSizeBox wasn't bound as its purpose is unclear (container?)
	// Bind it if needed, e.g., UPROPERTY(meta = (BindWidget)) class USizeBox* PowerSizeBox;

	// --- Data --- 

	// The item slot data this widget displays
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget Data", meta=(ExposeOnSpawn=true)) // ExposeOnSpawn allows setting when creating widget
	FSlotStruct Item;

	// Data table containing item definitions
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Widget Data")
	TSoftObjectPtr<UDataTable> ItemDataTable;

	// Texture for the Eatables item type
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Widget Data | Textures")
	TSoftObjectPtr<UTexture2D> EatablesItemTypeTexture;

	// Add textures for other types if needed later
	// UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Widget Data | Textures")
	// TSoftObjectPtr<UTexture2D> SwordItemTypeTexture;
	// UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Widget Data | Textures")
	// TSoftObjectPtr<UTexture2D> ShieldItemTypeTexture;

protected:
	// Called during widget construction in the editor and at runtime
	virtual void NativePreConstruct() override;
	// Called when the widget is constructed at runtime (after PreConstruct)
	virtual void NativeConstruct() override;

	// Updates the widget's appearance based on the Item data
	UFUNCTION(BlueprintCallable, Category = "Widget Functions")
	virtual void RefreshWidgetDisplay(); // Made virtual for potential BP override

public:
	// Allows external objects to set the item data and refresh the widget
	UFUNCTION(BlueprintCallable, Category = "Widget Functions")
	void SetItemAndUpdate(const FSlotStruct& NewItem);

}; 