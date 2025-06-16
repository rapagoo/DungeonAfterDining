// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Inventory/CookingWidget.h"
#include "Components/Button.h" // Include for UButton
#include "Components/VerticalBox.h" // Include for UVerticalBox
#include "Components/TextBlock.h" // Include for UTextBlock (Example for adding items later)
#include "Components/BoxComponent.h" // Needed for IngredientArea detection
#include "Components/Image.h" // Include for UImage (rhythm game circles)
#include "Components/Overlay.h" // Include for UOverlay (rhythm game overlay)
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
#include "Cooking/FryingRhythmMinigame.h" // Include for UFryingRhythmMinigame casting

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

	// Initialize rhythm game UI to be hidden
	if (RhythmGameOverlay)
	{
		RhythmGameOverlay->SetVisibility(ESlateVisibility::Collapsed);
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
		// 냄비와 상호작용 시 리듬 게임 UI가 보일 수 있도록 (필요하다면 OnMinigameStarted에서 다시 설정됨)
		if (RhythmGameOverlay)
		{
			RhythmGameOverlay->SetVisibility(ESlateVisibility::Collapsed); // 기본은 숨김, 필요시 MinigameStarted에서 보이게 함
		}
	}
	else
	{
        // If not a pot, or pot is null, set a default "empty" state
        UpdateWidgetState({}, false, false, false, NAME_None);
		UE_LOG(LogTemp, Warning, TEXT("UCookingWidget::SetAssociatedTable - Associated table is not an InteractablePot or is null. Widget will show default state."));
		// 테이블과 상호작용 시 리듬 게임 UI를 확실히 숨김
		if (RhythmGameOverlay)
		{
			RhythmGameOverlay->SetVisibility(ESlateVisibility::Collapsed);
			UE_LOG(LogTemp, Log, TEXT("UCookingWidget::SetAssociatedTable - Hiding RhythmGameOverlay for InteractableTable."));
		}
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
	UE_LOG(LogTemp, Verbose, TEXT("OnStirButtonClicked - bIsFryingGame: %s, bIsRhythmNoteActive: %s, CurrentRhythmAction: %s"), 
		bIsFryingGame ? TEXT("true") : TEXT("false"),
		bIsRhythmNoteActive ? TEXT("true") : TEXT("false"),
		*CurrentRhythmAction);

	if (bIsInMinigameMode && CurrentMinigame.IsValid())
	{
		// 타이머 기반 미니게임 확인
		FString MinigameType = CurrentMinigame->GetClass()->GetName();
		if (MinigameType.Contains(TEXT("TimerBased")))
		{
			// 타이머 기반 미니게임에서는 ActionButton 또는 SpaceBar 액션 사용
			FString ActionToSend = TEXT("ActionButton");
			UE_LOG(LogTemp, Log, TEXT("OnStirButtonClicked - Timer-based minigame, sending action: %s"), *ActionToSend);
			HandleMinigameInput(ActionToSend);
		}
		else
		{
			// 기존 미니게임들 (리듬 게임 등)
			FString ActionToSend = TEXT("Stir");
			// 모든 미니게임에서 Stir 버튼은 "Stir" 액션을 사용
			// (이제 FryingRhythmMinigame에서도 올바르게 "Stir"을 기대함)
			
			UE_LOG(LogTemp, Log, TEXT("OnStirButtonClicked - Rhythm minigame, sending action: %s"), *ActionToSend);
			HandleMinigameInput(ActionToSend);
		}
	}
	else
	{
		UE_LOG(LogTemp, Verbose, TEXT("요리가 진행중이 아닙니다. InMinigame: %s, Minigame valid: %s"), 
			bIsInMinigameMode ? TEXT("true") : TEXT("false"),
			CurrentMinigame.IsValid() ? TEXT("true") : TEXT("false"));
	}
}

