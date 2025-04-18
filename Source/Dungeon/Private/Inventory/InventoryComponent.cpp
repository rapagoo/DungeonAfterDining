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
// #include "Inventory/AllItemStruct.h" // No longer needed
// Include the header for your specific inventory widget class if needed for casting
// #include "UI/WBP_Inventory.h" 


// Sets default values for this component's properties
UInventoryComponent::UInventoryComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame. 
	PrimaryComponentTick.bCanEverTick = true; // ENABLE TICK

	InventoryWidgetInstance = nullptr; // Initialize pointer
	InteractWidgetInstance = nullptr;  // Initialize pointer

	// Initialize inventory slots (e.g., with a fixed size)
	// InventorySlots.SetNum(20); // Example: Initialize with 20 empty slots
}


// Called when the game starts
void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	// Get the owning player controller
	APlayerController* PlayerController = Cast<APlayerController>(GetOwner()->GetInstigatorController());
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Error, TEXT("InventoryComponent: Could not get Player Controller in BeginPlay."));
		return;
	}

	// Create Inventory Widget if class is set
	if (InventoryWidgetClass)
	{
		InventoryWidgetInstance = CreateWidget<UUserWidget>(PlayerController, InventoryWidgetClass);
		if (InventoryWidgetInstance)
		{
			UE_LOG(LogTemp, Log, TEXT("Inventory Widget created."));
			// Don't add to viewport here, ToggleInventory will handle it
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

	// Create widget if it doesn't exist
	if (!InventoryWidgetInstance && InventoryWidgetClass)
	{
		InventoryWidgetInstance = CreateWidget<UUserWidget>(PlayerController, InventoryWidgetClass);
		if (!InventoryWidgetInstance) return; // Failed to create widget
	}

	if (!InventoryWidgetInstance) return; // No widget instance available

	// Toggle visibility and input mode
	if (InventoryWidgetInstance->IsInViewport())
	{
		InventoryWidgetInstance->RemoveFromParent();
		PlayerController->SetInputMode(FInputModeGameOnly());
		PlayerController->SetShowMouseCursor(false);
		UE_LOG(LogTemp, Log, TEXT("Inventory Closed"));
	}
	else
	{
		InventoryWidgetInstance->AddToViewport();

		FInputModeUIOnly InputModeData;
		InputModeData.SetWidgetToFocus(InventoryWidgetInstance->TakeWidget()); // Set focus to the widget
		InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock); // Match BP

		PlayerController->SetInputMode(InputModeData);
		PlayerController->SetShowMouseCursor(true); // Match BP
		UE_LOG(LogTemp, Log, TEXT("Inventory Opened"));
		
		// Potentially update UI when opened
		UpdateInventoryUI(); 
	}
}

void UInventoryComponent::HandlePickup()
{
	FSlotStruct FoundItemData;
	AInventoryItemActor* FoundItemActor = nullptr;

	UE_LOG(LogTemp, Log, TEXT("HandlePickup triggered."));

	if (TraceForItem(FoundItemData, FoundItemActor))
	{
		UE_LOG(LogTemp, Log, TEXT("Trace found item: %s"), *FoundItemData.ItemID.RowName.ToString());
		if (AddItem(FoundItemData))
		{
			UE_LOG(LogTemp, Log, TEXT("Item added successfully."));
			if (FoundItemActor)
			{
				UE_LOG(LogTemp, Log, TEXT("Destroying item actor: %s"), *FoundItemActor->GetName());
				FoundItemActor->Destroy();
			}
			UpdateInventoryUI();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to add item to inventory."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Trace did not find any item."));
	}
}

