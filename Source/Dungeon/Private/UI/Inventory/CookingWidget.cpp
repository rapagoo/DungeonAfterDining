// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Inventory/CookingWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/BoxComponent.h"
#include "Inventory/InventoryItemActor.h"
#include "Inventory/SlotStruct.h"
#include "Inventory/CookingRecipeStruct.h"
#include "Engine/DataTable.h"
#include "Characters/WarriorHeroCharacter.h"
#include "Inventory/InventoryComponent.h"
#include "Blueprint/UserWidget.h"
#include "Interactables/InteractableTable.h"
#include "InteractablePot.h"
#include "Kismet/GameplayStatics.h"
#include "Inventory/InvenItemStruct.h"
#include "Cooking/TimerMinigame.h"
#include "Cooking/CookingMinigameBase.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UI/Inventory/IngredientSlotWidget.h"

void UCookingWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Bind button click events
	if (AddIngredientButton)
	{
		AddIngredientButton->OnClicked.AddDynamic(this, &UCookingWidget::OnAddIngredientClicked);
	}
	if (CookButton)
	{
		CookButton->OnClicked.AddDynamic(this, &UCookingWidget::OnCookClicked);
	}
	if (CollectButton)
	{
		CollectButton->OnClicked.AddDynamic(this, &UCookingWidget::OnCollectButtonClicked);
	}
	if (StartButton)
	{
		StartButton->OnClicked.AddDynamic(this, &UCookingWidget::OnStartButtonClicked);
	}
	if (EventActionButton)
	{
		EventActionButton->OnClicked.AddDynamic(this, &UCookingWidget::OnEventActionButtonClicked);
	}

	// Bind minigame button events
	if (StirButton)
	{
		StirButton->OnClicked.AddDynamic(this, &UCookingWidget::OnStirButtonClicked);
	}
	if (FlipButton)
	{
		FlipButton->OnClicked.AddDynamic(this, &UCookingWidget::OnFlipButtonClicked);
	}
	if (HeatUpButton)
	{
		HeatUpButton->OnClicked.AddDynamic(this, &UCookingWidget::OnHeatUpButtonClicked);
	}
	if (HeatDownButton)
	{
		HeatDownButton->OnClicked.AddDynamic(this, &UCookingWidget::OnHeatDownButtonClicked);
	}
	if (CheckButton)
	{
		CheckButton->OnClicked.AddDynamic(this, &UCookingWidget::OnCheckButtonClicked);
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

	// Hide all minigame buttons initially
	StirButton->SetVisibility(ESlateVisibility::Collapsed);
	FlipButton->SetVisibility(ESlateVisibility::Collapsed);
	HeatUpButton->SetVisibility(ESlateVisibility::Collapsed);
	HeatDownButton->SetVisibility(ESlateVisibility::Collapsed);
	CheckButton->SetVisibility(ESlateVisibility::Collapsed);
	ActionText->SetVisibility(ESlateVisibility::Collapsed);

	// Hide minigame UI by default
	if (TimerEventOverlay)
	{
		TimerEventOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (OverallProgressBar)
	{
		OverallProgressBar->SetVisibility(ESlateVisibility::Collapsed);
	}

	// Create a dynamic material for the success zone if we have a base material
	if (EventSuccessZone && !SuccessZoneMID)
	{
		UMaterialInterface* Mat = EventSuccessZone->GetBrush().GetResourceObject();
		if (Mat)
		{
			SuccessZoneMID = UMaterialInstanceDynamic::Create(Mat, this);
			EventSuccessZone->SetBrushFromMaterial(SuccessZoneMID);
		}
	}
}

void UCookingWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// If the timer minigame is active, update relevant UI elements
	if (CurrentTimerMinigame.IsValid())
	{
		// Update Overall Progress Bar
		if (OverallProgressBar)
		{
			OverallProgressBar->SetPercent(CurrentTimerMinigame->GetCookProgress());
		}

		// Update Event Arrow Rotation if an event is active
		if (TimerEventOverlay && TimerEventOverlay->IsVisible())
		{
			if(EventArrow)
			{
				EventArrow->SetRenderTransformAngle(CurrentTimerMinigame->GetCurrentArrowAngle());
			}
		}
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

void UCookingWidget::OnStirButtonClicked()
{
	if (bIsInMinigameMode && CurrentMinigame.IsValid())
	{
		HandleMinigameInput(TEXT("Stir"));
	}
}

void UCookingWidget::OnCheckButtonClicked()
{
	if (bIsInMinigameMode && CurrentMinigame.IsValid())
	{
		HandleMinigameInput(TEXT("Check"));
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
	if (!Minigame) return;

	SetUIVisibility(false);
	CurrentMinigame = Minigame;

	// Try to cast to TimerMinigame and bind delegates
	UTimerMinigame* TimerMinigame = Cast<UTimerMinigame>(Minigame);
	if (TimerMinigame)
	{
		CurrentTimerMinigame = TimerMinigame;

		TimerMinigame->OnTimerEventSpawned.AddDynamic(this, &UCookingWidget::HandleTimerEventSpawned);
		TimerMinigame->OnTimerEventCompleted.AddDynamic(this, &UCookingWidget::HandleTimerEventCompleted);

		if (OverallProgressBar)
		{
			OverallProgressBar->SetVisibility(ESlateVisibility::Visible);
		}
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
}

void UCookingWidget::OnMinigameEnded(bool bSuccess, const TArray<FItemData>& Results)
{
	bIsInMinigameMode = false;
	CurrentMinigame = nullptr;

	UE_LOG(LogTemp, Log, TEXT("UCookingWidget::OnMinigameEnded - Result: %d"), bSuccess ? 1 : 0);

	// 결과에 따른 메시지 표시
	if (StatusText)
	{
		FString ResultMessage;
		if (bSuccess)
		{
			ResultMessage = TEXT("완벽한 요리! 최고입니다!");
		}
		else
		{
			ResultMessage = TEXT("요리 실패! 다시 시도해보세요");
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

	// Unbind from the timer minigame if it was active
	if (CurrentTimerMinigame.IsValid())
	{
		CurrentTimerMinigame->OnTimerEventSpawned.RemoveDynamic(this, &UCookingWidget::HandleTimerEventSpawned);
		CurrentTimerMinigame->OnTimerEventCompleted.RemoveDynamic(this, &UCookingWidget::HandleTimerEventCompleted);
		CurrentTimerMinigame.Reset();
	}

	// Hide minigame-specific UI
	if (TimerEventOverlay)
	{
		TimerEventOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (OverallProgressBar)
	{
		OverallProgressBar->SetVisibility(ESlateVisibility::Collapsed);
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
				ActionText->SetText(FText::FromString(TEXT("익힘 정도를 확인하세요!")));
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

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UCookingWidget::OnStartButtonClicked()
{
	// Hide the start button after clicking
	StartButton->SetVisibility(ESlateVisibility::Collapsed);

	OnStartCooking.Broadcast();
}

void UCookingWidget::OnEventActionButtonClicked()
{
	if (CurrentTimerMinigame.IsValid() && TimerEventOverlay && TimerEventOverlay->IsVisible())
	{
		CurrentTimerMinigame->HandlePlayerInput();
	}
}

void UCookingWidget::HandleTimerEventSpawned(const FTimerEvent& EventData)
{
	if (TimerEventOverlay)
	{
		TimerEventOverlay->SetVisibility(ESlateVisibility::Visible);
	}
	
	if (SuccessZoneMID)
	{
		SuccessZoneMID->SetScalarParameterValue("SuccessStartAngle", EventData.SuccessZoneStartAngle);
		SuccessZoneMID->SetScalarParameterValue("SuccessEndAngle", EventData.SuccessZoneEndAngle);
	}
}

void UCookingWidget::HandleTimerEventCompleted(bool bSuccess)
{
	if (TimerEventOverlay)
	{
		TimerEventOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}
	// Optionally: Add feedback for success/failure here
}

void UCookingWidget::SetUIVisibility(bool bVisible)
{
	if (TimerEventOverlay)
	{
		TimerEventOverlay->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (OverallProgressBar)
	{
		OverallProgressBar->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
} 