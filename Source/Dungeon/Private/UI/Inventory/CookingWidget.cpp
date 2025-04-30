// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Inventory/CookingWidget.h"
#include "Components/Button.h" // Include for UButton
#include "Components/VerticalBox.h" // Include for UVerticalBox
#include "Components/TextBlock.h" // Include for UTextBlock (Example for adding items later)
#include "Inventory/InventoryItemActor.h" // Include the item actor class
#include "Inventory/SlotStruct.h" // Needed for FSlotStruct
#include "Inventory/CookingRecipeStruct.h" // Include recipe struct
#include "Engine/DataTable.h" // Ensure DataTable is included
#include "Characters/WarriorHeroCharacter.h" // Include character to get inventory
#include "Inventory/InventoryComponent.h" // Include inventory component

void UCookingWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Bind functions to button click events
	if (AddIngredientButton)
	{
		AddIngredientButton->OnClicked.AddDynamic(this, &UCookingWidget::OnAddIngredientClicked);
		// Start disabled, enabled by UpdateNearbyIngredient
		AddIngredientButton->SetIsEnabled(false);
	}
	if (CookButton)
	{
		CookButton->OnClicked.AddDynamic(this, &UCookingWidget::OnCookClicked);
		// Initially disable the cook button until valid ingredients are added
		CookButton->SetIsEnabled(false);
	}

	NearbyIngredient = nullptr;
	AddedIngredientIDs.Empty(); // Initialize the array
}

void UCookingWidget::UpdateNearbyIngredient(AInventoryItemActor* ItemActor)
{
	bool bShouldEnableButton = false;
	if (ItemActor && ItemActor->IsSliced()) // Check if the item actor is valid AND sliced
	{
		NearbyIngredient = ItemActor;
		bShouldEnableButton = true;
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
	// Check if we have a valid, sliced ingredient actor nearby
	if (NearbyIngredient.IsValid() && NearbyIngredient->IsSliced())
	{
		AInventoryItemActor* IngredientToAdd = NearbyIngredient.Get();
		FSlotStruct IngredientData = IngredientToAdd->GetItemData();
		FName IngredientID = IngredientData.ItemID.RowName; // Get Item ID as FName

		UE_LOG(LogTemp, Warning, TEXT("Adding Ingredient ID: %s"), *IngredientID.ToString());

		// Add the ingredient ID to our internal list
		AddedIngredientIDs.Add(IngredientID);

		// Update the UI list
		if (IngredientsList)
		{
			UTextBlock* NewIngredientText = NewObject<UTextBlock>(this);
			if (NewIngredientText)
			{
				// Ideally, get a display name from ItemData table here
				NewIngredientText->SetText(FText::FromName(IngredientID)); // Use FText::FromName
				IngredientsList->AddChildToVerticalBox(NewIngredientText);
			}
		}

		// Destroy the actor in the world
		IngredientToAdd->Destroy();

		// Clear the nearby ingredient pointer and disable the button
		NearbyIngredient = nullptr;
		if (AddIngredientButton)
		{
			AddIngredientButton->SetIsEnabled(false);
		}

		// Check if the current ingredients match a recipe
		FName TempResultID;
		bool bRecipeFound = CheckRecipe(TempResultID);
		if (CookButton)
		{
			CookButton->SetIsEnabled(bRecipeFound);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Add Ingredient Clicked, but NearbyIngredient is not valid or not sliced."));
	}
}

void UCookingWidget::OnCookClicked()
{
	FName ResultItemID;
	if (CheckRecipe(ResultItemID))
	{
		UE_LOG(LogTemp, Log, TEXT("Cook button clicked. Valid recipe found for result: %s"), *ResultItemID.ToString());

		// Get Player Character and Inventory Component
		APawn* OwningPawn = GetOwningPlayerPawn();
		AWarriorHeroCharacter* PlayerCharacter = Cast<AWarriorHeroCharacter>(OwningPawn);
		if (PlayerCharacter)
		{
			UInventoryComponent* PlayerInventory = PlayerCharacter->GetInventoryComponent();
			if (PlayerInventory)
			{
				// Attempt to add the resulting item to the inventory
				FSlotStruct ResultItemData;
				ResultItemData.ItemID.RowName = ResultItemID;
				ResultItemData.Quantity = 1;

				if (PlayerInventory->AddItem(ResultItemData))
				{
					UE_LOG(LogTemp, Log, TEXT("Successfully added cooked item '%s' to inventory."), *ResultItemID.ToString());

					// Clear the internal list and UI
					AddedIngredientIDs.Empty();
					if (IngredientsList)
					{
						IngredientsList->ClearChildren();
					}

					// Disable the cook button again
					if (CookButton)
					{
						CookButton->SetIsEnabled(false);
					}

					// TODO: Play success animation/sound
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("Failed to add cooked item '%s' to inventory (Inventory might be full?). Cooking aborted, ingredients remain."), *ResultItemID.ToString());
					// Optionally provide feedback to the player that inventory is full
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("CookClicked: Could not get InventoryComponent from PlayerCharacter."));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("CookClicked: Could not get Owning Player Character."));
		}
	}
	else
	{
		// This case should ideally not happen if the button is disabled correctly
		UE_LOG(LogTemp, Warning, TEXT("Cook button clicked, but no valid recipe found for current ingredients."));
		if (CookButton)
		{
			CookButton->SetIsEnabled(false);
		}
	}
}

bool UCookingWidget::CheckRecipe(FName& OutResultItemID) const
{
	if (!RecipeDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("CheckRecipe: RecipeDataTable is not set!"));
		return false;
	}

	// Get all recipe row names
	TArray<FName> RecipeRowNames = RecipeDataTable->GetRowNames();

	for (const FName& RowName : RecipeRowNames)
	{
		FCookingRecipeStruct* Recipe = RecipeDataTable->FindRow<FCookingRecipeStruct>(RowName, TEXT("CheckRecipe Context"));
		if (Recipe)
		{
			// Check if the number of ingredients matches first
			if (Recipe->RequiredIngredients.Num() == AddedIngredientIDs.Num())
			{
				// Check if all ingredients match exactly (order matters)
				bool bMatch = true;
				for (int32 i = 0; i < AddedIngredientIDs.Num(); ++i)
				{
					if (AddedIngredientIDs[i] != Recipe->RequiredIngredients[i])
					{
						bMatch = false;
						break;
					}
				}

				if (bMatch)
				{
					// Found a matching recipe
					OutResultItemID = Recipe->ResultItem;
					UE_LOG(LogTemp, Log, TEXT("CheckRecipe: Found matching recipe '%s' for result '%s'"), *RowName.ToString(), *OutResultItemID.ToString());
					return true;
				}
			}
		}
	}

	// No matching recipe found
	OutResultItemID = NAME_None;
	// UE_LOG(LogTemp, Log, TEXT("CheckRecipe: No matching recipe found for current ingredients.")); // Optional: Reduce log spam
	return false;
} 