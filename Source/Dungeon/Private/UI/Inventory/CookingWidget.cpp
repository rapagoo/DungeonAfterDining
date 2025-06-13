// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/Inventory/CookingWidget.h"
#include "Components/Button.h"
#include "Components/VerticalBox.h"
#include "Components/TextBlock.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetSwitcher.h"
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
#include "Cooking/CookingMinigameBase.h"
#include "UI/Inventory/MinigameWidgetInterface.h"

void UCookingWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Bind functions for common buttons
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

	if (IngredientsList)
	{
		IngredientsList->ClearChildren();
	}

	if (MinigameSwitcher)
	{
		MinigameSwitcher->SetActiveWidgetIndex(-1);
	}
}

void UCookingWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	// Host widget no longer ticks for minigames.
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
		AInventoryItemActor* IngredientActor = NearbyIngredient.Get();

		AWarriorHeroCharacter* PlayerCharacter = Cast<AWarriorHeroCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
		if (!PlayerCharacter) { UE_LOG(LogTemp, Error, TEXT("OnAddIngredientClicked: No Player Character.")); return; }
		UInventoryComponent* PlayerInventory = PlayerCharacter->GetInventoryComponent();
		if (!PlayerInventory) { UE_LOG(LogTemp, Error, TEXT("OnAddIngredientClicked: No Player Inventory.")); return; }

		FSlotStruct OriginalSlotData = IngredientActor->GetItemData();
		FDataTableRowHandle OriginalItemIDHandle = OriginalSlotData.ItemID;

		if (!OriginalItemIDHandle.DataTable || OriginalItemIDHandle.RowName.IsNone())
		{
			UE_LOG(LogTemp, Error, TEXT("OnAddIngredientClicked: Nearby item actor '%s' has invalid ItemID handle."), *IngredientActor->GetName());
            NearbyIngredient = nullptr;
            if(AddIngredientButton) AddIngredientButton->SetIsEnabled(false);
			return;
		}

		FInventoryItemStruct* ItemDefinition = OriginalItemIDHandle.DataTable->FindRow<FInventoryItemStruct>(OriginalItemIDHandle.RowName, TEXT("OnAddIngredientClicked Context"));

		if (!ItemDefinition)
		{
			UE_LOG(LogTemp, Error, TEXT("OnAddIngredientClicked: Could not find Item Definition for '%s'."), *OriginalItemIDHandle.RowName.ToString());
            NearbyIngredient = nullptr;
            if(AddIngredientButton) AddIngredientButton->SetIsEnabled(false);
			return;
		}

        FName ItemIDToAdd = NAME_None;
        if (ItemDefinition->SlicedItemID != NAME_None)
        {
            ItemIDToAdd = ItemDefinition->SlicedItemID;
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("OnAddIngredientClicked: Original item '%s' does not have a valid SlicedItemID."), *OriginalItemIDHandle.RowName.ToString());
            NearbyIngredient = nullptr;
            if(AddIngredientButton) AddIngredientButton->SetIsEnabled(false);
            return;
        }

        FSlotStruct ItemDataToAdd;
        ItemDataToAdd.ItemID.RowName = ItemIDToAdd;
        ItemDataToAdd.Quantity = 1;
        ItemDataToAdd.ItemID.DataTable = OriginalItemIDHandle.DataTable;

		if (PlayerInventory->AddItem(ItemDataToAdd))
		{
			UE_LOG(LogTemp, Log, TEXT("Successfully added SLICED item '%s' to inventory."), *ItemIDToAdd.ToString());
			IngredientActor->Destroy();
			NearbyIngredient = nullptr;
			if (AddIngredientButton)
			{
				AddIngredientButton->SetIsEnabled(false);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to add SLICED item '%s' to player inventory."), *ItemIDToAdd.ToString());
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Add Ingredient Clicked, but NearbyIngredient is not valid or not sliced."));
        if (AddIngredientButton)
		{
			AddIngredientButton->SetIsEnabled(false);
		}
	}
}

void UCookingWidget::OnCookClicked()
{
	if (AssociatedInteractable)
	{
		if (AInteractablePot* Pot = Cast<AInteractablePot>(AssociatedInteractable))
		{
			Pot->StartCooking();
		}
	}
}

void UCookingWidget::OnCollectButtonClicked()
{
    if (AssociatedInteractable)
    {
        if (AInteractablePot* Pot = Cast<AInteractablePot>(AssociatedInteractable))
        {
            Pot->CollectCookedItem();
        }
    }
}

