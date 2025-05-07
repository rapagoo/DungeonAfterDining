// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Inventory/CookingWidget.h"
#include "Components/Button.h" // Include for UButton
#include "Components/VerticalBox.h" // Include for UVerticalBox
#include "Components/TextBlock.h" // Include for UTextBlock (Example for adding items later)
#include "Components/BoxComponent.h" // Needed for IngredientArea detection
#include "Inventory/InventoryItemActor.h" // Include the item actor class
#include "Inventory/SlotStruct.h" // Needed for FSlotStruct
#include "Inventory/CookingRecipeStruct.h" // Include recipe struct
#include "Engine/DataTable.h" // Ensure DataTable is included
#include "Characters/WarriorHeroCharacter.h" // Include character to get inventory
#include "Inventory/InventoryComponent.h" // Include inventory component
#include "Blueprint/UserWidget.h" // Needed for CreateWidget (if creating ingredient entries)
#include "Interactables/InteractableTable.h"
#include "InteractablePot.h" // Include the Pot header
#include "Kismet/GameplayStatics.h" // Include if getting player character/inventory
#include "Inventory/InvenItemStruct.h" // Include the item definition struct header (Adjust path if needed)

void UCookingWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Bind functions to button click events
	if (AddIngredientButton)
	{
		AddIngredientButton->OnClicked.AddDynamic(this, &UCookingWidget::OnAddIngredientClicked);
		AddIngredientButton->SetIsEnabled(false);
	}
	if (CookButton)
	{
		CookButton->OnClicked.AddDynamic(this, &UCookingWidget::OnCookClicked);
		CookButton->SetIsEnabled(false);
	}
	if (CollectButton)
	{
		CollectButton->OnClicked.AddDynamic(this, &UCookingWidget::OnCollectButtonClicked);
		CollectButton->SetIsEnabled(false);
		CollectButton->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (StatusText)
	{
		StatusText->SetText(FText::FromString(TEXT("")));
	}

	NearbyIngredient = nullptr;

	// Clear any existing ingredients in the UI list on construct
	if (IngredientsList)
	{
		IngredientsList->ClearChildren();
	}
}

void UCookingWidget::UpdateNearbyIngredient(AInventoryItemActor* ItemActor)
{
	bool bShouldEnableButton = false;
	if (ItemActor && ItemActor->IsSliced())
	{
		NearbyIngredient = ItemActor;
		bShouldEnableButton = true;
		UE_LOG(LogTemp, Log, TEXT("UpdateNearbyIngredient: Found nearby sliced item: %s"), *ItemActor->GetName());
	}
	else
	{
		NearbyIngredient = nullptr;
	}

	if (AddIngredientButton)
	{
		AddIngredientButton->SetIsEnabled(bShouldEnableButton);
	}
}

