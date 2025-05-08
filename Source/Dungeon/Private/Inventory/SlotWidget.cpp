// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/SlotWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "Inventory/SlotStruct.h"
#include "Inventory/InvenItemStruct.h"
#include "Inventory/InvenItemEnum.h"
#include "Inventory/ActionMenuWidget.h"
#include "Inventory/InventoryComponent.h"
#include "Characters/WarriorHeroCharacter.h"
#include "AbilitySystemComponent.h"          // For UAbilitySystemComponent
#include "GameplayEffect.h"               // For UGameplayEffect
#include "GameplayEffectTypes.h"          // For FGameplayEffectSpecHandle etc.
#include "Input/Events.h"                 // For FPointerEvent
#include "Blueprint/WidgetBlueprintLibrary.h" // For Detect Right Click
#include "Framework/Application/SlateApplication.h" // For GetCursorPos
#include "Inventory/ItemInfoWidget.h" // Include the full definition

void USlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
	// Any additional setup after properties are initialized
	// Button click events could be bound here if SlotButton exists and is used for Use action
	// if (SlotButton) SlotButton->OnClicked.AddDynamic(this, &USlotWidget::UseItemInSlot);

	// Initially hide the hover background
	if (HoverBorderImage)
	{
		HoverBorderImage->SetVisibility(ESlateVisibility::Hidden);
	}
}

void USlotWidget::InitializeSlot(const FSlotStruct& InItemData, int32 InSlotIndex, AWarriorHeroCharacter* InOwnerCharacter, UInventoryComponent* InOwnerInventory, UItemInfoWidget* InItemInfoWidget)
{
	ItemData = InItemData;
	SlotIndex = InSlotIndex;
	OwnerCharacter = InOwnerCharacter;
	OwnerInventory = InOwnerInventory;
	ItemInfoWidgetRef = InItemInfoWidget; // Store the reference

	UpdateSlotDisplay();

	UE_LOG(LogTemp, Verbose, TEXT("SlotWidget Initialized: Index %d, Item: %s, ItemInfoWidget: %s"), 
		SlotIndex, 
		*ItemData.ItemID.RowName.ToString(),
		ItemInfoWidgetRef ? *ItemInfoWidgetRef->GetName() : TEXT("None"));
}

void USlotWidget::UpdateSlotDisplay()
{
	if (!ItemImage || !QuantityText)
	{
		UE_LOG(LogTemp, Error, TEXT("SlotWidget: ItemImage or QuantityText is not bound!"));
		return;
	}

	// Check if the slot is empty (ItemID is None)
	if (ItemData.ItemID.RowName.IsNone() || ItemData.Quantity <= 0)
	{
		// Empty Slot: Hide icon, clear text
		ItemImage->SetBrush(FSlateNoResource());
		ItemImage->SetVisibility(ESlateVisibility::Hidden);
		QuantityText->SetText(FText::GetEmpty());
	}
	else
	{
		// Valid Item: Show icon and quantity
		QuantityText->SetText(FText::AsNumber(ItemData.Quantity));
		QuantityText->SetVisibility(ESlateVisibility::Visible);

		// Load the icon from the data table
		UTexture2D* IconTexture = nullptr;
		UDataTable* LoadedDT = ItemDataTable.LoadSynchronous();
		if (LoadedDT)
		{
			FInventoryItemStruct* ItemDef = LoadedDT->FindRow<FInventoryItemStruct>(ItemData.ItemID.RowName, TEXT("SlotDisplay"));
			if (ItemDef)
			{
				IconTexture = ItemDef->Thumbnail.LoadSynchronous();
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("SlotWidget: Could not find definition for %s in ItemDataTable."), *ItemData.ItemID.RowName.ToString());
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("SlotWidget: Failed to load ItemDataTable '%s'."), *ItemDataTable.ToString());
		}

		if (IconTexture)
		{
			ItemImage->SetBrushFromTexture(IconTexture);
			ItemImage->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			// Hide icon if texture loading failed
			ItemImage->SetBrush(FSlateNoResource());
			ItemImage->SetVisibility(ESlateVisibility::Hidden);
			UE_LOG(LogTemp, Warning, TEXT("SlotWidget: Failed to load icon texture for %s."), *ItemData.ItemID.RowName.ToString());
		}
	}
}

FReply USlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// Detect Right Mouse Button click
	if (InMouseEvent.IsMouseButtonDown(EKeys::RightMouseButton))
	{
		UE_LOG(LogTemp, Log, TEXT("Right-click detected on slot %d"), SlotIndex);

		// Check if the slot actually contains an item
		if (!ItemData.ItemID.RowName.IsNone() && ItemData.Quantity > 0 && ActionMenuClass && OwnerCharacter && OwnerInventory)
		{
			// Create the Action Menu widget
			UActionMenuWidget* ActionMenu = CreateWidget<UActionMenuWidget>(this, ActionMenuClass);
			if (ActionMenu)
			{
				// Initialize the menu with context
				ActionMenu->InitializeMenu(ItemData, SlotIndex, OwnerCharacter, OwnerInventory, this);

				// Add to viewport at mouse position
				// Get Mouse Position in Viewport requires Player Controller
				APlayerController* PC = GetOwningPlayer();
				if (PC)
				{
					FVector2D MousePosition;
					PC->GetMousePosition(MousePosition.X, MousePosition.Y);
					ActionMenu->SetPositionInViewport(MousePosition);
					ActionMenu->AddToViewport(10); // High Z-Order to appear on top
					UE_LOG(LogTemp, Log, TEXT("Action Menu created and added to viewport."));
					return FReply::Handled(); // We handled the click
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("Failed to get Player Controller for Action Menu positioning."));
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to create Action Menu Widget."));
			}
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("Right-click on empty slot or invalid context, ignoring."));
		}
	}

	// If not right-click or not handled, pass to base or return Unhandled
	// Returning Unhandled allows other potential handlers (like drag/drop) to process the event.
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent); 
}

void USlotWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	// Show hover background
	if (HoverBorderImage)
	{
		HoverBorderImage->SetVisibility(ESlateVisibility::Visible);
	}

	// Update and show Item Info if the slot is not empty
	if (ItemInfoWidgetRef && !ItemData.ItemID.RowName.IsNone() && ItemData.Quantity > 0)
	{
		ItemInfoWidgetRef->SetItemAndUpdate(ItemData); // Update with current item
		ItemInfoWidgetRef->SetVisibility(ESlateVisibility::Visible);
	}
	else if (ItemInfoWidgetRef) // Hide if slot is empty
	{
		ItemInfoWidgetRef->SetVisibility(ESlateVisibility::Hidden);
	}
}

void USlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	// Hide hover background
	if (HoverBorderImage)
	{
		HoverBorderImage->SetVisibility(ESlateVisibility::Hidden);
	}

	// Hide Item Info
	if (ItemInfoWidgetRef)
	{
		ItemInfoWidgetRef->SetVisibility(ESlateVisibility::Hidden);
	}
}

void USlotWidget::UseItemInSlot()
{
	UE_LOG(LogTemp, Log, TEXT("UseItemInSlot called for index %d, item %s"), SlotIndex, *ItemData.ItemID.RowName.ToString());

	// --- Input Validation ---
	if (!OwnerCharacter || !OwnerInventory || SlotIndex < 0 || ItemData.ItemID.RowName.IsNone() || ItemData.Quantity <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("UseItemInSlot: Invalid context or item data."));
		return;
	}
	UAbilitySystemComponent* ASC = OwnerCharacter->GetAbilitySystemComponent(); // Assuming GetAbilitySystemComponent exists on Character
	if (!ASC)
	{
		UE_LOG(LogTemp, Error, TEXT("UseItemInSlot: Could not get Ability System Component from OwnerCharacter."));
		return;
	}
	UDataTable* LoadedDT = ItemDataTable.LoadSynchronous();
	if (!LoadedDT)
	{
		UE_LOG(LogTemp, Error, TEXT("UseItemInSlot: Failed to load ItemDataTable: %s"), *ItemDataTable.ToString());
		return;
	}
	FInventoryItemStruct* ItemDef = LoadedDT->FindRow<FInventoryItemStruct>(ItemData.ItemID.RowName, TEXT("UseItem"));
	if (!ItemDef)
	{
		UE_LOG(LogTemp, Error, TEXT("UseItemInSlot: Could not find item definition for %s."), *ItemData.ItemID.RowName.ToString());
		return;
	}

	// --- Item Usage Logic (Specifically for Eatables based on BP) ---
	if (ItemData.ItemType == EInventoryItemType::EIT_Eatables || ItemData.ItemType == EInventoryItemType::EIT_Food)
	{
		UE_LOG(LogTemp, Log, TEXT("Using Eatables or Food item."));

		if (!HealEffectClass)
		{
			UE_LOG(LogTemp, Error, TEXT("UseItemInSlot: HealEffectClass is not set."));
			return;
		}

		// 1. Create Gameplay Effect Spec
		FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
		EffectContext.AddSourceObject(this); // The slot widget is the source
		FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(HealEffectClass, 1.0f, EffectContext);

		if (SpecHandle.IsValid())
		{
			// 2. Assign Tag Magnitude
			SpecHandle.Data->SetSetByCallerMagnitude(HealAmountTag, ItemDef->Power); // Use Power from Item Def
			UE_LOG(LogTemp, Log, TEXT("Applying Heal effect with Power: %f"), ItemDef->Power);

			// 3. Apply Gameplay Effect
			FActiveGameplayEffectHandle AppliedHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

			if (AppliedHandle.WasSuccessfullyApplied())
			{
				UE_LOG(LogTemp, Log, TEXT("Successfully applied Gameplay Effect."));

				// 4. Update Inventory (Decrement Quantity or Remove)
				if (OwnerInventory->InventorySlots.IsValidIndex(SlotIndex))
				{
					OwnerInventory->InventorySlots[SlotIndex].Quantity--;
					UE_LOG(LogTemp, Log, TEXT("Decremented quantity for slot %d. New quantity: %d"), SlotIndex, OwnerInventory->InventorySlots[SlotIndex].Quantity);

					// If quantity reaches zero, clear the slot
					if (OwnerInventory->InventorySlots[SlotIndex].Quantity <= 0)
					{
						OwnerInventory->InventorySlots[SlotIndex] = FSlotStruct(); // Reset to empty
						UE_LOG(LogTemp, Log, TEXT("Cleared slot %d as quantity reached zero."), SlotIndex);
					}
					
					// 5. Update Inventory UI
					OwnerInventory->UpdateInventoryUI();
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("UseItemInSlot: SlotIndex %d is invalid after applying effect."), SlotIndex);
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to apply Gameplay Effect."));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to make Gameplay Effect Spec."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UseItemInSlot: Item type is not Eatables or Food, usage not implemented for type %s."), *UEnum::GetValueAsString(ItemData.ItemType));
	}
} 