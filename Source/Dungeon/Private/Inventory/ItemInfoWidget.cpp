// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/ItemInfoWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Engine/DataTable.h"
#include "Inventory/SlotStruct.h"
#include "Inventory/InvenItemStruct.h"
#include "Inventory/InvenItemEnum.h" // Include the enum definition

void UItemInfoWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	// Update the widget preview in the editor based on default Item data
	RefreshWidgetDisplay();
}

void UItemInfoWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Initial update when the widget is created at runtime
	// RefreshWidgetDisplay(); // Often called by SetItemAndUpdate initially
}

void UItemInfoWidget::RefreshWidgetDisplay()
{
	// Ensure all bound widgets are valid before accessing them
	if (!ItemName || !ItemDescriptionText || !PowerText || !ItemTypeImage)
	{
		UE_LOG(LogTemp, Error, TEXT("ItemInfoWidget: One or more bound widgets are invalid!"));
		return;
	}

	// Check if the Item RowName is valid
	if (Item.ItemID.RowName.IsValid() && !Item.ItemID.RowName.IsNone())
	{
		// Attempt to load the Data Table
		UDataTable* LoadedDT = ItemDataTable.LoadSynchronous();
		if (LoadedDT)
		{
			// Find the row using the C++ struct type
			const FString ContextString = TEXT("ItemInfoWidget Display");
			FInventoryItemStruct* ItemData = LoadedDT->FindRow<FInventoryItemStruct>(Item.ItemID.RowName, ContextString);

			if (ItemData)
			{
				// --- Row Found: Update Widgets --- 
				ItemName->SetText(ItemData->Name);
				ItemDescriptionText->SetText(ItemData->Description);
				PowerText->SetText(FText::AsNumber(ItemData->Power)); // Convert float to FText

				// Set Item Type Image based on the Item's type
				UTexture2D* TypeTexture = nullptr;
				switch (Item.ItemType) // Use ItemType from the FSlotStruct Item
				{
					case EInventoryItemType::EIT_Eatables:
						TypeTexture = EatablesItemTypeTexture.LoadSynchronous();
						break;
					// Add cases for Sword, Shield later if needed
					// case EInventoryItemType::EIT_Sword:
					// 	TypeTexture = SwordItemTypeTexture.LoadSynchronous();
					// 	break;
					// case EInventoryItemType::EIT_Shield:
					// 	TypeTexture = ShieldItemTypeTexture.LoadSynchronous();
					// 	break;
					default:
						// Keep TypeTexture null or set a default?
						break;
				}

				if (TypeTexture)
				{
					ItemTypeImage->SetBrushFromTexture(TypeTexture);
					ItemTypeImage->SetVisibility(ESlateVisibility::Visible);
				}
				else
				{
					// Hide image if no texture found/set for this type
					ItemTypeImage->SetBrush(FSlateNoResource()); // Clear brush
					ItemTypeImage->SetVisibility(ESlateVisibility::Hidden); 
				}
			}
			else
			{
				// Row not found in DataTable
				UE_LOG(LogTemp, Warning, TEXT("ItemInfoWidget: Row '%s' not found in DataTable '%s'. Clearing display."), *Item.ItemID.RowName.ToString(), *LoadedDT->GetName());
				// Clear UI Elements if row not found
				ItemName->SetText(FText::GetEmpty());
				ItemDescriptionText->SetText(FText::GetEmpty());
				PowerText->SetText(FText::GetEmpty());
				ItemTypeImage->SetBrush(FSlateNoResource());
				ItemTypeImage->SetVisibility(ESlateVisibility::Hidden);
			}
		}
		else
		{
			// Failed to load Data Table
			UE_LOG(LogTemp, Error, TEXT("ItemInfoWidget: Failed to load ItemDataTable '%s'."), *ItemDataTable.ToString());
			// Clear UI Elements if DT failed to load
			ItemName->SetText(FText::GetEmpty());
			ItemDescriptionText->SetText(FText::GetEmpty());
			PowerText->SetText(FText::GetEmpty());
			ItemTypeImage->SetBrush(FSlateNoResource());
			ItemTypeImage->SetVisibility(ESlateVisibility::Hidden);
		}
	}
	else
	{
		// ItemID is not valid (e.g., empty slot)
		// Clear UI Elements for empty slot
		ItemName->SetText(FText::GetEmpty());
		ItemDescriptionText->SetText(FText::GetEmpty());
		PowerText->SetText(FText::GetEmpty());
		ItemTypeImage->SetBrush(FSlateNoResource());
		ItemTypeImage->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UItemInfoWidget::SetItemAndUpdate(const FSlotStruct& NewItem)
{
	Item = NewItem;
	RefreshWidgetDisplay();
} 