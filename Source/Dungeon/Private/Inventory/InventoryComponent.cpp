// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/InventoryComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/ActorComponent.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/InventoryItemActor.h"
#include "Inventory/SlotStruct.h"
#include "Inventory/InvenItemStruct.h" // Needed for item data lookup
#include "Kismet/GameplayStatics.h" // For GetPlayerController
#include "Kismet/KismetSystemLibrary.h" // For SphereTraceSingle
#include "DrawDebugHelpers.h" // For DrawDebugSphere (optional)
#include "Camera/CameraComponent.h" // Needed for camera-based trace
#include "Characters/WarriorHeroCharacter.h" // Needed to get camera component or owner cast
#include "Inventory/InventoryWidget.h" // Needed for casting InventoryWidgetInstance
#include "Inventory/ItemInfoWidget.h" // Needed for getting/using ItemInfoWidget
// #include "Camera/CameraComponent.h" // No longer needed

// Include necessary headers for casting
#include "Characters/WarriorHeroCharacter.h" 
#include "Inventory/InventoryWidget.h"

// Sets default values for this component's properties
UInventoryComponent::UInventoryComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame. 
	PrimaryComponentTick.bCanEverTick = true; // ENABLE TICK

	InventoryWidgetInstance = nullptr; // Initialize pointer
	InteractWidgetInstance = nullptr;  // Initialize pointer

	// Initialize inventory slots (e.g., with a fixed size)
	// InventorySlots.SetNum(20); // Example: Initialize with 20 empty slots
	InventorySlots.SetNum(20); // Initialize with 20 empty slots by default
}