void UCookingWidget::OnAddIngredientClicked()
{
	UE_LOG(LogTemp, Log, TEXT("Add Ingredient button clicked (Attempting to add SLICED version of nearby item to inventory)"));
	if (NearbyIngredient.IsValid() && NearbyIngredient->IsSliced())
	{
		AInventoryItemActor* IngredientActor = NearbyIngredient.Get(); // Renamed for clarity

		// Get Player Character and Inventory Component
		AWarriorHeroCharacter* PlayerCharacter = Cast<AWarriorHeroCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
		if (!PlayerCharacter) { /* Error Log + return */ UE_LOG(LogTemp, Error, TEXT("OnAddIngredientClicked: No Player Character.")); return; }
		UInventoryComponent* PlayerInventory = PlayerCharacter->GetInventoryComponent();
		if (!PlayerInventory) { /* Error Log + return */ UE_LOG(LogTemp, Error, TEXT("OnAddIngredientClicked: No Player Inventory.")); return; }

		// Get the original item data handle from the actor
		FSlotStruct OriginalSlotData = IngredientActor->GetItemData();
		FDataTableRowHandle OriginalItemIDHandle = OriginalSlotData.ItemID;

		// Validate the handle and DataTable pointer
		if (!OriginalItemIDHandle.DataTable || OriginalItemIDHandle.RowName.IsNone())
		{
			UE_LOG(LogTemp, Error, TEXT("OnAddIngredientClicked: Nearby item actor '%s' has invalid ItemID handle (DataTable or RowName missing)."), *IngredientActor->GetName());
            NearbyIngredient = nullptr;
            if(AddIngredientButton) AddIngredientButton->SetIsEnabled(false);
			return;
		}

		// Find the item definition in the DataTable
		FInventoryItemStruct* ItemDefinition = OriginalItemIDHandle.DataTable->FindRow<FInventoryItemStruct>(OriginalItemIDHandle.RowName, TEXT("OnAddIngredientClicked Context"));

		if (!ItemDefinition)
		{
			UE_LOG(LogTemp, Error, TEXT("OnAddIngredientClicked: Could not find Item Definition for '%s' in DataTable '%s'."), *OriginalItemIDHandle.RowName.ToString(), *OriginalItemIDHandle.DataTable->GetName());
            NearbyIngredient = nullptr;
            if(AddIngredientButton) AddIngredientButton->SetIsEnabled(false);
			return;
		}

        // --- Check for and use the SlicedItemID ---
        FName ItemIDToAdd = NAME_None;
        // Assuming the field name is SlicedItemID. Adjust if different.
        if (ItemDefinition->SlicedItemID != NAME_None)
        {
            ItemIDToAdd = ItemDefinition->SlicedItemID;
            UE_LOG(LogTemp, Log, TEXT("Found SlicedItemID '%s' for original item '%s'."), *ItemIDToAdd.ToString(), *OriginalItemIDHandle.RowName.ToString());
        }
        else
        {
            // Fallback or Error? Decide behavior if SlicedItemID is not defined.
            // Option 1: Log error and do nothing
            UE_LOG(LogTemp, Error, TEXT("OnAddIngredientClicked: Original item '%s' does not have a valid SlicedItemID defined in its data table row. Cannot add sliced version."), *OriginalItemIDHandle.RowName.ToString());
            // Option 2: Add the original item back (might be confusing)
            // ItemIDToAdd = OriginalItemIDHandle.RowName;
            // UE_LOG(LogTemp, Warning, TEXT("OnAddIngredientClicked: SlicedItemID not found for '%s'. Adding original item ID instead."), *OriginalItemIDHandle.RowName.ToString());

            // For now, let's prevent adding anything if SlicedItemID is missing
            NearbyIngredient = nullptr;
            if(AddIngredientButton) AddIngredientButton->SetIsEnabled(false);
            return;
        }

        // Create the FSlotStruct for the item to add to inventory
        FSlotStruct ItemDataToAdd;
        ItemDataToAdd.ItemID.RowName = ItemIDToAdd;
        ItemDataToAdd.Quantity = 1;
        // Assume the sliced item uses the same DataTable. If not, ItemDefinition needs to specify it.
        ItemDataToAdd.ItemID.DataTable = OriginalItemIDHandle.DataTable;

		UE_LOG(LogTemp, Log, TEXT("Attempting to add SLICED item '%s' to player inventory."), *ItemIDToAdd.ToString());

		// Attempt to add the item back to the player's inventory
		if (PlayerInventory->AddItem(ItemDataToAdd))
		{
			UE_LOG(LogTemp, Log, TEXT("Successfully added SLICED item '%s' to inventory from nearby actor %s."), *ItemIDToAdd.ToString(), *IngredientActor->GetName());

			// Destroy the original actor in the world
			IngredientActor->Destroy();

			// Clear the nearby ingredient pointer and disable the button
			NearbyIngredient = nullptr;
			if (AddIngredientButton)
			{
				AddIngredientButton->SetIsEnabled(false);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to add SLICED item '%s' to player inventory (Inventory might be full?). Actor '%s' remains."), *ItemIDToAdd.ToString(), *IngredientActor->GetName());
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Add Ingredient Clicked, but NearbyIngredient is not valid or not sliced. Stored pointer: %s"), NearbyIngredient.IsValid() ? *NearbyIngredient->GetName() : TEXT("Invalid"));
        if (AddIngredientButton)
		{
			AddIngredientButton->SetIsEnabled(false);
		}
	}
}

void UCookingWidget::OnCookClicked()
{
	UE_LOG(LogTemp, Log, TEXT("Cook Button Clicked!"));

	UE_LOG(LogTemp, Log, TEXT("[OnCookClicked] Checking AssociatedInteractable: %s"), AssociatedInteractable ? *AssociatedInteractable->GetName() : TEXT("nullptr"));

	if (AssociatedInteractable)
	{
		AInteractablePot* Pot = Cast<AInteractablePot>(AssociatedInteractable);
		if (Pot)
		{
			UE_LOG(LogTemp, Log, TEXT("Attempting to start cooking on Pot: %s"), *Pot->GetName());
			Pot->StartCooking();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Cook Button Clicked, but AssociatedInteractable is not an AInteractablePot."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Cook Button Clicked, but AssociatedInteractable is null."));
	}
}

void UCookingWidget::OnCollectButtonClicked()
{
    UE_LOG(LogTemp, Log, TEXT("Collect Button Clicked!"));
    if (AssociatedInteractable)
    {
        AInteractablePot* Pot = Cast<AInteractablePot>(AssociatedInteractable);
        if (Pot)
        {
            UE_LOG(LogTemp, Log, TEXT("Attempting to collect item from Pot: %s"), *Pot->GetName());
            bool bCollected = Pot->CollectCookedItem();
            if (bCollected)
            {
                UE_LOG(LogTemp, Log, TEXT("UCookingWidget: Item collected successfully via UI."));
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("UCookingWidget: CollectCookedItem returned false (inventory full or item not ready?)."));
            }
            // Pot->CollectCookedItem() should call NotifyWidgetUpdate internally to refresh the widget.
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Collect Button Clicked, but AssociatedInteractable is not an AInteractablePot."));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Collect Button Clicked, but AssociatedInteractable is null."));
    }
}