void UCookingWidget::OnCheckButtonClicked()
{
	UE_LOG(LogTemp, Verbose, TEXT("OnCheckButtonClicked - bIsFryingGame: %s, bIsRhythmNoteActive: %s, CurrentRhythmAction: %s"), 
		bIsFryingGame ? TEXT("true") : TEXT("false"),
		bIsRhythmNoteActive ? TEXT("true") : TEXT("false"),
		*CurrentRhythmAction);

	if (bIsInMinigameMode && CurrentMinigame.IsValid())
	{
		// 모든 미니게임에서 온도확인은 "Check" 액션 사용
		UE_LOG(LogTemp, Log, TEXT("OnCheckButtonClicked - Sending action: Check"));
		HandleMinigameInput(TEXT("Check"));
	}
	else
	{
		UE_LOG(LogTemp, Verbose, TEXT("요리가 진행중이 아닙니다. InMinigame: %s, Minigame valid: %s"), 
			bIsInMinigameMode ? TEXT("true") : TEXT("false"),
			CurrentMinigame.IsValid() ? TEXT("true") : TEXT("false"));
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
	UE_LOG(LogTemp, Log, TEXT("UCookingWidget::OnMinigameStarted - Minigame: %s"), Minigame ? *Minigame->GetName() : TEXT("nullptr"));
	if (AssociatedInteractable)
	{
		UE_LOG(LogTemp, Log, TEXT("UCookingWidget::OnMinigameStarted - AssociatedInteractable: %s, Class: %s"), *AssociatedInteractable->GetName(), *AssociatedInteractable->GetClass()->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("UCookingWidget::OnMinigameStarted - AssociatedInteractable is nullptr"));
	}

	if (!Minigame)
	{
		UE_LOG(LogTemp, Warning, TEXT("UCookingWidget::OnMinigameStarted - Invalid minigame"));
		return;
	}

	CurrentMinigame = Minigame;
	bIsInMinigameMode = true;

	UE_LOG(LogTemp, Log, TEXT("UCookingWidget::OnMinigameStarted - Minigame started"));

	// 현재 연결된 오브젝트가 테이블인지 냄비인지 확인
	bool bIsTableInteraction = false;
	if (AssociatedInteractable)
	{
		// InteractableTable이지만 InteractablePot이 아닌 경우 = 순수 테이블 (재료 썰기)
		AInteractablePot* Pot = Cast<AInteractablePot>(AssociatedInteractable);
		if (!Pot)
		{
			bIsTableInteraction = true;
			UE_LOG(LogTemp, Log, TEXT("OnMinigameStarted - Table interaction detected (ingredient slicing)"));
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("OnMinigameStarted - Pot interaction detected (cooking)"));
		}
	}

	// 미니게임 종류 확인
	FString MinigameType = Minigame->GetClass()->GetName();
	bool bIsGrillingMinigame = MinigameType.Contains(TEXT("Grilling"));
	bool bIsRhythmMinigame = MinigameType.Contains(TEXT("Rhythm"));
	bool bIsFryingMinigame = MinigameType.Contains(TEXT("FryingRhythm"));
	bool bIsTimerBasedMinigame = MinigameType.Contains(TEXT("TimerBased"));

	// bIsFryingGame 멤버 변수 업데이트
	bIsFryingGame = bIsFryingMinigame;

	// 테이블 상호작용(재료 썰기)인 경우 리듬게임 UI 숨기기
	if (bIsTableInteraction)
	{
		UE_LOG(LogTemp, Log, TEXT("OnMinigameStarted - Hiding rhythm game UI for table interaction. RhythmGameOverlay Ptr: %s"), RhythmGameOverlay ? TEXT("Valid") : TEXT("Null"));
		if (RhythmGameOverlay)
		{
			RhythmGameOverlay->SetVisibility(ESlateVisibility::Collapsed);
			UE_LOG(LogTemp, Log, TEXT("OnMinigameStarted - RhythmGameOverlay visibility set to Collapsed for table interaction."));
		}
		return; // 나머지 UI 설정 건너뛰기
	}

	// UI 업데이트 - 미니게임 모드 활성화
	if (StatusText)
	{
		if (bIsGrillingMinigame)
		{
			StatusText->SetText(FText::FromString(TEXT("굽기 미니게임 시작! 뒤집기, 화력 조절, 확인 버튼을 사용하세요!")));
		}
		else if (bIsFryingMinigame)
		{
			StatusText->SetText(FText::FromString(TEXT("튀기기 리듬게임 시작! 타이밍에 맞춰 버튼을 누르세요!")));
		}
		else if (bIsRhythmMinigame)
		{
			StatusText->SetText(FText::FromString(TEXT("리듬 미니게임 시작! 'Stir' 버튼을 타이밍에 맞춰 누르세요!")));
		}
		else
		{
			StatusText->SetText(FText::FromString(TEXT("미니게임 시작!")));
		}
	}

	// ActionText 초기화
	if (ActionText)
	{
		ActionText->SetText(FText::FromString(TEXT("액션을 기다려주세요...")));
	}

	// 미니게임 종류에 따른 버튼 표시
	if (bIsGrillingMinigame)
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
			StirButton->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	else if (bIsFryingMinigame)
	{
		// 튀기기 리듬게임: 필요한 버튼들만 표시하고 키보드 가이드 제공
		if (StirButton)
		{
			StirButton->SetVisibility(ESlateVisibility::Visible);
			StirButton->SetIsEnabled(true); // 항상 활성화 (키보드 입력용)
			SetButtonText(StirButton, TEXT("흔들기 (Space)"));
		}
		if (CheckButton)
		{
			CheckButton->SetVisibility(ESlateVisibility::Visible);
			CheckButton->SetIsEnabled(true); // 항상 활성화 (키보드 입력용)
			SetButtonText(CheckButton, TEXT("온도확인 (V)"));
		}
		
		// 다른 버튼들 숨기기
		if (FlipButton) FlipButton->SetVisibility(ESlateVisibility::Collapsed);
		if (HeatUpButton) HeatUpButton->SetVisibility(ESlateVisibility::Collapsed);
		if (HeatDownButton) HeatDownButton->SetVisibility(ESlateVisibility::Collapsed);

		// 리듬게임 UI 초기화 (처음에는 숨김)
		if (RhythmGameOverlay)
		{
			RhythmGameOverlay->SetVisibility(ESlateVisibility::Collapsed);
		}

		// 튀기기 게임 전용 UI 요소들 초기화
		if (ComboText)
		{
			ComboText->SetText(FText::FromString(TEXT("콤보: 0")));
			ComboText->SetVisibility(ESlateVisibility::Visible);
		}
		if (TemperatureText)
		{
			TemperatureText->SetText(FText::FromString(TEXT("온도: 50.0% (적정)")));
			TemperatureText->SetVisibility(ESlateVisibility::Visible);
		}
		
		// 상태 텍스트에 조작 가이드 표시
		if (StatusText)
		{
			StatusText->SetText(FText::FromString(TEXT("리듬게임 시작! 원이 겹칠 때 해당 키를 누르세요!")));
		}
	}
	else if (bIsTimerBasedMinigame)
	{
		// 타이머 기반 미니게임: 단순한 UI 구성
		if (StirButton)
		{
			StirButton->SetVisibility(ESlateVisibility::Visible);
			StirButton->SetIsEnabled(false); // 이벤트 발생 시에만 활성화
			SetButtonText(StirButton, TEXT("행동 (Space)"));
		}
		
		// 다른 버튼들 숨기기
		if (FlipButton) FlipButton->SetVisibility(ESlateVisibility::Collapsed);
		if (HeatUpButton) HeatUpButton->SetVisibility(ESlateVisibility::Collapsed);
		if (HeatDownButton) HeatDownButton->SetVisibility(ESlateVisibility::Collapsed);
		if (CheckButton) CheckButton->SetVisibility(ESlateVisibility::Collapsed);

		// 기본 UI 요소들 숨기기
		if (ComboText) ComboText->SetVisibility(ESlateVisibility::Collapsed);
		if (TemperatureText) TemperatureText->SetVisibility(ESlateVisibility::Collapsed);
		
		// 리듬게임 UI는 타이머 미니게임에서 원형 이벤트용으로 재사용
		if (RhythmGameOverlay)
		{
			RhythmGameOverlay->SetVisibility(ESlateVisibility::Collapsed);
		}
		
		// 상태 텍스트에 조작 가이드 표시
		if (StatusText)
		{
			StatusText->SetText(FText::FromString(TEXT("타이머 요리 미니게임! 이벤트가 발생하면 정확한 타이밍에 Space를 누르세요!")));
		}
	}
	else
	{
		// 일반 요리 (썰기, 끓이기 등): 기본 UI만 표시
		if (StirButton)
		{
			StirButton->SetVisibility(ESlateVisibility::Visible);
			StirButton->SetIsEnabled(CurrentRequiredAction == TEXT("Stir"));
			SetButtonText(StirButton, TEXT("젓기"));
		}
		if (CheckButton)
		{
			CheckButton->SetVisibility(ESlateVisibility::Visible);
			CheckButton->SetIsEnabled(CurrentRequiredAction == TEXT("CheckTemperature"));
			SetButtonText(CheckButton, TEXT("온도확인"));
		}
		if (FlipButton)
		{
			FlipButton->SetVisibility(ESlateVisibility::Visible);
			FlipButton->SetIsEnabled(CurrentRequiredAction == TEXT("Flip"));
		}
		if (HeatUpButton) HeatUpButton->SetVisibility(ESlateVisibility::Visible);
		if (HeatDownButton) HeatDownButton->SetVisibility(ESlateVisibility::Visible);
		
		// 리듬게임 UI 숨기기 (일반 요리에서는 사용하지 않음)
		if (RhythmGameOverlay)
		{
			RhythmGameOverlay->SetVisibility(ESlateVisibility::Collapsed);
		}
		
		// 튀기기 전용 UI 숨기기
		if (ComboText) ComboText->SetVisibility(ESlateVisibility::Collapsed);
		if (TemperatureText && bIsFryingGame == false) TemperatureText->SetVisibility(ESlateVisibility::Collapsed);
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

	// 점수나 페이즈가 크게 변했을 때만 로그 출력 (스팸 방지)
	static float LastLoggedScore = -999.0f;
	static int32 LastLoggedPhase = -1;
	
	bool bShouldLog = (FMath::Abs(Score - LastLoggedScore) >= 10.0f) || (Phase != LastLoggedPhase);
	
	if (bShouldLog)
	{
		UE_LOG(LogTemp, Log, TEXT("UCookingWidget::OnMinigameUpdated - Score: %.2f, Phase: %d"), Score, Phase);
		LastLoggedScore = Score;
		LastLoggedPhase = Phase;
	}

	// StatusText는 점수와 전체 상태만 표시 (ActionText는 별도로 관리)
	if (StatusText)
	{
		FString StatusMessage;
		
		if (Score < 0)
		{
			StatusMessage = FString::Printf(TEXT("미니게임 | 점수: %.0f | 실패!"), Score);
		}
		else
		{
			StatusMessage = FString::Printf(TEXT("미니게임 | 점수: %.0f"), Score);
		}
		
		StatusText->SetText(FText::FromString(StatusMessage));
	}

	// 튀기기 리듬게임에서 콤보와 온도 정보 업데이트
	if (CurrentMinigame.IsValid())
	{
		FString MinigameType = CurrentMinigame->GetClass()->GetName();
		if (MinigameType.Contains(TEXT("FryingRhythm")))
		{
			// 콤보 정보 표시
			if (ComboText)
			{
				// UFryingRhythmMinigame에서 콤보 정보를 가져와야 함
				if (auto* FryingGame = Cast<UFryingRhythmMinigame>(CurrentMinigame.Get()))
				{
					int32 CurrentCombo = FryingGame->GetCurrentCombo();
					FString ComboMessage = FString::Printf(TEXT("콤보: %d"), CurrentCombo);
					ComboText->SetText(FText::FromString(ComboMessage));
				}
			}

			// 온도 정보 표시
			if (TemperatureText)
			{
				if (auto* FryingGame = Cast<UFryingRhythmMinigame>(CurrentMinigame.Get()))
				{
					float Temperature = FryingGame->GetCookingTemperature();
					bool bOptimal = FryingGame->IsTemperatureOptimal();
					FString TempMessage = FString::Printf(TEXT("온도: %.1f%% %s"), 
						Temperature * 100.0f, 
						bOptimal ? TEXT("(적정)") : TEXT("(주의!)"));
					TemperatureText->SetText(FText::FromString(TempMessage));
				}
			}
		}
	}

	// 버튼 상태는 UpdateRequiredAction에서만 관리하므로 여기서는 터치하지 않음
	// 매 프레임마다 이 로그가 나오는 것은 제거
}

void UCookingWidget::OnMinigameEnded(int32 Result)
{
	bIsInMinigameMode = false;
	CurrentMinigame = nullptr;
	
	// 타이머 기반 미니게임 상태 정리
	bIsTimerMinigameActive = false;
	bIsCircularEventActive = false;

	UE_LOG(LogTemp, Log, TEXT("UCookingWidget::OnMinigameEnded - Result: %d"), Result);

	// 결과에 따른 메시지 표시
	if (StatusText)
	{
		FString ResultMessage;
		switch (Result)
		{
		case 1: // Perfect
			ResultMessage = TEXT("완벽한 요리! 최고입니다!");
			break;
		case 2: // Good
			ResultMessage = TEXT("좋은 요리! 잘 하셨어요!");
			break;
		case 3: // Average
			ResultMessage = TEXT("평범한 요리 괜찮네요");
			break;
		case 4: // Poor
			ResultMessage = TEXT("아쉬운 요리... 다음엔 더 잘할 수 있어요");
			break;
		case 5: // Failed
			ResultMessage = TEXT("요리 실패! 다시 시도해보세요");
			break;
		default:
			ResultMessage = TEXT("요리 완료! 수거하세요");
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
		StirButton->SetVisibility(ESlateVisibility::Collapsed);
		StirButton->SetIsEnabled(false);
		// 버튼 텍스트를 기본값으로 복원
		SetButtonText(StirButton, TEXT("젓기"));
	}
	if (FlipButton)
	{
		FlipButton->SetVisibility(ESlateVisibility::Collapsed);
		FlipButton->SetIsEnabled(false);
	}
	if (HeatUpButton)
	{
		HeatUpButton->SetVisibility(ESlateVisibility::Collapsed);
		HeatUpButton->SetIsEnabled(false);
	}
	if (HeatDownButton)
	{
		HeatDownButton->SetVisibility(ESlateVisibility::Collapsed);
		HeatDownButton->SetIsEnabled(false);
	}
	if (CheckButton)
	{
		CheckButton->SetVisibility(ESlateVisibility::Collapsed);
		CheckButton->SetIsEnabled(false);
	}

	// 튀기기 게임 전용 UI 요소들 숨기기
	if (ComboText)
	{
		ComboText->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (TemperatureText)
	{
		TemperatureText->SetVisibility(ESlateVisibility::Collapsed);
	}

	// 리듬게임 UI 요소들 숨기기
	if (RhythmGameOverlay)
	{
		RhythmGameOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (RhythmOuterCircle)
	{
		RhythmOuterCircle->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (RhythmInnerCircle)
	{
		RhythmInnerCircle->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (RhythmActionText)
	{
		RhythmActionText->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (RhythmTimingText)
	{
		RhythmTimingText->SetVisibility(ESlateVisibility::Collapsed);
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

	// NEW: 오디오 매니저에게 피드백 요청
	if (AssociatedInteractable)
	{
		// AssociatedInteractable을 InteractablePot으로 캐스팅
		if (AInteractablePot* Pot = Cast<AInteractablePot>(AssociatedInteractable))
		{
			if (UCookingAudioManager* AudioManager = Pot->GetAudioManager())
			{
				AudioManager->PlayScoreBasedFeedback(ScoreDifference);
			}
		}
	}

	// 시각적 피드백 제공
	if (StatusText)
	{
		FString FeedbackMessage;
		if (ScoreDifference > 0)
		{
			// 성공적인 입력
			if (ScoreDifference >= 100.0f)
			{
				FeedbackMessage = FString::Printf(TEXT("PERFECT! +%.0f 점"), ScoreDifference);
			}
			else if (ScoreDifference >= 75.0f)
			{
				FeedbackMessage = FString::Printf(TEXT("GOOD! +%.0f 점"), ScoreDifference);
			}
			else
			{
				FeedbackMessage = FString::Printf(TEXT("HIT! +%.0f 점"), ScoreDifference);
			}
		}
		else if (ScoreDifference < 0)
		{
			// 실패한 입력 (점수 차감)
			FeedbackMessage = FString::Printf(TEXT("타이밍 실패! %.0f 점"), ScoreDifference);
		}
		else
		{
			// 점수 변화 없음 (이미 처리된 이벤트나 잘못된 타이밍)
			FeedbackMessage = TEXT("아직 타이밍이 아닙니다!");
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
		// NEW: 알림음 재생
		if (AssociatedInteractable)
		{
			if (AInteractablePot* Pot = Cast<AInteractablePot>(AssociatedInteractable))
			{
				if (UCookingAudioManager* AudioManager = Pot->GetAudioManager())
				{
					AudioManager->PlayNotificationSound();
				}
			}
		}

		// 필요한 액션에 따라 해당 버튼만 활성화
		if (ActionType == TEXT("Flip"))
		{
			// 굽기 게임에서만 FlipButton 사용
			if (CurrentMinigame.IsValid())
			{
				FString MinigameType = CurrentMinigame->GetClass()->GetName();
				if (MinigameType.Contains(TEXT("Grilling")))
				{
					if (FlipButton)
					{
						FlipButton->SetIsEnabled(true);
						UE_LOG(LogTemp, Log, TEXT("UCookingWidget::UpdateRequiredAction - FlipButton ENABLED"));
					}
					if (ActionText)
					{
						ActionText->SetText(FText::FromString(TEXT("지금 뒤집으세요!")));
					}
				}
				else if (MinigameType.Contains(TEXT("FryingRhythm")))
				{
					// 튀기기에서는 StirButton 사용
					if (StirButton)
					{
						StirButton->SetIsEnabled(true);
						UE_LOG(LogTemp, Log, TEXT("UCookingWidget::UpdateRequiredAction - StirButton ENABLED for Frying"));
					}
					if (ActionText)
					{
						// 튀기기에서는 고정된 메시지 사용 (랜덤 메시지로 인한 깜빡임 방지)
						ActionText->SetText(FText::FromString(TEXT("지금 흔들어주세요!")));
					}
				}
				else
				{
					// 일반 리듬게임에서는 StirButton 사용
					if (StirButton)
					{
						StirButton->SetIsEnabled(true);
						UE_LOG(LogTemp, Log, TEXT("UCookingWidget::UpdateRequiredAction - StirButton ENABLED for Rhythm"));
					}
					if (ActionText)
					{
						ActionText->SetText(FText::FromString(TEXT("지금 젓으세요!")));
					}
				}
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
				ActionText->SetText(FText::FromString(TEXT("화력을 높이세요!")));
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
				ActionText->SetText(FText::FromString(TEXT("화력을 낮추세요!")));
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
				// 현재 미니게임 타입에 따라 다른 메시지 표시
				if (bIsInMinigameMode && CurrentMinigame.IsValid())
				{
					FString MinigameType = CurrentMinigame->GetClass()->GetName();
					if (MinigameType.Contains(TEXT("FryingRhythm")))
					{
						ActionText->SetText(FText::FromString(TEXT("온도를 확인하세요!")));
					}
					else
					{
						ActionText->SetText(FText::FromString(TEXT("익힘 정도를 확인하세요!")));
					}
				}
				else
				{
					ActionText->SetText(FText::FromString(TEXT("익힘 정도를 확인하세요!")));
				}
			}
		}
	}
	else
	{
		// 액션이 필요하지 않을 때
		if (ActionText)
		{
			ActionText->SetText(FText::FromString(TEXT("다음 액션을 기다려주세요...")));
		}
	}
}

void UCookingWidget::SetButtonText(UButton* Button, const FString& Text)
{
	if (Button && Button->GetChildrenCount() > 0)
	{
		// UButton의 첫 번째 자식 위젯이 보통 TextBlock입니다
		if (UTextBlock* TextBlock = Cast<UTextBlock>(Button->GetChildAt(0)))
		{
			TextBlock->SetText(FText::FromString(Text));
			UE_LOG(LogTemp, Log, TEXT("SetButtonText: Changed button text to %s"), *Text);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("SetButtonText: Button's first child is not a TextBlock"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("SetButtonText: Button is null or has no children"));
	}
}

void UCookingWidget::StartRhythmGameNote(const FString& ActionType, float NoteDuration)
{
	UE_LOG(LogTemp, Log, TEXT("UCookingWidget::StartRhythmGameNote - CALLED. Action: %s, Duration: %.2f. RhythmGameOverlay Ptr: %s, InnerCircle Ptr: %s, InitialCircleScale: %.2f"), 
		*ActionType, NoteDuration, RhythmGameOverlay ? TEXT("Valid") : TEXT("Null"), RhythmInnerCircle ? TEXT("Valid") : TEXT("Null"), InitialCircleScale);

	// Clear any existing hide UI timer to prevent conflicts
	if (HideUITimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(HideUITimerHandle);
		UE_LOG(LogTemp, Log, TEXT("UCookingWidget::StartRhythmGameNote - Cleared existing hide UI timer"));
	}

	if (!RhythmGameOverlay || !RhythmOuterCircle || !RhythmInnerCircle)
	{
		UE_LOG(LogTemp, Warning, TEXT("UCookingWidget::StartRhythmGameNote - Missing rhythm game UI elements"));
		return;
	}

	bIsRhythmNoteActive = true;
	RhythmNoteStartTime = GetWorld()->GetTimeSeconds();
	RhythmNoteDuration = NoteDuration;
	CurrentRhythmAction = ActionType;

	UE_LOG(LogTemp, Log, TEXT("StartRhythmGameNote - ActionType: %s, bIsRhythmNoteActive: %s"), 
		*ActionType, bIsRhythmNoteActive ? TEXT("true") : TEXT("false"));

	// 리듬게임 UI 표시
	RhythmGameOverlay->SetVisibility(ESlateVisibility::Visible);
	
	// 외부 원 (고정)
	RhythmOuterCircle->SetVisibility(ESlateVisibility::Visible);
	RhythmOuterCircle->SetRenderScale(FVector2D(1.0f, 1.0f));
	
	// 내부 원 (수축할 원) - 초기 크기를 크게 설정
	RhythmInnerCircle->SetVisibility(ESlateVisibility::Visible);
	RhythmInnerCircle->SetRenderScale(FVector2D(InitialCircleScale, InitialCircleScale));
	
	// 액션 텍스트 표시
	if (RhythmActionText)
	{
		FString ActionMessage;
		if (ActionType == TEXT("Stir"))
		{
			ActionMessage = TEXT("흔들기 (Space)");
		}
		else if (ActionType == TEXT("Check"))
		{
			ActionMessage = TEXT("온도 확인 (V)");
		}
		else
		{
			ActionMessage = ActionType;
		}
		RhythmActionText->SetText(FText::FromString(ActionMessage));
		RhythmActionText->SetVisibility(ESlateVisibility::Visible);
	}

	// 타이밍 텍스트 초기화
	if (RhythmTimingText)
	{
		RhythmTimingText->SetText(FText::FromString(TEXT("준비...")));
		RhythmTimingText->SetVisibility(ESlateVisibility::Visible);
	}

	UE_LOG(LogTemp, Log, TEXT("UCookingWidget::StartRhythmGameNote - Started %s note for %.2f seconds"), *ActionType, NoteDuration);
}

void UCookingWidget::UpdateRhythmGameTiming(float Progress)
{
	if (!bIsRhythmNoteActive || !RhythmInnerCircle)
	{
		return;
	}

	// Progress: 0.0 (시작) ~ 1.0 (완료)
	float CurrentScale = InitialCircleScale * (1.0f - Progress);
	RhythmInnerCircle->SetRenderScale(FVector2D(CurrentScale, CurrentScale));

	UE_LOG(LogTemp, Verbose, TEXT("UCookingWidget::UpdateRhythmGameTiming - Progress: %.3f, CurrentScale: %.3f, InitialScale: %.2f"), Progress, CurrentScale, InitialCircleScale);

	// 타이밍 텍스트 업데이트
	if (RhythmTimingText)
	{
		if (Progress < 0.7f)
		{
			RhythmTimingText->SetText(FText::FromString(TEXT("대기...")));
		}
		else if (Progress < 0.9f)
		{
			RhythmTimingText->SetText(FText::FromString(TEXT("준비!")));
		}
		else
		{
			RhythmTimingText->SetText(FText::FromString(TEXT("지금!")));
		}
	}
}

void UCookingWidget::EndRhythmGameNote()
{
	bIsRhythmNoteActive = false;
	CurrentRhythmAction = TEXT("");

	// 리듬게임 UI 숨기기 (약간의 딜레이 후) - 멤버 변수 타이머 핸들 사용
	GetWorld()->GetTimerManager().SetTimer(HideUITimerHandle, [this]()
	{
		if (RhythmGameOverlay)
		{
			RhythmGameOverlay->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (RhythmOuterCircle)
		{
			RhythmOuterCircle->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (RhythmInnerCircle)
		{
			RhythmInnerCircle->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (RhythmActionText)
		{
			RhythmActionText->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (RhythmTimingText)
		{
			RhythmTimingText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}, 1.0f, false);

	UE_LOG(LogTemp, Log, TEXT("UCookingWidget::EndRhythmGameNote - Note ended"));
}

void UCookingWidget::ShowRhythmGameResult(const FString& Result)
{
	if (RhythmTimingText)
	{
		FString ResultText;
		if (Result == TEXT("Perfect"))
		{
			ResultText = TEXT("PERFECT!");
		}
		else if (Result == TEXT("Good"))
		{
			ResultText = TEXT("GOOD!");
		}
		else if (Result == TEXT("Hit"))
		{
			ResultText = TEXT("HIT!");
		}
		else if (Result == TEXT("Miss"))
		{
			ResultText = TEXT("MISS...");
		}
		else
		{
			ResultText = Result;
		}
		
		RhythmTimingText->SetText(FText::FromString(ResultText));
		
		// 결과에 따른 색상 변경 등 추가 효과 가능
		UE_LOG(LogTemp, Log, TEXT("UCookingWidget::ShowRhythmGameResult - Showing %s"), *ResultText);
	}
}

// ========================================
// NEW: Timer-based Minigame UI Functions
// ========================================

void UCookingWidget::StartTimerBasedMinigame(float TotalTime)
{
	bIsTimerMinigameActive = true;
	TimerMinigameTotalTime = TotalTime;
	TimerMinigameRemainingTime = TotalTime;
	bIsCircularEventActive = false;

	UE_LOG(LogTemp, Log, TEXT("UCookingWidget::StartTimerBasedMinigame - Started with %.1f seconds"), TotalTime);

	// 기존 리듬 게임 UI 숨기기
	if (RhythmGameOverlay)
	{
		RhythmGameOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}

	// 상태 텍스트 업데이트
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(TEXT("타이머 요리 미니게임 시작!")));
	}

	// 액션 텍스트 초기화
	if (ActionText)
	{
		ActionText->SetText(FText::FromString(TEXT("이벤트를 기다려주세요...")));
	}

	// 버튼들을 기본 상태로 설정
	if (StirButton)
	{
		StirButton->SetVisibility(ESlateVisibility::Visible);
		StirButton->SetIsEnabled(false);
		SetButtonText(StirButton, TEXT("행동 (Space)"));
	}
}

void UCookingWidget::UpdateMainTimer(float Progress, float RemainingTime)
{
	if (!bIsTimerMinigameActive)
	{
		return;
	}

	TimerMinigameRemainingTime = RemainingTime;

	// 진행률을 UI에 표시 (프로그레스 바나 텍스트로)
	if (StatusText && !bIsCircularEventActive)
	{
		FString TimeText = FString::Printf(TEXT("남은 시간: %.1f초"), RemainingTime);
		StatusText->SetText(FText::FromString(TimeText));
	}

	UE_LOG(LogTemp, Verbose, TEXT("UCookingWidget::UpdateMainTimer - Progress: %.2f, Remaining: %.1f"), Progress, RemainingTime);
}

void UCookingWidget::StartCircularEvent(float StartAngle, float EndAngle, float ArrowSpeed)
{
	if (!bIsTimerMinigameActive)
	{
		return;
	}

	// 이전 이벤트의 UI 숨김 타이머가 있다면 취소
	if (HideUITimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(HideUITimerHandle);
		UE_LOG(LogTemp, Log, TEXT("UCookingWidget::StartCircularEvent - Cleared previous hide timer"));
	}

	bIsCircularEventActive = true;
	CircularEventStartAngle = StartAngle;
	CircularEventEndAngle = EndAngle;
	CircularEventArrowAngle = 0.0f; // 12시에서 시작
	CircularEventArrowSpeed = ArrowSpeed;

	UE_LOG(LogTemp, Log, TEXT("UCookingWidget::StartCircularEvent - Arc: %.1f-%.1f degrees, Speed: %.1f"), 
		   StartAngle, EndAngle, ArrowSpeed);

	// 원형 이벤트 UI 표시
	if (RhythmGameOverlay)
	{
		RhythmGameOverlay->SetVisibility(ESlateVisibility::Visible);
	}

	// 외부 원 (검은색 베이스) - 크기를 더 크게 설정
	if (RhythmOuterCircle)
	{
		RhythmOuterCircle->SetVisibility(ESlateVisibility::Visible);
		RhythmOuterCircle->SetRenderScale(FVector2D(1.2f, 1.2f)); // 크기 증가
		// 회전 초기화
		FWidgetTransform ResetTransform;
		ResetTransform.Scale = FVector2D(1.2f, 1.2f);
		ResetTransform.Angle = 0.0f;
		RhythmOuterCircle->SetRenderTransform(ResetTransform);
	}

	// 내부 원을 성공 구간으로 사용 (회색 호)
	if (RhythmInnerCircle)
	{
		RhythmInnerCircle->SetVisibility(ESlateVisibility::Visible);
		// 외부 원과 같은 크기로 설정
		RhythmInnerCircle->SetRenderScale(FVector2D(1.2f, 1.2f));
		
		// 성공 구간의 회전 설정 (StartAngle 기준으로 회전)
		// UI 회전 오프셋을 적용 (기본값: -90도, 블루프린트에서 조정 가능)
		float UIRotationOffset = -90.0f; // 기본값, 나중에 미니게임에서 가져올 수 있음
		float RotationDegrees = StartAngle + UIRotationOffset;
		FWidgetTransform SuccessZoneTransform;
		SuccessZoneTransform.Scale = FVector2D(1.2f, 1.2f);
		SuccessZoneTransform.Angle = RotationDegrees;
		RhythmInnerCircle->SetRenderTransform(SuccessZoneTransform);
		
		UE_LOG(LogTemp, Log, TEXT("UCookingWidget::StartCircularEvent - Success zone rotated to %.1f degrees"), StartAngle);
	}

	// 화살표 이미지 설정 (빨간 화살표)
	if (RhythmArrowImage)
	{
		RhythmArrowImage->SetVisibility(ESlateVisibility::Visible);
		RhythmArrowImage->SetRenderScale(FVector2D(1.0f, 1.0f));
		// 초기 위치 (12시 방향)
		FWidgetTransform ArrowTransform;
		ArrowTransform.Scale = FVector2D(1.0f, 1.0f);
		ArrowTransform.Angle = 0.0f;
		RhythmArrowImage->SetRenderTransform(ArrowTransform);
	}

	// 액션 버튼 활성화
	if (StirButton)
	{
		StirButton->SetIsEnabled(true);
	}

	// 액션 텍스트 업데이트
	if (ActionText)
	{
		ActionText->SetText(FText::FromString(TEXT("화살표가 색칠된 구간에 올 때 Space를 누르세요!")));
	}

	// 화살표 각도 표시를 위한 텍스트 (디버그용)
	if (RhythmTimingText)
	{
		FString DebugText = FString::Printf(TEXT("성공 구간: %.0f°-%.0f°"), StartAngle, EndAngle);
		RhythmTimingText->SetText(FText::FromString(DebugText));
		RhythmTimingText->SetVisibility(ESlateVisibility::Visible);
	}
}

void UCookingWidget::UpdateCircularEvent(float ArrowAngle)
{
	if (!bIsCircularEventActive)
	{
		return;
	}

	CircularEventArrowAngle = ArrowAngle;

	// 화살표 이미지 회전 업데이트
	if (RhythmArrowImage)
	{
		// 12시 방향 기준으로 보정 (FWidgetTransform은 각도를 도 단위로 사용)
		float UIRotationOffset = -90.0f; // 기본값, 나중에 미니게임에서 가져올 수 있음
		float RotationDegrees = ArrowAngle + UIRotationOffset;
		
		FWidgetTransform ArrowTransform;
		ArrowTransform.Scale = FVector2D(1.0f, 1.0f);
		ArrowTransform.Angle = RotationDegrees;
		RhythmArrowImage->SetRenderTransform(ArrowTransform);
	}

	// 디버그 텍스트 업데이트
	if (RhythmTimingText)
	{
		FString DebugText = FString::Printf(TEXT("화살표: %.0f° | 성공구간: %.0f°-%.0f°"), 
			ArrowAngle, CircularEventStartAngle, CircularEventEndAngle);
		RhythmTimingText->SetText(FText::FromString(DebugText));
	}

	// 성공 구간 내에 있는지 시각적 피드백 (선택사항)
	bool bInSuccessZone = (ArrowAngle >= CircularEventStartAngle && ArrowAngle <= CircularEventEndAngle);
	if (bInSuccessZone && ActionText)
	{
		ActionText->SetText(FText::FromString(TEXT("지금 누르세요! (Space)")));
	}
	else if (ActionText)
	{
		ActionText->SetText(FText::FromString(TEXT("화살표가 색칠된 구간에 올 때 Space를 누르세요!")));
	}

	UE_LOG(LogTemp, Verbose, TEXT("UCookingWidget::UpdateCircularEvent - Arrow at %.1f degrees, InZone: %s"), 
		ArrowAngle, bInSuccessZone ? TEXT("YES") : TEXT("NO"));
}

void UCookingWidget::EndCircularEvent(const FString& Result)
{
	if (!bIsCircularEventActive)
	{
		return;
	}

	bIsCircularEventActive = false;

	UE_LOG(LogTemp, Log, TEXT("UCookingWidget::EndCircularEvent - Result: %s"), *Result);

	// 결과 표시
	if (RhythmTimingText)
	{
		FString ResultText;
		if (Result == TEXT("Success"))
		{
			ResultText = TEXT("성공!");
		}
		else if (Result == TEXT("Failed"))
		{
			ResultText = TEXT("실패...");
		}
		else if (Result == TEXT("Timeout"))
		{
			ResultText = TEXT("시간 초과!");
		}
		else
		{
			ResultText = Result;
		}
		
		RhythmTimingText->SetText(FText::FromString(ResultText));
	}

	// 액션 버튼 비활성화
	if (StirButton)
	{
		StirButton->SetIsEnabled(false);
	}

	// 설정 가능한 지연 시간 후 원형 이벤트 UI 숨기기
	GetWorld()->GetTimerManager().SetTimer(HideUITimerHandle, [this]()
	{
		HideCircularEvent();
	}, CircularEventHideDelay, false);
	
	UE_LOG(LogTemp, Log, TEXT("UCookingWidget::EndCircularEvent - Hide timer set for %.1f seconds"), CircularEventHideDelay);
}

void UCookingWidget::HideCircularEvent()
{
	// 타이머 정리
	if (HideUITimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(HideUITimerHandle);
	}

	if (RhythmGameOverlay)
	{
		RhythmGameOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (RhythmOuterCircle)
	{
		RhythmOuterCircle->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (RhythmInnerCircle)
	{
		RhythmInnerCircle->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (RhythmArrowImage)
	{
		RhythmArrowImage->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (RhythmTimingText)
	{
		RhythmTimingText->SetVisibility(ESlateVisibility::Collapsed);
	}

	// 액션 텍스트를 기본 상태로 복원
	if (ActionText && bIsTimerMinigameActive)
	{
		ActionText->SetText(FText::FromString(TEXT("다음 이벤트를 기다려주세요...")));
	}

	UE_LOG(LogTemp, Log, TEXT("UCookingWidget::HideCircularEvent - Circular event UI hidden"));
}

FReply UCookingWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	FKey PressedKey = InKeyEvent.GetKey();

	if (PressedKey == EKeys::SpaceBar)
	{
		// Space 키 = 흔들기/젓기
		OnStirButtonClicked();
		return FReply::Handled();
	}
	else if (PressedKey == EKeys::V)
	{
		// V 키 = 온도확인
		OnCheckButtonClicked();
		return FReply::Handled();
	}

	// 일반 요리에서만 사용되는 키들
	if (!bIsFryingGame)
	{
		if (PressedKey == EKeys::F)
		{
			// F 키 = 뒤집기
			OnFlipButtonClicked();
			return FReply::Handled();
		}
		else if (PressedKey == EKeys::Q)
		{
			// Q 키 = 불 올리기
			OnHeatUpButtonClicked();
			return FReply::Handled();
		}
		else if (PressedKey == EKeys::E)
		{
			// E 키 = 불 내리기
			OnHeatDownButtonClicked();
			return FReply::Handled();
		}
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UCookingWidget::HandleRhythmGameInput()
{
	UE_LOG(LogTemp, Verbose, TEXT("HandleRhythmGameInput - CurrentMinigame valid: %s, CurrentRhythmAction: %s"), 
		CurrentMinigame.IsValid() ? TEXT("true") : TEXT("false"), *CurrentRhythmAction);

	// 현재 미니게임이 있는지 확인
	if (CurrentMinigame.IsValid())
	{
		// 현재 필요한 액션을 미니게임에 전달
		UE_LOG(LogTemp, Log, TEXT("HandleRhythmGameInput - Sending input to minigame: %s"), *CurrentRhythmAction);
		CurrentMinigame->HandlePlayerInput(CurrentRhythmAction);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("HandleRhythmGameInput - No active minigame!"));
	}
} 