// Called when the game starts
void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PlayerController = Cast<APlayerController>(GetOwner()->GetInstigatorController());
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Error, TEXT("InventoryComponent: Could not get Player Controller in BeginPlay."));
		return;
	}

	if (InventoryWidgetClass)
	{
		InventoryWidgetInstance = CreateWidget<UUserWidget>(PlayerController, InventoryWidgetClass);
		if (InventoryWidgetInstance)
		{
			UE_LOG(LogTemp, Log, TEXT("Inventory Widget created."));

			// Try to set owner references using ACharacter for more generality
			ACharacter* OwningActorAsCharacter = Cast<ACharacter>(GetOwner()); // Cast to ACharacter
			UInventoryWidget* TypedWidget = Cast<UInventoryWidget>(InventoryWidgetInstance);
			if(TypedWidget && OwningActorAsCharacter) // Check if both are valid
			{
				// Assuming UInventoryWidget::SetOwnerReferences can take ACharacter*
				// If it specifically needs AWarriorHeroCharacter*, this part will need adjustment
				// or the SetOwnerReferences signature in UInventoryWidget needs to be changed.
				TypedWidget->SetOwnerReferences(OwningActorAsCharacter, this);
			}
			else if (TypedWidget && !OwningActorAsCharacter)
			{
			    UE_LOG(LogTemp, Warning, TEXT("InventoryComponent::BeginPlay: Owner is not an ACharacter. Cannot set owner references in InventoryWidget."));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create Inventory Widget."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("InventoryWidgetClass not set in InventoryComponent."));
	}

	// Create Interact Widget if class is set
	if (InteractWidgetClass)
	{
		InteractWidgetInstance = CreateWidget<UUserWidget>(PlayerController, InteractWidgetClass);
		if (InteractWidgetInstance)
		{
			UE_LOG(LogTemp, Log, TEXT("Interact Widget created."));
			// Assuming Interact widget should be added to viewport immediately
			InteractWidgetInstance->AddToViewport(); 
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create Interact Widget."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("InteractWidgetClass not set in InventoryComponent."));
	}
	
}

void UInventoryComponent::SetupInputBinding(UEnhancedInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent); // Ensure input component is valid

	if (InventoryToggleAction)
	{
		PlayerInputComponent->BindAction(InventoryToggleAction, ETriggerEvent::Triggered, this, &UInventoryComponent::ToggleInventory);
	}

	if (PickupAction)
	{
		PlayerInputComponent->BindAction(PickupAction, ETriggerEvent::Triggered, this, &UInventoryComponent::HandlePickup);
	}
}

void UInventoryComponent::ToggleInventory()
{
	APlayerController* PlayerController = Cast<APlayerController>(GetOwner()->GetInstigatorController());
	if (!PlayerController) return;

	if (!InventoryWidgetInstance && InventoryWidgetClass)
	{
		InventoryWidgetInstance = CreateWidget<UUserWidget>(PlayerController, InventoryWidgetClass);
		if (!InventoryWidgetInstance) return; 

		// Set owner references - similar to BeginPlay, attempt with ACharacter
		ACharacter* OwningActorAsCharacter = Cast<ACharacter>(GetOwner());
		UInventoryWidget* TypedWidget = Cast<UInventoryWidget>(InventoryWidgetInstance);
		if(TypedWidget && OwningActorAsCharacter)
		{
			TypedWidget->SetOwnerReferences(OwningActorAsCharacter, this);
		}
        else if (TypedWidget && !OwningActorAsCharacter)
        {
            UE_LOG(LogTemp, Warning, TEXT("InventoryComponent::ToggleInventory: Owner is not an ACharacter. Cannot set owner references in InventoryWidget."));
        }
	}

	if (!InventoryWidgetInstance) return; 

	if (InventoryWidgetInstance->IsInViewport())
	{
		InventoryWidgetInstance->RemoveFromParent();

		bool bRestoreGameOnlyInput = true;
		if (AWarriorHeroCharacter* WarriorChar = Cast<AWarriorHeroCharacter>(GetOwner()))
		{
			if (WarriorChar->IsInCookingMode())
			{
				UE_LOG(LogTemp, Log, TEXT("Inventory Closed, but WarriorHeroCharacter is in Cooking Mode. Setting GameAndUI input."));
				FInputModeGameAndUI InputModeData;
				InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
				InputModeData.SetHideCursorDuringCapture(false);
				PlayerController->SetInputMode(InputModeData);
				PlayerController->SetShowMouseCursor(true);
				bRestoreGameOnlyInput = false; // GameOnly input mode should not be set
			}
		}

		if (bRestoreGameOnlyInput)
		{
			UE_LOG(LogTemp, Log, TEXT("Inventory Closed, returning to GameOnly input for %s."), *GetOwner()->GetName());
			PlayerController->SetInputMode(FInputModeGameOnly());
			PlayerController->SetShowMouseCursor(false);
		}
	}
	else
	{
		// ... (인벤토리 열기 로직은 거의 동일하게 유지) ...
		UE_LOG(LogTemp, Log, TEXT("Attempting to open inventory for %s..."), *GetOwner()->GetName());

		if (!InventoryWidgetInstance)
		{
			UE_LOG(LogTemp, Error, TEXT("InventoryWidgetInstance is NULL before AddToViewport for %s!"), *GetOwner()->GetName());
			return; 
		}

		InventoryWidgetInstance->AddToViewport();
		UE_LOG(LogTemp, Log, TEXT("Called AddToViewport for %s. IsInViewport: %s"), *GetOwner()->GetName(), InventoryWidgetInstance->IsInViewport() ? TEXT("True") : TEXT("False"));

		UInventoryWidget* InventoryWidget = Cast<UInventoryWidget>(InventoryWidgetInstance);
		if (InventoryWidget)
		{
			UItemInfoWidget* ItemInfo = InventoryWidget->GetItemInfoWidget(); 
			if (ItemInfo)
			{
				ItemInfo->SetVisibility(ESlateVisibility::Hidden);
				UE_LOG(LogTemp, Log, TEXT("InventoryComponent: Explicitly hid ItemInfoWidget on inventory open for %s."), *GetOwner()->GetName());
			}
		}

		FInputModeUIOnly InputModeData;
		TSharedPtr<SWidget> WidgetToFocus = InventoryWidgetInstance->TakeWidget();
		if (WidgetToFocus.IsValid())
		{
			InputModeData.SetWidgetToFocus(WidgetToFocus);
			UE_LOG(LogTemp, Log, TEXT("SetWidgetToFocus successful for %s."), *GetOwner()->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("InventoryWidgetInstance->TakeWidget() returned invalid widget for %s!"), *GetOwner()->GetName());
		}
		InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock); 

		PlayerController->SetInputMode(InputModeData);
		UE_LOG(LogTemp, Log, TEXT("SetInputMode(UIOnly) called for %s."), *GetOwner()->GetName());

		PlayerController->SetShowMouseCursor(true); 
		UE_LOG(LogTemp, Log, TEXT("Inventory Opened successfully for %s."), *GetOwner()->GetName());
		
		UpdateInventoryUI(); 
	}
}

