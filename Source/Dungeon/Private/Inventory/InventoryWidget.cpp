// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/InventoryWidget.h"
#include "Components/Button.h"
#include "Components/WrapBox.h"
#include "Components/Image.h"
#include "Components/WidgetSwitcher.h"
#include "Inventory/ItemInfoWidget.h" // Include our C++ Item Info Widget
#include "Blueprint/UserWidget.h"      // For SlotWidgetClass base
#include "Inventory/SlotStruct.h"       // For FSlotStruct
// Include your actual Slot Widget header if it has specific functions to call
#include "Inventory/SlotWidget.h"
#include "GameFramework/PlayerController.h" // For Input Mode setting
#include "Input/Events.h" // For FKeyEvent, FReply
#include "Framework/Application/SlateApplication.h" // For SetFocusToGameViewport
#include "Kismet/GameplayStatics.h" // For GetPlayerController

UInventoryWidget::UInventoryWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Initialize Colors using FColor::FromHex - Note: Alpha is first (AARRGGBB)
	// HoveredColor: FF282899 -> 992828FF
	HoveredColor = FColor::FromHex("992828FF").ReinterpretAsLinear();
	// ActivatedColor: FFFFFFFF -> FFFFFFFF
	ActivatedColor = FColor::FromHex("FFFFFFFF").ReinterpretAsLinear(); // Or FLinearColor::White
	// NotActivatedColor: FFFFFF4D -> 4DFFFFFF
	NotActivatedColor = FColor::FromHex("4DFFFFFF").ReinterpretAsLinear();

	// Set default for SlotWidgetClass (important to set a valid class in BP defaults)
	SlotWidgetClass = nullptr; 

	// Allow this widget to receive focus when UI input mode is set
	bIsFocusable = true;
}

void UInventoryWidget::SetOwnerReferences(AWarriorHeroCharacter* InOwnerCharacter, UInventoryComponent* InOwnerInventory)
{
	OwnerCharacter = InOwnerCharacter;
	OwnerInventory = InOwnerInventory;
}

void UInventoryWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	// Update tab visuals based on the initial active index in the editor preview
	if (TabWidgetSwitcher)
	{
		SelectTab(TabWidgetSwitcher->GetActiveWidgetIndex());
	}
}

void UInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Bind button clicks and hover events
	if (EatableButton)
	{
		EatableButton->OnClicked.AddDynamic(this, &UInventoryWidget::OnEatableButtonClicked);
		EatableButton->OnHovered.AddDynamic(this, &UInventoryWidget::OnEatableButtonHovered);
		EatableButton->OnUnhovered.AddDynamic(this, &UInventoryWidget::OnEatableButtonUnhovered);
	}

	// Optional: Ensure the first tab is selected on construct if needed
	// SelectTab(2); // Or maybe SelectTab(2) if Eatables is default?
}

FReply UInventoryWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// Check for Tab key to switch tabs (Simple example: cycle forward)
	if (InKeyEvent.GetKey() == EKeys::Tab)
	{
		if (TabWidgetSwitcher)
		{
			int32 CurrentIndex = TabWidgetSwitcher->GetActiveWidgetIndex();
			int32 NextIndex = (CurrentIndex + 1) % TabWidgetSwitcher->GetNumWidgets(); // Cycle through tabs
			SelectTab(NextIndex); // Select the next tab
			return FReply::Handled(); // Indicate we handled the key press
		}
	}
	// Check for Escape key to close the inventory
	else if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		RemoveFromParent(); // Close this widget
		APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
		if (PlayerController)
		{
			PlayerController->SetInputMode(FInputModeGameOnly());
			PlayerController->SetShowMouseCursor(false);
			// Optional: Set focus back to the game viewport
			// FSlateApplication::Get().SetFocusToGameViewport(); // Old method
			FSlateApplication::Get().SetUserFocusToGameViewport(PlayerController->GetLocalPlayer()->GetControllerId());
		}
		return FReply::Handled();
	}

	// If not handled, pass to base class or return Unhandled
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UInventoryWidget::SelectTab(int32 TabIndex)
{
	// Assuming Eatables Tab corresponds to Index 0
	const int32 EatablesTabIndex = 0;

	if (!TabWidgetSwitcher || !EatableTabImage /* || !OtherTabImages... */)
	{
		UE_LOG(LogTemp, Warning, TEXT("SelectTab: Required widgets are not bound."));
		return;
	}

	// Set active widget in the switcher
	TabWidgetSwitcher->SetActiveWidgetIndex(TabIndex);

	// Update EatableTabImage color
	if (TabIndex == EatablesTabIndex)
	{
		EatableTabImage->SetColorAndOpacity(ActivatedColor);
	}
	else
	{
		EatableTabImage->SetColorAndOpacity(NotActivatedColor);
	}

	// Add logic for other tab images here if they exist
}

