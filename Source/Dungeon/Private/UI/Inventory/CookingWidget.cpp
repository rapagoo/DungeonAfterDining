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
	if (StirButton)
	{
		StirButton->OnClicked.AddDynamic(this, &UCookingWidget::OnStirButtonClicked);
		StirButton->SetIsEnabled(false);
		StirButton->SetVisibility(ESlateVisibility::Collapsed);
	}

	// NEW: Grilling minigame buttons
	if (FlipButton)
	{
		FlipButton->OnClicked.AddDynamic(this, &UCookingWidget::OnFlipButtonClicked);
		FlipButton->SetIsEnabled(false);
		FlipButton->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (HeatUpButton)
	{
		HeatUpButton->OnClicked.AddDynamic(this, &UCookingWidget::OnHeatUpButtonClicked);
		HeatUpButton->SetIsEnabled(false);
		HeatUpButton->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (HeatDownButton)
	{
		HeatDownButton->OnClicked.AddDynamic(this, &UCookingWidget::OnHeatDownButtonClicked);
		HeatDownButton->SetIsEnabled(false);
		HeatDownButton->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (CheckButton)
	{
		CheckButton->OnClicked.AddDynamic(this, &UCookingWidget::OnCheckButtonClicked);
		CheckButton->SetIsEnabled(false);
		CheckButton->SetVisibility(ESlateVisibility::Collapsed);
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

    // Handle AddIngredient button visibility - only show for InteractableTable, not for InteractablePot
    if (AddIngredientButton)
    {
        AInteractablePot* Pot = Cast<AInteractablePot>(AssociatedInteractable);
        if (Pot)
        {
            // For InteractablePot, hide the AddIngredient button as it's not used
            AddIngredientButton->SetVisibility(ESlateVisibility::Collapsed);
        }
        else
        {
            // For InteractableTable, show the AddIngredient button and update its state
            AddIngredientButton->SetVisibility(ESlateVisibility::Visible);
            // Enable/disable based on whether there's a nearby sliced ingredient
            bool bHasNearbyIngredient = NearbyIngredient.IsValid();
            AddIngredientButton->SetIsEnabled(bHasNearbyIngredient);
        }
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

// NEW: Timing minigame functions
void UCookingWidget::OnStirButtonClicked()
{
	UE_LOG(LogTemp, Log, TEXT("Stir Button Clicked!"));
	
	// NEW: Check if we're in minigame mode first
	if (bIsInMinigameMode && CurrentMinigame.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("OnStirButtonClicked - Processing as minigame input"));
		// 굽기 미니게임에서는 Flip 입력으로 처리
		HandleMinigameInput(TEXT("Flip"));
		return;
	}
	
	// 기존 타이밍 시스템 처리
	if (!bIsTimingEventActive)
	{
		UE_LOG(LogTemp, Warning, TEXT("Stir button clicked but no timing event is active."));
		return;
	}

	// Calculate if the click was within the allowed time window
	float CurrentTime = GetWorld()->GetTimeSeconds();
	float TimeElapsed = CurrentTime - TimingEventStartTime;
	
	if (TimeElapsed <= TimingEventDuration)
	{
		// Success!
		UE_LOG(LogTemp, Log, TEXT("Timing event SUCCESS! Responded in %.2f seconds"), TimeElapsed);
		
		// Hide the stir button
		if (StirButton)
		{
			StirButton->SetVisibility(ESlateVisibility::Collapsed);
			StirButton->SetIsEnabled(false);
		}
		
		// Update status text
		if (StatusText)
		{
			StatusText->SetText(FText::FromString(TEXT("성공! 요리를 획득하세요.")));
		}
		
		// Notify the pot about successful timing event
		AInteractablePot* Pot = Cast<AInteractablePot>(AssociatedInteractable);
		if (Pot)
		{
			Pot->OnTimingEventSuccess();
			UE_LOG(LogTemp, Log, TEXT("Notifying pot about successful timing event"));
		}
		
		// Reset timing event state
		bIsTimingEventActive = false;
		TimingEventStartTime = 0.0f;
	}
	else
	{
		// Too late - this shouldn't happen if we properly hide the button on timeout
		UE_LOG(LogTemp, Warning, TEXT("Stir button clicked too late! Time elapsed: %.2f"), TimeElapsed);
		CheckTimingEventTimeout(); // Force timeout handling
	}
}

void UCookingWidget::StartTimingEvent()
{
	UE_LOG(LogTemp, Log, TEXT("Starting timing event!"));
	
	if (bIsTimingEventActive)
	{
		UE_LOG(LogTemp, Warning, TEXT("Timing event already active, ignoring new start request"));
		return;
	}
	
	// Set timing event state
	bIsTimingEventActive = true;
	TimingEventStartTime = GetWorld()->GetTimeSeconds();
	
	// Show and enable the stir button
	if (StirButton)
	{
		StirButton->SetVisibility(ESlateVisibility::Visible);
		StirButton->SetIsEnabled(true);
	}
	
	// Update status text
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(TEXT("지금 저어주세요!")));
	}
	
	// Set up a timer to check for timeout
	FTimerHandle TimingEventTimer;
	GetWorld()->GetTimerManager().SetTimer(TimingEventTimer, this, &UCookingWidget::CheckTimingEventTimeout, TimingEventDuration, false);
}

void UCookingWidget::CheckTimingEventTimeout()
{
	if (!bIsTimingEventActive)
	{
		return; // Event already handled
	}
	
	float CurrentTime = GetWorld()->GetTimeSeconds();
	float TimeElapsed = CurrentTime - TimingEventStartTime;
	
	if (TimeElapsed >= TimingEventDuration)
	{
		// Timeout - player failed to respond in time
		UE_LOG(LogTemp, Log, TEXT("Timing event FAILED! Player didn't respond in time (%.2f seconds)"), TimeElapsed);
		
		// Hide the stir button
		if (StirButton)
		{
			StirButton->SetVisibility(ESlateVisibility::Collapsed);
			StirButton->SetIsEnabled(false);
		}
		
		// Update status text
		if (StatusText)
		{
			StatusText->SetText(FText::FromString(TEXT("놓쳤어요... 요리 중...")));
		}
		
		// Notify the pot about failed timing event
		AInteractablePot* Pot = Cast<AInteractablePot>(AssociatedInteractable);
		if (Pot)
		{
			Pot->OnTimingEventFailure();
			UE_LOG(LogTemp, Log, TEXT("Notifying pot about failed timing event"));
		}
		
		// Reset timing event state
		bIsTimingEventActive = false;
		TimingEventStartTime = 0.0f;
	}
}

// NEW: Minigame system functions
void UCookingWidget::OnMinigameStarted(UCookingMinigameBase* Minigame)
{
	if (!Minigame)
	{
		UE_LOG(LogTemp, Warning, TEXT("UCookingWidget::OnMinigameStarted - Invalid minigame"));
		return;
	}

	CurrentMinigame = Minigame;
	bIsInMinigameMode = true;

	UE_LOG(LogTemp, Log, TEXT("UCookingWidget::OnMinigameStarted - Minigame started"));

	// 미니게임 종류 확인
	FString MinigameType = Minigame->GetClass()->GetName();
	bool bIsGrillingGame = MinigameType.Contains(TEXT("Grilling"));
	bool bIsRhythmGame = MinigameType.Contains(TEXT("Rhythm"));

	// UI 업데이트 - 미니게임 모드 활성화
	if (StatusText)
	{
		if (bIsGrillingGame)
		{
			StatusText->SetText(FText::FromString(TEXT("🍖 굽기 미니게임 시작! 뒤집기, 화력 조절, 확인 버튼을 사용하세요!")));
		}
		else if (bIsRhythmGame)
		{
			StatusText->SetText(FText::FromString(TEXT("🎵 리듬 미니게임 시작! 'Stir' 버튼을 타이밍에 맞춰 누르세요!")));
		}
		else
		{
			StatusText->SetText(FText::FromString(TEXT("🎮 미니게임 시작!")));
		}
	}

	// ActionText 초기화
	if (ActionText)
	{
		ActionText->SetText(FText::FromString(TEXT("⏳ 액션을 기다려주세요...")));
	}

	// 미니게임 종류에 따른 버튼 표시
	if (bIsGrillingGame)
	{
		// 굽기 미니게임: Flip, HeatUp, HeatDown, Check 버튼 표시
		if (FlipButton)
		{
			FlipButton->SetVisibility(ESlateVisibility::Visible);
			FlipButton->SetIsEnabled(false); // 타이밍에 따라 활성화/비활성화
		}
		if (HeatUpButton)
		{
			HeatUpButton->SetVisibility(ESlateVisibility::Visible);
			HeatUpButton->SetIsEnabled(false);
		}
		if (HeatDownButton)
		{
			HeatDownButton->SetVisibility(ESlateVisibility::Visible);
			HeatDownButton->SetIsEnabled(false);
		}
		if (CheckButton)
		{
			CheckButton->SetVisibility(ESlateVisibility::Visible);
			CheckButton->SetIsEnabled(false);
		}
		// Stir 버튼 숨기기
		if (StirButton)
		{
			StirButton->SetVisibility(ESlateVisibility::Hidden);
		}
	}
	else
	{
		// 리듬 미니게임: Stir 버튼만 표시
		if (StirButton)
		{
			StirButton->SetVisibility(ESlateVisibility::Visible);
			StirButton->SetIsEnabled(false);
		}
		// 굽기 버튼들 숨기기
		if (FlipButton) FlipButton->SetVisibility(ESlateVisibility::Hidden);
		if (HeatUpButton) HeatUpButton->SetVisibility(ESlateVisibility::Hidden);
		if (HeatDownButton) HeatDownButton->SetVisibility(ESlateVisibility::Hidden);
		if (CheckButton) CheckButton->SetVisibility(ESlateVisibility::Hidden);
	}

	// 다른 버튼들 비활성화
	if (CookButton)
	{
		CookButton->SetIsEnabled(false);
	}
	if (AddIngredientButton)
	{
		AddIngredientButton->SetIsEnabled(false);
	}
}

void UCookingWidget::OnMinigameUpdated(float Score, int32 Phase)
{
	if (!bIsInMinigameMode)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("UCookingWidget::OnMinigameUpdated - Score: %.2f, Phase: %d"), Score, Phase);

	// StatusText는 점수와 전체 상태만 표시 (ActionText는 별도로 관리)
	if (StatusText)
	{
		FString StatusMessage;
		
		if (Score < 0)
		{
			StatusMessage = FString::Printf(TEXT("🎮 미니게임 | 점수: %.0f | ❌ 실패!"), Score);
		}
		else
		{
			StatusMessage = FString::Printf(TEXT("🎮 미니게임 | 점수: %.0f"), Score);
		}
		
		StatusText->SetText(FText::FromString(StatusMessage));
	}

	// 버튼 상태는 UpdateRequiredAction에서만 관리하므로 여기서는 터치하지 않음
	UE_LOG(LogTemp, Log, TEXT("UCookingWidget::OnMinigameUpdated - Status updated, buttons managed by UpdateRequiredAction"));
}

void UCookingWidget::OnMinigameEnded(int32 Result)
{
	bIsInMinigameMode = false;
	CurrentMinigame = nullptr;

	UE_LOG(LogTemp, Log, TEXT("UCookingWidget::OnMinigameEnded - Result: %d"), Result);

	// 결과에 따른 메시지 표시
	if (StatusText)
	{
		FString ResultMessage;
		switch (Result)
		{
		case 1: // Perfect
			ResultMessage = TEXT("🌟 완벽한 요리! 최고입니다!");
			break;
		case 2: // Good
			ResultMessage = TEXT("👍 좋은 요리! 잘 하셨어요!");
			break;
		case 3: // Average
			ResultMessage = TEXT("😊 평범한 요리 괜찮네요");
			break;
		case 4: // Poor
			ResultMessage = TEXT("😕 아쉬운 요리... 다음엔 더 잘할 수 있어요");
			break;
		case 5: // Failed
			ResultMessage = TEXT("💔 요리 실패! 다시 시도해보세요");
			break;
		default:
			ResultMessage = TEXT("✅ 요리 완료! 수거하세요");
			break;
		}
		StatusText->SetText(FText::FromString(ResultMessage));
	}

	// ActionText 정리
	if (ActionText)
	{
		ActionText->SetText(FText::FromString(TEXT("")));
	}

	// 미니게임 버튼들 숨기기
	if (StirButton)
	{
		StirButton->SetVisibility(ESlateVisibility::Hidden);
		StirButton->SetIsEnabled(false);
	}
	if (FlipButton)
	{
		FlipButton->SetVisibility(ESlateVisibility::Hidden);
		FlipButton->SetIsEnabled(false);
	}
	if (HeatUpButton)
	{
		HeatUpButton->SetVisibility(ESlateVisibility::Hidden);
		HeatUpButton->SetIsEnabled(false);
	}
	if (HeatDownButton)
	{
		HeatDownButton->SetVisibility(ESlateVisibility::Hidden);
		HeatDownButton->SetIsEnabled(false);
	}
	if (CheckButton)
	{
		CheckButton->SetVisibility(ESlateVisibility::Hidden);
		CheckButton->SetIsEnabled(false);
	}

	// 수거 버튼 활성화 (요리가 완료되었으므로)
	if (CollectButton)
	{
		CollectButton->SetVisibility(ESlateVisibility::Visible);
		CollectButton->SetIsEnabled(true);
	}

	// 다른 버튼들 상태 복원
	if (CookButton)
	{
		CookButton->SetIsEnabled(false); // 이미 요리했으므로 비활성화
	}
	if (AddIngredientButton)
	{
		AddIngredientButton->SetIsEnabled(false); // 요리 중이므로 비활성화
	}
}

void UCookingWidget::HandleMinigameInput(const FString& InputType)
{
	if (!bIsInMinigameMode || !CurrentMinigame.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("UCookingWidget::HandleMinigameInput - Not in minigame mode or invalid minigame"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("UCookingWidget::HandleMinigameInput - Input: %s"), *InputType);

	// 현재 점수를 저장해서 변화를 감지
	float PreviousScore = CurrentMinigame->GetCurrentScore();

	// 미니게임에 입력 전달
	CurrentMinigame->HandlePlayerInput(InputType);

	// 점수 변화를 확인하여 피드백 제공
	float NewScore = CurrentMinigame->GetCurrentScore();
	float ScoreDifference = NewScore - PreviousScore;

	// 시각적 피드백 제공
	if (StatusText)
	{
		FString FeedbackMessage;
		if (ScoreDifference > 0)
		{
			// 성공적인 입력
			if (ScoreDifference >= 100.0f)
			{
				FeedbackMessage = FString::Printf(TEXT("🌟 PERFECT! +%.0f 점"), ScoreDifference);
			}
			else if (ScoreDifference >= 75.0f)
			{
				FeedbackMessage = FString::Printf(TEXT("👍 GOOD! +%.0f 점"), ScoreDifference);
			}
			else
			{
				FeedbackMessage = FString::Printf(TEXT("✅ HIT! +%.0f 점"), ScoreDifference);
			}
		}
		else if (ScoreDifference < 0)
		{
			// 실패한 입력 (점수 차감)
			FeedbackMessage = FString::Printf(TEXT("❌ 타이밍 실패! %.0f 점"), ScoreDifference);
		}
		else
		{
			// 점수 변화 없음 (이미 처리된 이벤트나 잘못된 타이밍)
			FeedbackMessage = TEXT("⏸️ 아직 타이밍이 아닙니다!");
		}
		
		StatusText->SetText(FText::FromString(FeedbackMessage));
	}

	// 버튼 상태는 UpdateMinigame에서 관리됨
	UE_LOG(LogTemp, Log, TEXT("UCookingWidget::HandleMinigameInput - Score change: %.2f"), ScoreDifference);
}

// NEW: Grilling minigame button handlers
void UCookingWidget::OnFlipButtonClicked()
{
	UE_LOG(LogTemp, Log, TEXT("Flip Button Clicked!"));
	if (bIsInMinigameMode && CurrentMinigame.IsValid())
	{
		HandleMinigameInput(TEXT("Flip"));
	}
}

void UCookingWidget::OnHeatUpButtonClicked()
{
	UE_LOG(LogTemp, Log, TEXT("Heat Up Button Clicked!"));
	if (bIsInMinigameMode && CurrentMinigame.IsValid())
	{
		HandleMinigameInput(TEXT("HeatUp"));
	}
}

void UCookingWidget::OnHeatDownButtonClicked()
{
	UE_LOG(LogTemp, Log, TEXT("Heat Down Button Clicked!"));
	if (bIsInMinigameMode && CurrentMinigame.IsValid())
	{
		HandleMinigameInput(TEXT("HeatDown"));
	}
}

void UCookingWidget::OnCheckButtonClicked()
{
	UE_LOG(LogTemp, Log, TEXT("Check Button Clicked!"));
	if (bIsInMinigameMode && CurrentMinigame.IsValid())
	{
		HandleMinigameInput(TEXT("Check"));
	}
}

void UCookingWidget::UpdateRequiredAction(const FString& ActionType, bool bActionRequired)
{
	if (!bIsInMinigameMode)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("UCookingWidget::UpdateRequiredAction - Action: %s, Required: %s"), 
		   *ActionType, bActionRequired ? TEXT("true") : TEXT("false"));

	// 모든 미니게임 버튼을 비활성화
	if (FlipButton) 
	{
		FlipButton->SetIsEnabled(false);
		UE_LOG(LogTemp, Log, TEXT("UCookingWidget::UpdateRequiredAction - FlipButton disabled"));
	}
	if (HeatUpButton) 
	{
		HeatUpButton->SetIsEnabled(false);
		UE_LOG(LogTemp, Log, TEXT("UCookingWidget::UpdateRequiredAction - HeatUpButton disabled"));
	}
	if (HeatDownButton) 
	{
		HeatDownButton->SetIsEnabled(false);
		UE_LOG(LogTemp, Log, TEXT("UCookingWidget::UpdateRequiredAction - HeatDownButton disabled"));
	}
	if (CheckButton) 
	{
		CheckButton->SetIsEnabled(false);
		UE_LOG(LogTemp, Log, TEXT("UCookingWidget::UpdateRequiredAction - CheckButton disabled"));
	}
	if (StirButton) 
	{
		StirButton->SetIsEnabled(false);
		UE_LOG(LogTemp, Log, TEXT("UCookingWidget::UpdateRequiredAction - StirButton disabled"));
	}

	if (bActionRequired && !ActionType.IsEmpty())
	{
		// 필요한 액션에 따라 해당 버튼만 활성화
		if (ActionType == TEXT("Flip"))
		{
			if (FlipButton)
			{
				FlipButton->SetIsEnabled(true);
				UE_LOG(LogTemp, Log, TEXT("UCookingWidget::UpdateRequiredAction - FlipButton ENABLED"));
			}
			if (ActionText)
			{
				ActionText->SetText(FText::FromString(TEXT("🔄 지금 뒤집으세요!")));
			}
		}
		else if (ActionType == TEXT("HeatUp"))
		{
			if (HeatUpButton)
			{
				HeatUpButton->SetIsEnabled(true);
				UE_LOG(LogTemp, Log, TEXT("UCookingWidget::UpdateRequiredAction - HeatUpButton ENABLED"));
			}
			if (ActionText)
			{
				ActionText->SetText(FText::FromString(TEXT("🔥 화력을 높이세요!")));
			}
		}
		else if (ActionType == TEXT("HeatDown"))
		{
			if (HeatDownButton)
			{
				HeatDownButton->SetIsEnabled(true);
				UE_LOG(LogTemp, Log, TEXT("UCookingWidget::UpdateRequiredAction - HeatDownButton ENABLED"));
			}
			if (ActionText)
			{
				ActionText->SetText(FText::FromString(TEXT("❄️ 화력을 낮추세요!")));
			}
		}
		else if (ActionType == TEXT("Check"))
		{
			if (CheckButton)
			{
				CheckButton->SetIsEnabled(true);
				UE_LOG(LogTemp, Log, TEXT("UCookingWidget::UpdateRequiredAction - CheckButton ENABLED"));
			}
			if (ActionText)
			{
				ActionText->SetText(FText::FromString(TEXT("👀 익힘 정도를 확인하세요!")));
			}
		}
		else if (ActionType == TEXT("Flip") && StirButton) // 리듬 게임에서 Flip은 Stir로 처리
		{
			if (StirButton)
			{
				StirButton->SetIsEnabled(true);
				UE_LOG(LogTemp, Log, TEXT("UCookingWidget::UpdateRequiredAction - StirButton ENABLED for Flip"));
			}
			if (ActionText)
			{
				ActionText->SetText(FText::FromString(TEXT("🎵 지금 젓으세요!")));
			}
		}
	}
	else
	{
		// 액션이 필요하지 않을 때
		if (ActionText)
		{
			ActionText->SetText(FText::FromString(TEXT("⏳ 다음 액션을 기다려주세요...")));
		}
	}
} 