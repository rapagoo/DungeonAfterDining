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
#include "Inventory/InventoryComponent.h" // Include Inventory Component header
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
	// bIsFocusable = true; // Deprecated direct access
	SetIsFocusable(true); // Use the setter function instead
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
	/* // Moved to Initialize() to prevent multiple bindings
	if (EatableButton)
	{
		EatableButton->OnClicked.AddDynamic(this, &UInventoryWidget::OnEatableButtonClicked);
		EatableButton->OnHovered.AddDynamic(this, &UInventoryWidget::OnEatableButtonHovered);
		EatableButton->OnUnhovered.AddDynamic(this, &UInventoryWidget::OnEatableButtonUnhovered);
	}
	*/

	// Optional: Ensure the first tab is selected on construct if needed
	// SelectTab(2); // Or maybe SelectTab(2) if Eatables is default?
}

bool UInventoryWidget::Initialize()
{
	const bool bSuccess = Super::Initialize();
	if (!bSuccess) return false;

	// Bind button clicks and hover events here, ensuring they are bound only once
	if (EatableButton)
	{
		if (!EatableButton->OnClicked.IsBound()) // Optional check to be extra safe
		{
			EatableButton->OnClicked.AddDynamic(this, &UInventoryWidget::OnEatableButtonClicked);
		}
		if (!EatableButton->OnHovered.IsBound())
		{
			 EatableButton->OnHovered.AddDynamic(this, &UInventoryWidget::OnEatableButtonHovered);
		}
	   if (!EatableButton->OnUnhovered.IsBound())
		{
			EatableButton->OnUnhovered.AddDynamic(this, &UInventoryWidget::OnEatableButtonUnhovered);
		}
	}
	else
	{
		// Log if button not bound in Initialize, potential issue with widget setup
		UE_LOG(LogTemp, Warning, TEXT("UInventoryWidget::Initialize - EatableButton is not valid!"));
	}

	// Add bindings for other tab buttons if they exist...

	return true;
}

FReply UInventoryWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// Check for 'I' key to toggle inventory close
	if (InKeyEvent.GetKey() == EKeys::I)
	{
		UE_LOG(LogTemp, Log, TEXT("InventoryWidget: 'I' key pressed."));
		if (OwnerInventory) 
		{
			UE_LOG(LogTemp, Log, TEXT("InventoryWidget: Calling OwnerInventory->ToggleInventory()."));
			OwnerInventory->ToggleInventory();
			return FReply::Handled(); // We handled the key press
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("InventoryWidget: OwnerInventory is null, cannot toggle."));
		}
	}

	// Check for Tab key to switch tabs (Simple example: cycle forward)
	else if (InKeyEvent.GetKey() == EKeys::Tab)
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
	/* // Remove Escape key functionality for closing inventory
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
	*/

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
		// Ensure ItemInfoWidget is hidden after updating the inventory UI
		WBP_ItemInfo->SetVisibility(ESlateVisibility::Hidden);
		UE_LOG(LogTemp, Log, TEXT("InventoryWidget: Explicitly hid ItemInfoWidget after UI update."));
	}
} 