void UInventoryWidget::OnEatableButtonClicked()
{
	SelectTab(0); // Select the Eatables tab (index 0)
}

void UInventoryWidget::OnEatableButtonHovered()
{
	if (EatableTabImage)
	{
		EatableTabImage->SetColorAndOpacity(HoveredColor);
	}
}

void UInventoryWidget::OnEatableButtonUnhovered()
{
	if (EatableTabImage && TabWidgetSwitcher)
	{
		// Set color back based on whether this tab is currently active
		const int32 CurrentActiveIndex = TabWidgetSwitcher->GetActiveWidgetIndex();
		const int32 EatablesTabIndex = 0;
		if (CurrentActiveIndex == EatablesTabIndex)
		{
			EatableTabImage->SetColorAndOpacity(ActivatedColor);
		}
		else
		{
			EatableTabImage->SetColorAndOpacity(NotActivatedColor);
		}
	}
}

void UInventoryWidget::UpdateItemsInInventoryUI(const TArray<FSlotStruct>& AllItems)
{
	// Check if WrapBox and Slot class are valid
	if (!EatablesWrapBox || !SlotWidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("UpdateItemsInInventoryUI: EatablesWrapBox or SlotWidgetClass is not set!"));
		return;
	}

	// Clear existing slots in the Eatables wrap box
	EatablesWrapBox->ClearChildren();

	UE_LOG(LogTemp, Log, TEXT("Updating Eatables UI with %d total items."), AllItems.Num());

	// Loop through all items and create slot widgets for Eatables
	for (int32 Index = 0; Index < AllItems.Num(); ++Index)
	{
		const FSlotStruct& CurrentItem = AllItems[Index];

		// Filter for Eatables (or other types if needed)
		if (CurrentItem.ItemType == EInventoryItemType::EIT_Eatables)
		{
			// Create a new slot widget
			UUserWidget* CreatedWidget = CreateWidget(this, SlotWidgetClass);
			if (CreatedWidget)
			{
				// --- Pass data to the Slot Widget --- 
				// Cast the created widget to our C++ Slot Widget class
				USlotWidget* SlotWidget = Cast<USlotWidget>(CreatedWidget);
				if (SlotWidget)
				{
					// Call the initialization function on the C++ slot widget
					// Pass OwnerCharacter and OwnerInventory (now stored as members)
					// Also pass the Item Info Widget reference
					SlotWidget->InitializeSlot(CurrentItem, Index, OwnerCharacter, OwnerInventory, WBP_ItemInfo);
					UE_LOG(LogTemp, Verbose, TEXT("Initialized Slot Widget for item %s at index %d"), *CurrentItem.ItemID.RowName.ToString(), Index);
				}
				else
				{
					// Log an error if casting failed - SlotWidgetClass might not be derived from USlotWidget
					UE_LOG(LogTemp, Error, TEXT("Failed to cast CreatedWidget to USlotWidget. Ensure SlotWidgetClass is derived from USlotWidget."));
				}
				
				// Add the created slot widget to the wrap box
				EatablesWrapBox->AddChildToWrapBox(CreatedWidget);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to create SlotWidget."));
			}
		} // End if ItemType is Eatables
	} // End for loop

	// Update the Item Info display (if bound and valid)
	if (WBP_ItemInfo)
	{
		// Typically, you would clear the Item Info or set it based on the first 
		// item, or perhaps only update it when a slot is clicked.
		// Clearing it here for now:
		WBP_ItemInfo->SetItemAndUpdate(FSlotStruct()); // Pass an empty struct to clear it
	}
} 