void UInventoryComponent::HandlePickup()
{
	FSlotStruct FoundItemData;
	AInventoryItemActor* FoundItemActor = nullptr;

	UE_LOG(LogTemp, Log, TEXT("[HandlePickup] Function Called.")); // Log: HandlePickup start

	if (TraceForItem(FoundItemData, FoundItemActor))
	{
		// UE_LOG(LogTemp, Log, TEXT("[HandlePickup] Trace found item: %s (Actor: %s)"), 
		// 	*FoundItemData.ItemID.RowName.ToString(), 
		// 	FoundItemActor ? *FoundItemActor->GetName() : TEXT("None"));
		
		bool bAdded = AddItem(FoundItemData);
		UE_LOG(LogTemp, Log, TEXT("[HandlePickup] AddItem returned: %s"), bAdded ? TEXT("True") : TEXT("False"));

		if (bAdded)
		{
			UE_LOG(LogTemp, Log, TEXT("[HandlePickup] Item added successfully."));
			if (FoundItemActor)
			{
				UE_LOG(LogTemp, Log, TEXT("[HandlePickup] Destroying item actor: %s"), *FoundItemActor->GetName());
				FoundItemActor->Destroy();
			}
			UpdateInventoryUI();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[HandlePickup] Failed to add item to inventory."));
		}
	}
	else
	{
		// UE_LOG(LogTemp, Log, TEXT("[HandlePickup] Trace did not find any suitable item."));
	}
}

