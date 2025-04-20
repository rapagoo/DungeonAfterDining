// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/ActionMenuWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Characters/WarriorHeroCharacter.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryItemActor.h" // Needed for spawning item actor
#include "Inventory/InvenItemEnum.h"     // For EInventoryItemType
#include "Kismet/GameplayStatics.h"   // For GetPlayerCharacter, GetPlayerController
#include "Inventory/SlotStruct.h"
#include "Inventory/SlotWidget.h" // Include SlotWidget header
#include "Inventory/InventoryComponent.h" // Needed to get InventoryWidgetInstance
#include "Inventory/InventoryWidget.h" // Needed for casting InventoryWidgetInstance
#include "GameFramework/PlayerController.h" // Needed for SetInputMode

void UActionMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Bind button events
	if (UseButton) UseButton->OnClicked.AddDynamic(this, &UActionMenuWidget::OnUseButtonClicked);
	if (DropButton) DropButton->OnClicked.AddDynamic(this, &UActionMenuWidget::OnDropButtonClicked);
	if (CancelButton) CancelButton->OnClicked.AddDynamic(this, &UActionMenuWidget::OnCancelButtonClicked);

	if (UseButton) UseButton->OnHovered.AddDynamic(this, &UActionMenuWidget::OnUseButtonHovered);
	if (UseButton) UseButton->OnUnhovered.AddDynamic(this, &UActionMenuWidget::OnUseButtonUnhovered);
	if (DropButton) DropButton->OnHovered.AddDynamic(this, &UActionMenuWidget::OnDropButtonHovered);
	if (DropButton) DropButton->OnUnhovered.AddDynamic(this, &UActionMenuWidget::OnDropButtonUnhovered);
	if (CancelButton) CancelButton->OnHovered.AddDynamic(this, &UActionMenuWidget::OnCancelButtonHovered);
	if (CancelButton) CancelButton->OnUnhovered.AddDynamic(this, &UActionMenuWidget::OnCancelButtonUnhovered);

	// Hide hover images initially
	if (UseButtonHoverImage) UseButtonHoverImage->SetVisibility(ESlateVisibility::Hidden);
	if (DropButtonHoverImage) DropButtonHoverImage->SetVisibility(ESlateVisibility::Hidden);
	if (CancelButtonHoverImage) CancelButtonHoverImage->SetVisibility(ESlateVisibility::Hidden);

	// Set focus to this widget to allow mouse leave event to work properly
	// TakeWidget() returns the SWidget backing this UUserWidget
	TSharedPtr<SWidget> SafeWidget = TakeWidget();
	if (SafeWidget.IsValid())
	{
		FSlateApplication::Get().SetKeyboardFocus(SafeWidget);
	}
	
	// Position setting is typically handled by the creator, but could be attempted here if needed
	// based on OwnerCharacter's player controller mouse position.
}

void UActionMenuWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	RemoveFromParent();
}

void UActionMenuWidget::InitializeMenu(const FSlotStruct& InItem, int32 InSlotIndex, AWarriorHeroCharacter* InOwnerCharacter, UInventoryComponent* InOwnerInventory, USlotWidget* InOwningSlotWidget)
{
	ItemToActOn = InItem;
	ItemSlotIndex = InSlotIndex;
	OwnerCharacter = InOwnerCharacter;
	OwnerInventory = InOwnerInventory;
	OwningSlotWidget = InOwningSlotWidget; // Store the owning slot widget

	UE_LOG(LogTemp, Log, TEXT("ActionMenu initialized for Slot %d, Item %s, OwningSlot: %s"), 
		ItemSlotIndex, 
		*ItemToActOn.ItemID.RowName.ToString(),
		OwningSlotWidget ? *OwningSlotWidget->GetName() : TEXT("None"));
	
	// Potentially update button visibility/text based on ItemToActOn if needed
}

void UActionMenuWidget::OnUseButtonClicked()
{
	UE_LOG(LogTemp, Log, TEXT("'Use' button clicked for slot %d"), ItemSlotIndex);

	// Call the UseItemInSlot function on the slot that owns this menu
	if (OwningSlotWidget)
	{
		OwningSlotWidget->UseItemInSlot();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("OnUseButtonClicked: OwningSlotWidget is null!"));
	}

	// Close the menu after action
	// Set focus back to the Inventory Widget before removing self
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	UInventoryComponent* OwnerInvComp = OwningSlotWidget ? OwningSlotWidget->GetOwnerInventory() : nullptr;
	if (PC && OwnerInvComp)
	{
		UUserWidget* InvWidgetInstance = OwnerInvComp->GetInventoryWidgetInstance();
		UInventoryWidget* InventoryWidget = Cast<UInventoryWidget>(InvWidgetInstance);
		if (InventoryWidget)
		{
			FInputModeUIOnly InputModeData;
			InputModeData.SetWidgetToFocus(InventoryWidget->TakeWidget());
			InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock); 
			PC->SetInputMode(InputModeData);
			UE_LOG(LogTemp, Log, TEXT("OnUseButtonClicked: Set focus back to InventoryWidget."));
		}
	}
	RemoveFromParent(); 
}