void UCookingWidget::UpdateWidgetState(const TArray<FName>& IngredientIDs, bool bIsPotCooking, bool bIsPotCookingComplete, bool bIsPotBurnt, FName CookedResultID)
{
    UE_LOG(LogTemp, Log, TEXT("UCookingWidget::UpdateWidgetState (C++) - Cooking: %d, Complete: %d, Burnt: %d."), bIsPotCooking, bIsPotCookingComplete, bIsPotBurnt);

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
                if (CurrentItemTable) 
                {
                    FInventoryItemStruct* ItemData = CurrentItemTable->FindRow<FInventoryItemStruct>(IngredientID, TEXT("UpdateWidgetState Context"));
                    if (ItemData && !ItemData->Name.IsEmpty()) 
                    {
                        DisplayName = ItemData->Name.ToString();
                    }
                }
                NewIngredientText->SetText(FText::FromString(DisplayName));
                IngredientsList->AddChildToVerticalBox(NewIngredientText); 
            }
        }
    }

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
            if (CurrentItemTableForResult)
            {
                FInventoryItemStruct* ItemData = CurrentItemTableForResult->FindRow<FInventoryItemStruct>(CookedResultID, TEXT("UpdateWidgetState CookedResult Context"));
                if (ItemData && !ItemData->Name.IsEmpty())
                {
                    ItemName = ItemData->Name.ToString();
                }
            }
            StatusText->SetText(FText::Format(FText::FromString(TEXT("{0} 완성! 획득하세요.")), FText::FromString(ItemName)));
        }
        else if (bIsPotCooking)
        {
            StatusText->SetText(FText::FromString(TEXT("요리 중...")));
        }
        else
        {
            StatusText->SetText(IngredientIDs.Num() > 0 ? FText::FromString(TEXT("요리하시겠습니까?")) : FText::FromString(TEXT("재료를 넣어주세요.")));
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
    
    if (AddIngredientButton)
    {
        AInteractablePot* Pot = Cast<AInteractablePot>(AssociatedInteractable);
        AddIngredientButton->SetVisibility(Pot ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
        if (!Pot)
        {
            AddIngredientButton->SetIsEnabled(NearbyIngredient.IsValid());
        }
    }
}

void UCookingWidget::SetAssociatedTable(AInteractableTable* Table)
{
	AssociatedInteractable = Table;
	if (AInteractablePot* Pot = Cast<AInteractablePot>(AssociatedInteractable))
	{
        Pot->NotifyWidgetUpdate();
	}
	else
	{
        UpdateWidgetState({}, false, false, false, NAME_None);
	}
	UpdateNearbyIngredient(FindNearbySlicedIngredient());
}

AInventoryItemActor* UCookingWidget::FindNearbySlicedIngredient()
{
	if (!AssociatedInteractable) return nullptr;

	UBoxComponent* DetectionArea = AssociatedInteractable->FindComponentByClass<UBoxComponent>();
	if(!DetectionArea)
	{
		DetectionArea = Cast<UBoxComponent>(AssociatedInteractable->GetDefaultSubobjectByName(FName("InteractionVolume")));
        if(!DetectionArea)
        {
            DetectionArea = Cast<UBoxComponent>(AssociatedInteractable->GetDefaultSubobjectByName(FName("IngredientArea")));
        }
	}
	if (!DetectionArea) return nullptr;

	TArray<AActor*> OverlappingActors;
	DetectionArea->GetOverlappingActors(OverlappingActors, AInventoryItemActor::StaticClass());

	for (AActor* Actor : OverlappingActors)
	{
		AInventoryItemActor* ItemActor = Cast<AInventoryItemActor>(Actor);
		if (ItemActor && ItemActor->IsSliced())
		{
			return ItemActor;
		}
	}
	return nullptr;
}

void UCookingWidget::OnMinigameStarted(UCookingMinigameBase* Minigame)
{
	if (!Minigame || !MinigameSwitcher)
	{
		UE_LOG(LogTemp, Warning, TEXT("UCookingWidget::OnMinigameStarted - Invalid minigame or WidgetSwitcher is null."));
		return;
	}

	CurrentMinigame = Minigame;
	bIsInMinigameMode = true;

	TSubclassOf<UUserWidget> WidgetClassToUse = nullptr;
	UClass* MinigameClass = Minigame->GetClass();
	
	for (const auto& Pair : MinigameWidgetMap)
	{
		if (Pair.Key && MinigameClass->IsChildOf(Pair.Key))
		{
			WidgetClassToUse = Pair.Value;
			break;
		}
	}

	if (!WidgetClassToUse)
	{
		UE_LOG(LogTemp, Error, TEXT("OnMinigameStarted: No widget class found in MinigameWidgetMap for minigame class %s"), *MinigameClass->GetName());
		MinigameSwitcher->SetActiveWidgetIndex(-1);
		return;
	}

	UUserWidget* FoundWidget = nullptr;
	for (int32 i = 0; i < MinigameSwitcher->GetNumWidgets(); ++i)
	{
		if (UUserWidget* Widget = MinigameSwitcher->GetWidgetAtIndex(i))
		{
			if (Widget->IsA(WidgetClassToUse))
			{
				FoundWidget = Widget;
				break;
			}
		}
	}

	if (FoundWidget)
	{
		CurrentMinigameWidget = FoundWidget;
		MinigameSwitcher->SetActiveWidget(FoundWidget);
	}
	else
	{
		UUserWidget* NewMinigameWidget = CreateWidget<UUserWidget>(this, WidgetClassToUse);
		if (NewMinigameWidget)
		{
			const int32 NewWidgetIndex = MinigameSwitcher->AddChild(NewMinigameWidget);
			MinigameSwitcher->SetActiveWidgetIndex(NewWidgetIndex);
			CurrentMinigameWidget = NewMinigameWidget;
		}
	}

	if (CurrentMinigameWidget.IsValid())
	{
		if (IMinigameWidgetInterface* MinigameInterface = Cast<IMinigameWidgetInterface>(CurrentMinigameWidget.Get()))
		{
			MinigameInterface->Execute_InitializeMinigameWidget(CurrentMinigameWidget.Get(), Minigame);
		}
	}

	if (StatusText) StatusText->SetText(FText::FromString(TEXT("미니게임 시작!")));
	if (CookButton) CookButton->SetVisibility(ESlateVisibility::Collapsed);
	if (AddIngredientButton) AddIngredientButton->SetVisibility(ESlateVisibility::Collapsed);
	if (CollectButton) CollectButton->SetVisibility(ESlateVisibility::Collapsed);
}

void UCookingWidget::OnMinigameUpdated(float Score, int32 Phase)
{
	if (!bIsInMinigameMode || !CurrentMinigameWidget.IsValid()) return;
	
	if (IMinigameWidgetInterface* MinigameInterface = Cast<IMinigameWidgetInterface>(CurrentMinigameWidget.Get()))
	{
		MinigameInterface->Execute_UpdateMinigameWidget(CurrentMinigameWidget.Get(), Score, Phase);
	}
}

void UCookingWidget::OnMinigameEnded(int32 Result)
{
	bIsInMinigameMode = false;
	CurrentMinigame = nullptr;
	CurrentMinigameWidget = nullptr;

	if (MinigameSwitcher)
	{
		MinigameSwitcher->SetActiveWidgetIndex(-1);
	}
	
	if (StatusText)
	{
		FString ResultMessage;
		switch (Result)
		{
		case 1: ResultMessage = TEXT("완벽한 요리!"); break;
		case 2: ResultMessage = TEXT("좋은 요리!"); break;
		case 3: ResultMessage = TEXT("평범한 요리."); break;
		case 4: ResultMessage = TEXT("아쉬운 요리..."); break;
		case 5: ResultMessage = TEXT("요리 실패!"); break;
		default: ResultMessage = TEXT("요리 완료!"); break;
		}
		StatusText->SetText(FText::FromString(ResultMessage));
	}

	if (ActionText)
	{
		ActionText->SetText(FText::FromString(TEXT("")));
	}
	
	if (CollectButton && Result != 5) // 5 is Failed
	{
		CollectButton->SetVisibility(ESlateVisibility::Visible);
		CollectButton->SetIsEnabled(true);
	}

	if (CookButton)
	{
		CookButton->SetVisibility(ESlateVisibility::Collapsed); 
	}
}

FReply UCookingWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (bIsInMinigameMode && CurrentMinigameWidget.IsValid() && CurrentMinigameWidget->IsFocusable())
	{
		return CurrentMinigameWidget->NativeOnKeyDown(InGeometry, InKeyEvent);
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
} 