bool UInventoryComponent::AddItem(const FSlotStruct& ItemToAdd)
{
	UE_LOG(LogTemp, Log, TEXT("AddItem: Trying to add %s (Qty: %d)"), *ItemToAdd.ItemID.RowName.ToString(), ItemToAdd.Quantity);

	if (!ItemDataTable.IsValid() || ItemToAdd.ItemID.RowName.IsNone() || ItemToAdd.Quantity <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("AddItem: Invalid ItemDataTable or ItemToAdd data."));
		return false;
	}

	UDataTable* LoadedItemTable = ItemDataTable.LoadSynchronous();
	if (!LoadedItemTable)
	{
		UE_LOG(LogTemp, Error, TEXT("AddItem: Failed to load ItemDataTable: %s"), *ItemDataTable.ToString());
		return false;
	}

	// --- Logic for Adding Item to TArray<FSlotStruct> based on BP --- 

	// 1. Try to stack with existing items
	for (FSlotStruct& ExistingSlot : InventorySlots)
	{
		// Check if slot has the same item ID (and is not empty)
		if (ExistingSlot.ItemID.RowName == ItemToAdd.ItemID.RowName && !ExistingSlot.ItemID.RowName.IsNone()) 
		{
			// Get Item Definition from Data Table to find StackSize
			const FString ContextString = TEXT("AddItem Stack Check");
			FInventoryItemStruct* ItemDefinition = LoadedItemTable->FindRow<FInventoryItemStruct>(ItemToAdd.ItemID.RowName, ContextString);

			if (ItemDefinition)
			{
				const int32 MaxStackSize = ItemDefinition->StackSize > 0 ? ItemDefinition->StackSize : 1; // Use StackSize from DT, ensure at least 1

				// Check if the existing slot can stack more (Quantity < MaxStackSize)
				if (ExistingSlot.Quantity < MaxStackSize)
				{
					// Calculate how much can be added
					int32 QuantityToAdd = FMath::Min(ItemToAdd.Quantity, MaxStackSize - ExistingSlot.Quantity);

					if (QuantityToAdd > 0)
					{
						ExistingSlot.Quantity += QuantityToAdd;
						UE_LOG(LogTemp, Log, TEXT("Stacked %d of %s. New quantity: %d"), QuantityToAdd, *ItemToAdd.ItemID.RowName.ToString(), ExistingSlot.Quantity);
						
						// NOTE: This BP logic assumes if ANY stacking happens, the function returns true.
						// A more robust system might need to handle remaining quantity if ItemToAdd.Quantity > QuantityToAdd
						// and continue to find empty slots for the remainder.
						// For now, mirroring BP: return true if any amount was stacked.
						return true; 
					}
					else
					{
						// This case should technically not happen if ExistingSlot.Quantity < MaxStackSize, but included for completeness
						UE_LOG(LogTemp, Log, TEXT("Cannot stack %s, calculated QuantityToAdd is 0."), *ItemToAdd.ItemID.RowName.ToString());
					}
				}
				else
				{
					// Existing slot is already full
					UE_LOG(LogTemp, Log, TEXT("Cannot stack %s, existing slot is full (Qty: %d, Max: %d)."), *ItemToAdd.ItemID.RowName.ToString(), ExistingSlot.Quantity, MaxStackSize);
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("AddItem: Could not find item definition for %s in ItemDataTable."), *ItemToAdd.ItemID.RowName.ToString());
				// Decide how to handle - maybe treat as unstackable? For now, continue loop.
			}
		} // End if ItemIDs match
	} // End for loop (stacking check)

	// --- Completed Part (Find Empty Slot) would go here --- 

	// 2. If not stacked, find an empty slot (Placeholder for now, based on previous code)
	for (FSlotStruct& ExistingSlot : InventorySlots)
	{
		if (ExistingSlot.ItemID.RowName.IsNone()) // Check if slot is empty 
		{
			ExistingSlot = ItemToAdd; // Add item to the empty slot
			UE_LOG(LogTemp, Log, TEXT("Added item %s to an empty slot."), *ItemToAdd.ItemID.RowName.ToString());
			return true; // Successfully added to empty slot
		}
	}

	// 3. If no stacking possible and no empty slots
	UE_LOG(LogTemp, Warning, TEXT("Inventory full or could not add item %s."), *ItemToAdd.ItemID.RowName.ToString());
	return false; 
}