bool UInventoryComponent::AddItem(const FSlotStruct& ItemToAdd)
{
	// Log the item type being added
	UE_LOG(LogTemp, Log, TEXT("[AddItem] Attempting to add Item: %s, Type: %s, Quantity: %d"), 
		   *ItemToAdd.ItemID.RowName.ToString(), 
		   *UEnum::GetValueAsString(ItemToAdd.ItemType), 
		   ItemToAdd.Quantity);

	if (!ItemDataTable.IsValid() || ItemToAdd.ItemID.RowName.IsNone() || ItemToAdd.Quantity <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AddItem] Invalid ItemDataTable or ItemToAdd data."));
		return false;
	}

	UDataTable* LoadedItemTable = ItemDataTable.LoadSynchronous();
	if (!LoadedItemTable)
	{
		UE_LOG(LogTemp, Error, TEXT("[AddItem] Failed to load ItemDataTable: %s"), *ItemDataTable.ToString());
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("[AddItem] Checking for existing stacks..."));
	// 1. Try to stack with existing items
	for (FSlotStruct& ExistingSlot : InventorySlots)
	{
		// Check if slot has the same item ID (and is not empty)
		if (ExistingSlot.ItemID.RowName == ItemToAdd.ItemID.RowName && !ExistingSlot.ItemID.RowName.IsNone()) 
		{
			UE_LOG(LogTemp, Log, TEXT("[AddItem] Found potential stack slot with %s"), *ExistingSlot.ItemID.RowName.ToString());
			// Get Item Definition from Data Table to find StackSize
			const FString ContextString = TEXT("AddItem Stack Check");
			FInventoryItemStruct* ItemDefinition = LoadedItemTable->FindRow<FInventoryItemStruct>(ItemToAdd.ItemID.RowName, ContextString);

			if (ItemDefinition)
			{
				const int32 MaxStackSize = ItemDefinition->StackSize > 0 ? ItemDefinition->StackSize : 1; // Use StackSize from DT, ensure at least 1
				UE_LOG(LogTemp, Log, TEXT("[AddItem] Max stack size: %d, Current quantity: %d"), MaxStackSize, ExistingSlot.Quantity);

				// Check if the existing slot can stack more (Quantity < MaxStackSize)
				if (ExistingSlot.Quantity < MaxStackSize)
				{
					// Calculate how much can be added
					int32 QuantityToAdd = FMath::Min(ItemToAdd.Quantity, MaxStackSize - ExistingSlot.Quantity);
					UE_LOG(LogTemp, Log, TEXT("[AddItem] Can add %d quantity to stack."), QuantityToAdd);

					if (QuantityToAdd > 0)
					{
						ExistingSlot.Quantity += QuantityToAdd;
						UE_LOG(LogTemp, Log, TEXT("[AddItem] Stacked %d of %s. New quantity: %d. Returning true."), QuantityToAdd, *ItemToAdd.ItemID.RowName.ToString(), ExistingSlot.Quantity);
						return true; 
					}
				}
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[AddItem] No suitable stack found. Checking for empty slots..."));
	// 2. If not stacked, find an empty slot
	for (int32 Index = 0; Index < InventorySlots.Num(); ++Index) // Use index for logging
	{
		 FSlotStruct& ExistingSlot = InventorySlots[Index];
		if (ExistingSlot.ItemID.RowName.IsNone()) // Check if slot is empty 
		{
			ExistingSlot = ItemToAdd; // Add item to the empty slot
			UE_LOG(LogTemp, Log, TEXT("[AddItem] Added item %s to empty slot at index %d. Returning true."), *ItemToAdd.ItemID.RowName.ToString(), Index);
			return true; // Successfully added to empty slot
		}
	}

	// 3. If no stacking possible and no empty slots
	UE_LOG(LogTemp, Warning, TEXT("[AddItem] Inventory full or could not add item %s. Returning false."), *ItemToAdd.ItemID.RowName.ToString());
	return false; 
}

void UInventoryComponent::UpdateInventoryUI()
{
	// Check if the widget instance is valid
	if (InventoryWidgetInstance)
	{
		UE_LOG(LogTemp, Log, TEXT("[UpdateInventoryUI] Function called. Attempting to update widget."));

		// Cast InventoryWidgetInstance to our specific C++ widget class
		UInventoryWidget* InventoryWidget = Cast<UInventoryWidget>(InventoryWidgetInstance);
		if (InventoryWidget)
		{
			// Call the function on the C++ widget to update its slots
			InventoryWidget->UpdateItemsInInventoryUI(InventorySlots); // Pass the TArray
			UE_LOG(LogTemp, Log, TEXT("[UpdateInventoryUI] Called UpdateItemsInInventoryUI on C++ InventoryWidget."));
		}
		else
		{
			// Log an error if casting failed. InventoryWidgetClass might be wrong or not derived from UInventoryWidget.
			UE_LOG(LogTemp, Error, TEXT("[UpdateInventoryUI] Failed to cast InventoryWidgetInstance to UInventoryWidget. Ensure InventoryWidgetClass in BP defaults derives from UInventoryWidget."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[UpdateInventoryUI] Function called, but InventoryWidgetInstance is null."));
	}
}

bool UInventoryComponent::TraceForItem(FSlotStruct& OutItem, AInventoryItemActor*& OutItemActor)
{
	// UE_LOG(LogTemp, VeryVerbose, TEXT("[TraceForItem] Function Called."));
	OutItemActor = nullptr;

	// Get the owner actor (Player Character)
	AActor* Owner = GetOwner();
	if (!Owner) 
	{
		UE_LOG(LogTemp, Error, TEXT("[TraceForItem] Owner is null."));
		return false;
	}

	// Calculate Trace Start and End points based on the original Blueprint logic
	const FVector StartLocation = Owner->GetActorLocation() + FVector(0.0f, 0.0f, -65.0f); // Subtract Z value
	const FVector EndLocation = StartLocation + Owner->GetActorForwardVector() * 100.0f;
	const float SphereRadius = 30.0f;

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(Owner); // Ignore self

	FHitResult HitResult;
	const ETraceTypeQuery TraceChannel = UEngineTypes::ConvertToTraceType(ECC_Visibility);

	bool bHit = UKismetSystemLibrary::SphereTraceSingle(
		GetWorld(),
		StartLocation,
		EndLocation,
		SphereRadius,
		TraceChannel, 
		true, // bTraceComplex
		ActorsToIgnore,
		EDrawDebugTrace::None, // Disable debug drawing
		HitResult,
		true  // bIgnoreSelf
	);

	// --- Enable Debug Drawing --- 
/* // Debug Drawing Disabled
#if ENABLE_DRAW_DEBUG
	DrawDebugSphere(GetWorld(), StartLocation, SphereRadius, 12, FColor::Yellow, false, 2.0f);
	DrawDebugSphere(GetWorld(), EndLocation, SphereRadius, 12, FColor::Orange, false, 2.0f);
	DrawDebugLine(GetWorld(), StartLocation, EndLocation, FColor::Red, false, 2.0f);
	if(bHit)
	{
		DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, SphereRadius, 12, FColor::Green, false, 2.0f);
	}
#endif
*/
	// --- End Debug Drawing --- 

	if (bHit && HitResult.GetActor())
	{
		//UE_LOG(LogTemp, Log, TEXT("[TraceForItem] SphereTrace hit actor: %s (Component: %s)"), 
			//*HitResult.GetActor()->GetName(), 
			//HitResult.GetComponent() ? *HitResult.GetComponent()->GetName() : TEXT("None"));
		
		AInventoryItemActor* HitItemActor = Cast<AInventoryItemActor>(HitResult.GetActor());
		if (HitItemActor)
		{
			//UE_LOG(LogTemp, Log, TEXT("[TraceForItem] Successfully cast hit actor to AInventoryItemActor."));
			OutItemActor = HitItemActor;
			OutItem = HitItemActor->GetItemData(); // Use the getter
			return true;
		}
		else
		{
			//UE_LOG(LogTemp, Log, TEXT("[TraceForItem] Failed to cast hit actor (%s) to AInventoryItemActor."), *HitResult.GetActor()->GetName());
		}
	}
	// else { UE_LOG(LogTemp, VeryVerbose, TEXT("[TraceForItem] SphereTrace did not hit anything.")); } // Reduce log spam

	return false;
}


// Called every frame
void UInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                        FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	FSlotStruct FoundItemData; // Still needed for the Trace function signature
	AInventoryItemActor* FoundItemActor = nullptr; // Still needed
	bool bFound = TraceForItem(FoundItemData, FoundItemActor);

	// Manage Interact Widget visibility based on trace result
	if (InteractWidgetInstance)
	{
		if (bFound)
		{
			// If item found, ensure the widget is visible
			if (!InteractWidgetInstance->IsInViewport())
			{
				InteractWidgetInstance->AddToViewport();
				// UE_LOG(LogTemp, Verbose, TEXT("Added Interact Widget to viewport."));
			}
		}
		else
		{
			// If no item found, ensure the widget is hidden
			if (InteractWidgetInstance->IsInViewport())
			{
				InteractWidgetInstance->RemoveFromParent();
				// UE_LOG(LogTemp, Verbose, TEXT("Removed Interact Widget from viewport."));
			}
		}
	}
	// else { UE_LOG(LogTemp, Warning, TEXT("TickComponent: InteractWidgetInstance is null.")); }
}

// Implementation for removing items from a specific slot
bool UInventoryComponent::RemoveItemFromSlot(int32 SlotIndex, int32 QuantityToRemove /*= 1*/)
{
	UE_LOG(LogTemp, Log, TEXT("[RemoveItemFromSlot] Attempting to remove %d item(s) from slot %d."), QuantityToRemove, SlotIndex);

	// Validate input
	if (QuantityToRemove <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RemoveItemFromSlot] QuantityToRemove must be positive."));
		return false;
	}

	// Validate Slot Index
	if (!InventorySlots.IsValidIndex(SlotIndex))
	{
		UE_LOG(LogTemp, Error, TEXT("[RemoveItemFromSlot] Invalid SlotIndex: %d (Array Size: %d)"), SlotIndex, InventorySlots.Num());
		return false;
	}

	// Get the slot reference
	FSlotStruct& Slot = InventorySlots[SlotIndex];

	// Check if the slot is actually occupied and has enough quantity
	if (Slot.ItemID.RowName.IsNone() || Slot.Quantity < QuantityToRemove)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RemoveItemFromSlot] Slot %d is empty or has insufficient quantity (Has: %d, Needs: %d). ItemID: %s"), 
			SlotIndex, Slot.Quantity, QuantityToRemove, *Slot.ItemID.RowName.ToString());
		return false;
	}

	// Decrease quantity
	Slot.Quantity -= QuantityToRemove;
	UE_LOG(LogTemp, Log, TEXT("[RemoveItemFromSlot] Removed %d from slot %d. New quantity: %d."), QuantityToRemove, SlotIndex, Slot.Quantity);

	// If quantity is zero or less, clear the slot entirely
	if (Slot.Quantity <= 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[RemoveItemFromSlot] Slot %d is now empty. Clearing slot data."), SlotIndex);
		Slot = FSlotStruct(); // Reset to default empty slot
	}

	// Update the UI to reflect changes
	UpdateInventoryUI();

	return true; // Removal successful
}