void UCookingWidget::UpdateWidgetState(const TArray<FName>& IngredientIDs, bool bIsPotCooking, bool bIsPotCookingComplete, bool bIsPotBurnt, FName CookedResultID)
{
    UE_LOG(LogTemp, Log, TEXT("UCookingWidget::UpdateWidgetState (C++) - Cooking: %d, Complete: %d, Burnt: %d. Ingredients: %d"),
        bIsPotCooking, bIsPotCookingComplete, bIsPotBurnt, IngredientIDs.Num());

    // 1. Update Ingredient Display (C++ Logic)
    if (IngredientsList)
    {
        IngredientsList->ClearChildren(); 

        for (const FName& IngredientID : IngredientIDs)
        {
            UTextBlock* NewIngredientText = NewObject<UTextBlock>(this, UTextBlock::StaticClass()); 
            if (NewIngredientText)
            {
                FString DisplayName = IngredientID.ToString(); 
                AInteractablePot* Pot = Cast<AInteractablePot>(AssociatedInteractable);
                
                UDataTable* CurrentItemTable = Pot ? Pot->GetItemDataTable() : nullptr;
                if (CurrentItemTable && !IngredientID.IsNone()) 
                {
                    const FString ContextString(TEXT("Fetching Ingredient Name for Widget List"));
                    FInventoryItemStruct* ItemData = CurrentItemTable->FindRow<FInventoryItemStruct>(IngredientID, ContextString);
                    if (ItemData && !ItemData->Name.IsEmpty()) 
                    {
                        DisplayName = ItemData->Name.ToString();
                    }
                    else if(ItemData)
                    {
                         UE_LOG(LogTemp, Warning, TEXT("UCookingWidget: ItemData found for %s in list, but Name is empty. Using RowName."), *IngredientID.ToString());
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("UCookingWidget: Could not find ItemData for ingredient ID: %s in ItemDataTable for list. Using RowName."), *IngredientID.ToString());
                    }
                }
                NewIngredientText->SetText(FText::FromString(DisplayName));
                IngredientsList->AddChildToVerticalBox(NewIngredientText); 
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to create UTextBlock for Ingredient ID: %s"), *IngredientID.ToString());
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("IngredientsList (UVerticalBox*) is null in CookingWidget. Cannot display ingredients."));
    }

    // 2. Update Status Text and Button States
    if (StatusText)
    {
        if (bIsPotBurnt)
        {
            StatusText->SetText(FText::FromString(TEXT("타버렸습니다!")));
        }
        else if (bIsPotCookingComplete)
        {
            FString ItemName = CookedResultID.ToString(); 
            AInteractablePot* Pot = Cast<AInteractablePot>(AssociatedInteractable);

            UDataTable* CurrentItemTableForResult = Pot ? Pot->GetItemDataTable() : nullptr;
            if (CurrentItemTableForResult && !CookedResultID.IsNone())
            {
                const FString ContextString(TEXT("Fetching Cooked Item Name for Widget Status"));
                FInventoryItemStruct* ItemData = CurrentItemTableForResult->FindRow<FInventoryItemStruct>(CookedResultID, ContextString);
                if (ItemData && !ItemData->Name.IsEmpty())
                {
                    ItemName = ItemData->Name.ToString();
                }
                else if(ItemData)
                {
                     UE_LOG(LogTemp, Warning, TEXT("UCookingWidget: ItemData found for cooked result %s, but Name is empty. Using RowName for status."), *CookedResultID.ToString());
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("UCookingWidget: Could not find ItemData for cooked result ID: %s in ItemDataTable for status. Using RowName."), *CookedResultID.ToString());
                }
            }
            StatusText->SetText(FText::Format(FText::FromString(TEXT("{0} 완성! 획득하세요.")), FText::FromString(ItemName)));
        }
        else if (bIsPotCooking)
        {
            StatusText->SetText(FText::FromString(TEXT("요리 중...")));
        }
        else // Idle
        {
            if (IngredientIDs.Num() > 0)
            {
                StatusText->SetText(FText::FromString(TEXT("요리하시겠습니까?")));
            }
            else
            {
                StatusText->SetText(FText::FromString(TEXT("재료를 넣어주세요.")));
            }
        }
    }

    if (CookButton)
    {
        bool bCanCook = !bIsPotCooking && !bIsPotCookingComplete && !bIsPotBurnt && IngredientIDs.Num() > 0;
        CookButton->SetIsEnabled(bCanCook);
        CookButton->SetVisibility(bCanCook ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    if (CollectButton)
    {
        bool bCanCollect = bIsPotCookingComplete && !bIsPotBurnt;
        CollectButton->SetIsEnabled(bCanCollect);
        CollectButton->SetVisibility(bCanCollect ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
}

void UCookingWidget::SetAssociatedTable(AInteractableTable* Table)
{
	UE_LOG(LogTemp, Log, TEXT("[SetAssociatedTable] Setting AssociatedInteractable to: %s"), Table ? *Table->GetName() : TEXT("nullptr"));

	AssociatedInteractable = Table;

	AInteractablePot* Pot = Cast<AInteractablePot>(AssociatedInteractable);
	if (Pot)
	{
		// Pot will call NotifyWidgetUpdate which calls UpdateWidgetState
        // This ensures the widget reflects the pot's current state when first associated
        Pot->NotifyWidgetUpdate();
	}
	else
	{
        // If not a pot, or pot is null, set a default "empty" state
        UpdateWidgetState({}, false, false, false, NAME_None);
		UE_LOG(LogTemp, Warning, TEXT("UCookingWidget::SetAssociatedTable - Associated table is not an InteractablePot or is null. Widget will show default state."));
	}
	UpdateNearbyIngredient(FindNearbySlicedIngredient()); // This seems to be for a different "add ingredient to inventory" feature
}

AInventoryItemActor* UCookingWidget::FindNearbySlicedIngredient()
{
	if (!AssociatedInteractable)
	{
		return nullptr;
	}

	UBoxComponent* DetectionArea = AssociatedInteractable->FindComponentByClass<UBoxComponent>();
	if(!DetectionArea)
	{
        // Fallback to a common name if the direct component isn't found (e.g. inherited)
		DetectionArea = Cast<UBoxComponent>(AssociatedInteractable->GetDefaultSubobjectByName(FName("InteractionVolume")));
        if(!DetectionArea)
        {
            DetectionArea = Cast<UBoxComponent>(AssociatedInteractable->GetDefaultSubobjectByName(FName("IngredientArea")));
        }
	}

	if (!DetectionArea)
	{
		UE_LOG(LogTemp, Warning, TEXT("FindNearbySlicedIngredient: Could not find a suitable BoxComponent on %s."), *AssociatedInteractable->GetName());
		return nullptr;
	}

	TArray<AActor*> OverlappingActors;
	DetectionArea->GetOverlappingActors(OverlappingActors, AInventoryItemActor::StaticClass());

	for (AActor* Actor : OverlappingActors)
	{
		AInventoryItemActor* ItemActor = Cast<AInventoryItemActor>(Actor);
		if (ItemActor && ItemActor->IsSliced())
		{
			UE_LOG(LogTemp, Log, TEXT("FindNearbySlicedIngredient: Found valid sliced item: %s"), *ItemActor->GetName());
			return ItemActor;
		}
	}
	return nullptr;
}

/* // DEPRECATED: Add ingredient logic handled by Pot
void UCookingWidget::AddIngredient(FName IngredientID)
{
	// This function might not be needed if AInteractablePot directly manages
	// the ingredients and calls UpdateIngredientList.
	// Keeping it for now in case old logic relies on it.
	UE_LOG(LogTemp, Log, TEXT("UCookingWidget::AddIngredient called for %s (potentially deprecated)"), *IngredientID.ToString());
	// AddedIngredientIDs.AddUnique(IngredientID); // Example: Add to internal list
	// UpdateIngredientList(AddedIngredientIDs); // Update UI
}
*/

/* // DEPRECATED: Recipe Check logic handled by Pot
FName UCookingWidget::CheckRecipe()
{
	// This logic should ideally live entirely within AInteractablePot::CheckRecipeInternal
	// This implementation is kept as a placeholder based on the original header.
	UE_LOG(LogTemp, Warning, TEXT("UCookingWidget::CheckRecipe called - This logic should be handled by AInteractablePot."));

	if (!RecipeDataTable || AddedIngredientIDs.Num() == 0)
	{
		return NAME_None;
	}
	TArray<FName> SortedCurrentIngredients = AddedIngredientIDs;
	SortedCurrentIngredients.Sort([](const FName& A, const FName& B) { return A.ToString() < B.ToString(); });

	const TArray<FName> RowNames = RecipeDataTable->GetRowNames();
	for (const FName& RowName : RowNames)
	{
		FCookingRecipeStruct* RecipeRow = RecipeDataTable->FindRow<FCookingRecipeStruct>(RowName, TEXT("WidgetCheckRecipe"));
		if (RecipeRow && RecipeRow->InputIngredients.Num() == SortedCurrentIngredients.Num()) // ERROR: InputIngredients undefined here
		{
			TArray<FName> SortedRecipeIngredients = RecipeRow->InputIngredients; // ERROR: InputIngredients undefined here
			SortedRecipeIngredients.Sort([](const FName& A, const FName& B) { return A.ToString() < B.ToString(); });
			if (SortedCurrentIngredients == SortedRecipeIngredients)
			{
				return RecipeRow->OutputItemID; // ERROR: OutputItemID undefined here
			}
		}
	}
	return NAME_None;
}
*/

/* // DEPRECATED: Find ingredient logic handled by Character interaction
AInventoryItemActor* UCookingWidget::FindNearbySlicedIngredient()
{
	// This logic is likely replaced by the character directly interacting with the pot.
	UE_LOG(LogTemp, Warning, TEXT("UCookingWidget::FindNearbySlicedIngredient called (DEPRECATED)"));
	if (!AssociatedInteractable) return nullptr; // ERROR: AssociatedInteractable undefined here

	TArray<AActor*> OverlappingActors;
	// Need a component on the table (like IngredientArea in the original description) to get overlaps
	// AssociatedTable->IngredientArea->GetOverlappingActors(OverlappingActors, AInventoryItemActor::StaticClass()); // Example

	for (AActor* Actor : OverlappingActors)
	{
		AInventoryItemActor* Item = Cast<AInventoryItemActor>(Actor);
		// Check if item is valid and sliced (assuming IsSliced exists)
		if (Item && Item->IsSliced())
		{
			return Item; // Return the first sliced item found
		}
	}
	return nullptr;
}
*/ 