void UInventoryComponent::UpdateInventoryUI()
{
	// Check if the widget instance is valid
	if (InventoryWidgetInstance)
	{
		UE_LOG(LogTemp, Log, TEXT("UpdateInventoryUI called. Attempting to update widget."));

		// --- IMPORTANT --- 
		// You MUST cast InventoryWidgetInstance to your specific widget class (e.g., UWBP_Inventory)
		// and call the correct function that expects the TArray<FSlotStruct>.

		// Example assuming your widget class is UWBP_Inventory and has a function called UpdateItemsInInventoryUI
		// UWBP_Inventory* InventoryWidget = Cast<UWBP_Inventory>(InventoryWidgetInstance);
		// if (InventoryWidget)
		// {
		// 	InventoryWidget->UpdateItemsInInventoryUI(InventorySlots); // Pass the TArray
		// 	UE_LOG(LogTemp, Log, TEXT("Called UpdateItemsInInventoryUI on widget."));
		// }
		// else
		// {
		// 	UE_LOG(LogTemp, Error, TEXT("Failed to cast InventoryWidgetInstance to expected widget class."));
		// }
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UpdateInventoryUI called, but InventoryWidgetInstance is null."));
	}
}

bool UInventoryComponent::TraceForItem(FSlotStruct& OutItem, AInventoryItemActor*& OutItemActor)
{
	UE_LOG(LogTemp, Log, TEXT("TraceForItem called."));
	OutItemActor = nullptr; // Initialize output actor

	AActor* Owner = GetOwner();
	if (!Owner) 
	{
		UE_LOG(LogTemp, Error, TEXT("TraceForItem: Owner is null."));
		return false;
	}

	// Calculate Trace Start and End points based on the Blueprint logic
	const FVector StartLocation = Owner->GetActorLocation() + FVector(0.0f, 0.0f, 65.0f);
	const FVector EndLocation = StartLocation + Owner->GetActorForwardVector() * 100.0f;
	const float SphereRadius = 30.0f;

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(Owner); // Ignore self

	FHitResult HitResult;
	bool bHit = UKismetSystemLibrary::SphereTraceSingle(
		GetWorld(),
		StartLocation,
		EndLocation,
		SphereRadius,
		UEngineTypes::ConvertToTraceType(ECC_Visibility), // Trace Channel
		false, // bTraceComplex
		ActorsToIgnore,
		EDrawDebugTrace::None, // Draw Debug Type (set to ForDuration for testing)
		HitResult,
		true  // bIgnoreSelf (redundant as Owner is in ActorsToIgnore, but good practice)
	);

	// Optional: Draw debug sphere
	// DrawDebugSphere(GetWorld(), StartLocation, SphereRadius, 12, FColor::Yellow, false, 1.0f);
	// DrawDebugSphere(GetWorld(), EndLocation, SphereRadius, 12, FColor::Orange, false, 1.0f);
	// DrawDebugLine(GetWorld(), StartLocation, EndLocation, FColor::Red, false, 1.0f);

	if (bHit && HitResult.GetActor())
	{
		UE_LOG(LogTemp, Log, TEXT("SphereTrace hit actor: %s"), *HitResult.GetActor()->GetName());
		// Cast the hit actor to our C++ item actor class
		AInventoryItemActor* HitItemActor = Cast<AInventoryItemActor>(HitResult.GetActor());
		if (HitItemActor)
		{
			UE_LOG(LogTemp, Log, TEXT("Hit actor is an AInventoryItemActor."));
			OutItemActor = HitItemActor;
			OutItem = HitItemActor->GetItemData(); // Use the getter
			return true;
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("Hit actor is NOT an AInventoryItemActor."));
		}
	}
	else if (bHit)
	{
		UE_LOG(LogTemp, Log, TEXT("SphereTrace hit something, but GetActor() was null."));
	}
	else
	{
		//UE_LOG(LogTemp, Log, TEXT("SphereTrace did not hit anything.")); // Can be spammy
	}

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


