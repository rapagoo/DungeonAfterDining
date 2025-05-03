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

	// Bind tab button click events
	if (EatableButton)
	{
		EatableButton->OnClicked.AddDynamic(this, &UInventoryWidget::OnEatableButtonClicked);
	}
	if (FoodButton)
	{
		FoodButton->OnClicked.AddDynamic(this, &UInventoryWidget::OnFoodButtonClicked);
	}
	if (RecipesButton)
	{
		RecipesButton->OnClicked.AddDynamic(this, &UInventoryWidget::OnRecipesButtonClicked);
	}
	
	// Set initial tab (e.g., Eatables)
	if (TabWidgetSwitcher && EatablesWrapBox)
	{
		TabWidgetSwitcher->SetActiveWidget(EatablesWrapBox);
	}
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

	// Add bindings for Food button
	if (FoodButton)
	{
		 if (!FoodButton->OnClicked.IsBound())
		{
			FoodButton->OnClicked.AddDynamic(this, &UInventoryWidget::OnFoodButtonClicked);
		}
		if (!FoodButton->OnHovered.IsBound())
		{
			 FoodButton->OnHovered.AddDynamic(this, &UInventoryWidget::OnFoodButtonHovered);
		}
	   if (!FoodButton->OnUnhovered.IsBound())
		{
			FoodButton->OnUnhovered.AddDynamic(this, &UInventoryWidget::OnFoodButtonUnhovered);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UInventoryWidget::Initialize - FoodButton is not valid!"));
	}

	// Add bindings for Recipes button
	 if (RecipesButton)
	{
		 if (!RecipesButton->OnClicked.IsBound())
		{
			RecipesButton->OnClicked.AddDynamic(this, &UInventoryWidget::OnRecipesButtonClicked);
		}
		if (!RecipesButton->OnHovered.IsBound())
		{
			 RecipesButton->OnHovered.AddDynamic(this, &UInventoryWidget::OnRecipesButtonHovered);
		}
	   if (!RecipesButton->OnUnhovered.IsBound())
		{
			RecipesButton->OnUnhovered.AddDynamic(this, &UInventoryWidget::OnRecipesButtonUnhovered);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UInventoryWidget::Initialize - RecipesButton is not valid!"));
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
	if (TabWidgetSwitcher && EatablesWrapBox && EatableTabImage && FoodTabImage && RecipesTabImage)
	{
		TabWidgetSwitcher->SetActiveWidget(EatablesWrapBox);
		UE_LOG(LogTemp, Log, TEXT("Switched to Eatables Tab"));

		// Update all tab colors immediately
		EatableTabImage->SetColorAndOpacity(ActivatedColor);
		FoodTabImage->SetColorAndOpacity(NotActivatedColor);
		RecipesTabImage->SetColorAndOpacity(NotActivatedColor);
	}
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

void UInventoryWidget::OnFoodButtonClicked()
{
	if (TabWidgetSwitcher && FoodWrapBox && EatableTabImage && FoodTabImage && RecipesTabImage)
	{
		TabWidgetSwitcher->SetActiveWidget(FoodWrapBox);
		UE_LOG(LogTemp, Log, TEXT("Switched to Food Tab"));

		// Update all tab colors immediately
		EatableTabImage->SetColorAndOpacity(NotActivatedColor);
		FoodTabImage->SetColorAndOpacity(ActivatedColor);
		RecipesTabImage->SetColorAndOpacity(NotActivatedColor);
	}
}

void UInventoryWidget::OnFoodButtonHovered()
{
	if (FoodTabImage)
	{
		FoodTabImage->SetColorAndOpacity(HoveredColor);
	}
}

void UInventoryWidget::OnFoodButtonUnhovered()
{
	if (FoodTabImage && TabWidgetSwitcher && FoodWrapBox)
	{
		// Set color back based on whether this tab is currently active
		if (TabWidgetSwitcher->GetActiveWidget() == FoodWrapBox)
		{
			FoodTabImage->SetColorAndOpacity(ActivatedColor);
		}
		else
		{
			FoodTabImage->SetColorAndOpacity(NotActivatedColor);
		}
	}
}

void UInventoryWidget::OnRecipesButtonClicked()
{
	if (TabWidgetSwitcher && RecipesWrapBox && EatableTabImage && FoodTabImage && RecipesTabImage)
	{
		TabWidgetSwitcher->SetActiveWidget(RecipesWrapBox);
		UE_LOG(LogTemp, Log, TEXT("Switched to Recipes Tab"));

		// Update all tab colors immediately
		EatableTabImage->SetColorAndOpacity(NotActivatedColor);
		FoodTabImage->SetColorAndOpacity(NotActivatedColor);
		RecipesTabImage->SetColorAndOpacity(ActivatedColor);
	}
}

void UInventoryWidget::OnRecipesButtonHovered()
{
	if (RecipesTabImage)
	{
		RecipesTabImage->SetColorAndOpacity(HoveredColor);
	}
}

void UInventoryWidget::OnRecipesButtonUnhovered()
{
	if (RecipesTabImage && TabWidgetSwitcher && RecipesWrapBox)
	{
		// Set color back based on whether this tab is currently active
		 if (TabWidgetSwitcher->GetActiveWidget() == RecipesWrapBox)
		{
			RecipesTabImage->SetColorAndOpacity(ActivatedColor);
		}
		else
		{
			RecipesTabImage->SetColorAndOpacity(NotActivatedColor);
		}
	}
}

void UInventoryWidget::UpdateItemsInInventoryUI(const TArray<FSlotStruct>& AllItems)
{
	// Check if WrapBoxes and Slot class are valid
	if (!EatablesWrapBox || !FoodWrapBox || !RecipesWrapBox || !SlotWidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("UpdateItemsInInventoryUI: One or more WrapBoxes or SlotWidgetClass is not set!"));
		return;
	}

	// Clear existing slots in ALL wrap boxes
	EatablesWrapBox->ClearChildren();
	FoodWrapBox->ClearChildren();
	RecipesWrapBox->ClearChildren();

	UE_LOG(LogTemp, Log, TEXT("Updating Inventory UI with %d total items."), AllItems.Num());

	// Loop through all items and create slot widgets in the correct tab
	for (int32 Index = 0; Index < AllItems.Num(); ++Index)
	{
		const FSlotStruct& CurrentItem = AllItems[Index];
		UWrapBox* TargetWrapBox = nullptr;

		// Log the item type being processed in the UI update
		UE_LOG(LogTemp, Log, TEXT("[UpdateItemsInInventoryUI] Processing Item Index: %d, ID: %s, Type: %s, Qty: %d"), 
			   Index, 
			   *CurrentItem.ItemID.RowName.ToString(), 
			   *UEnum::GetValueAsString(CurrentItem.ItemType),
			   CurrentItem.Quantity);

		// Determine the target wrap box based on item type
		switch (CurrentItem.ItemType)
		{
			case EInventoryItemType::EIT_Eatables:
				TargetWrapBox = EatablesWrapBox;
				UE_LOG(LogTemp, Log, TEXT("[UpdateItemsInInventoryUI] Assigning to EatablesWrapBox"));
				break;
			case EInventoryItemType::EIT_Food:
				TargetWrapBox = FoodWrapBox;
				 UE_LOG(LogTemp, Log, TEXT("[UpdateItemsInInventoryUI] Assigning to FoodWrapBox"));
				break;
			case EInventoryItemType::EIT_Recipe:
				TargetWrapBox = RecipesWrapBox;
				 UE_LOG(LogTemp, Log, TEXT("[UpdateItemsInInventoryUI] Assigning to RecipesWrapBox"));
				break;
			// Add cases for other types like Sword, Shield if needed
			default:
				 UE_LOG(LogTemp, Warning, TEXT("[UpdateItemsInInventoryUI] Unhandled item type %s for item ID %s at index %d. Assigning to default (Eatables)."), 
					   *UEnum::GetValueAsString(CurrentItem.ItemType), 
					   *CurrentItem.ItemID.RowName.ToString(), 
					   Index);
				TargetWrapBox = EatablesWrapBox; // Default to Eatables for now
				break;
		}

		// If a target box was determined and the item is valid (e.g., has a valid ID)
		if (TargetWrapBox && !CurrentItem.ItemID.RowName.IsNone() && CurrentItem.Quantity > 0)
		{
			// Create a new slot widget
			UUserWidget* CreatedWidget = CreateWidget(this, SlotWidgetClass);
			USlotWidget* CreatedSlotWidget = Cast<USlotWidget>(CreatedWidget);

			if (CreatedSlotWidget)
			{
				// Initialize the slot widget with item data and index using the correct function
				// Also pass the Item Info Widget reference (WBP_ItemInfo)
				CreatedSlotWidget->InitializeSlot(CurrentItem, Index, OwnerCharacter, OwnerInventory, WBP_ItemInfo);
				
				// Add the created slot widget to the determined wrap box
				TargetWrapBox->AddChildToWrapBox(CreatedSlotWidget);
			}
		}
	}
} 