void UActionMenuWidget::OnDropButtonClicked()
{
	UE_LOG(LogTemp, Log, TEXT("Drop Button Clicked for Item: %s at Index: %d"), *ItemToActOn.ItemID.RowName.ToString(), ItemSlotIndex);

	if (!OwnerCharacter || !OwnerInventory || ItemSlotIndex < 0)
	{
		UE_LOG(LogTemp, Error, TEXT("OnDropButtonClicked: OwnerCharacter, OwnerInventory, or ItemSlotIndex is invalid."));
		RemoveFromParent();
		return;
	}

	// 1. Spawn the item actor in the world
	FVector SpawnLocation = OwnerCharacter->GetActorLocation() + OwnerCharacter->GetActorForwardVector() * 100.0f; // Drop slightly in front
	FRotator SpawnRotation = OwnerCharacter->GetActorRotation();
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerCharacter;
	SpawnParams.Instigator = OwnerCharacter; // Instigator might be controller
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// Ensure the ItemActorClass is set in the Blueprint defaults
	if (!ItemActorClass)
	{
		UE_LOG(LogTemp, Error, TEXT("OnDropButtonClicked: ItemActorClass is not set in ActionMenuWidget defaults!"));
		RemoveFromParent();
		return;
	}

	// Spawn the specific Blueprint class, not the base C++ class
	AInventoryItemActor* DroppedActor = GetWorld()->SpawnActor<AInventoryItemActor>(ItemActorClass, SpawnLocation, SpawnRotation, SpawnParams);
	
	if (DroppedActor)
	{
		// IMPORTANT: Set the item data on the spawned actor!
		// DroppedActor->Item = ItemToActOn; // Direct access not allowed
		DroppedActor->SetItemData(ItemToActOn); // Use the public setter function, and use ItemToActOn

		// We also need to trigger its OnConstruction manually if the item property was ExposeOnSpawn=false
		// and relied on construction script logic that might not run automatically here.
		// Consider if the mesh needs updating after setting data, potentially via RerunConstructionScripts or a specific update function.
		// DroppedActor->RerunConstructionScripts(); 

		UE_LOG(LogTemp, Log, TEXT("Spawned Item Actor: %s"), *DroppedActor->GetName());

		// 2. Remove the item from the inventory array
		if (OwnerInventory->InventorySlots.IsValidIndex(ItemSlotIndex))
		{
			OwnerInventory->InventorySlots[ItemSlotIndex] = FSlotStruct(); // Replace with empty slot
			UE_LOG(LogTemp, Log, TEXT("Removed item from inventory slot %d."), ItemSlotIndex);
			OwnerInventory->UpdateInventoryUI(); // Request UI update
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("OnDropButtonClicked: ItemSlotIndex %d is invalid for InventorySlots array."), ItemSlotIndex);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to spawn Item Actor."));
	}

	// 3. Close the menu
	// Set focus back to the Inventory Widget before removing self
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	UInventoryComponent* OwnerInvComp = OwningSlotWidget ? OwningSlotWidget->GetOwnerInventory() : nullptr;
	if (PC && OwnerInvComp)
	{
		UUserWidget* InvWidgetInstance = OwnerInvComp->GetInventoryWidgetInstance();
		UInventoryWidget* InventoryWidget = Cast<UInventoryWidget>(InvWidgetInstance);
		if (InventoryWidget)
		{
			FInputModeUIOnly InputModeData;
			InputModeData.SetWidgetToFocus(InventoryWidget->TakeWidget());
			InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock); 
			PC->SetInputMode(InputModeData);
			UE_LOG(LogTemp, Log, TEXT("OnDropButtonClicked: Set focus back to InventoryWidget."));
		}
	}
	RemoveFromParent();
	
	// Focusing the main inventory might be better handled by the inventory component
	// after the menu closes, rather than here.
}

void UActionMenuWidget::OnCancelButtonClicked()
{
	UE_LOG(LogTemp, Log, TEXT("Cancel Button Clicked"));
	// Set focus back to the Inventory Widget before removing self
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	UInventoryComponent* OwnerInvComp = OwningSlotWidget ? OwningSlotWidget->GetOwnerInventory() : nullptr;
	if (PC && OwnerInvComp)
	{
		UUserWidget* InvWidgetInstance = OwnerInvComp->GetInventoryWidgetInstance();
		UInventoryWidget* InventoryWidget = Cast<UInventoryWidget>(InvWidgetInstance);
		if (InventoryWidget)
		{
			FInputModeUIOnly InputModeData;
			InputModeData.SetWidgetToFocus(InventoryWidget->TakeWidget());
			InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock); 
			PC->SetInputMode(InputModeData);
			UE_LOG(LogTemp, Log, TEXT("OnCancelButtonClicked: Set focus back to InventoryWidget."));
		}
	}
	RemoveFromParent();
}

// --- Hover Implementations ---

void UActionMenuWidget::OnUseButtonHovered() { if(UseButtonHoverImage) UseButtonHoverImage->SetVisibility(ESlateVisibility::Visible); }
void UActionMenuWidget::OnUseButtonUnhovered() { if(UseButtonHoverImage) UseButtonHoverImage->SetVisibility(ESlateVisibility::Hidden); }

void UActionMenuWidget::OnDropButtonHovered() { if(DropButtonHoverImage) DropButtonHoverImage->SetVisibility(ESlateVisibility::Visible); }
void UActionMenuWidget::OnDropButtonUnhovered() { if(DropButtonHoverImage) DropButtonHoverImage->SetVisibility(ESlateVisibility::Hidden); }

void UActionMenuWidget::OnCancelButtonHovered() { if(CancelButtonHoverImage) CancelButtonHoverImage->SetVisibility(ESlateVisibility::Visible); }
void UActionMenuWidget::OnCancelButtonUnhovered() { if(CancelButtonHoverImage) CancelButtonHoverImage->SetVisibility(ESlateVisibility::Hidden); } 