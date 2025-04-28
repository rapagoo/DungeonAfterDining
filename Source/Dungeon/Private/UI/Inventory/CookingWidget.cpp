// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Inventory/CookingWidget.h"
#include "Components/Button.h" // Include for UButton
#include "Components/VerticalBox.h" // Include for UVerticalBox
#include "Components/TextBlock.h" // Include for UTextBlock (Example for adding items later)
#include "Inventory/InventoryItemActor.h" // Include the item actor class
#include "Inventory/SlotStruct.h" // Needed for FSlotStruct

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
		FString IngredientName = IngredientData.ItemID.RowName.ToString(); // Get Item ID as string

		UE_LOG(LogTemp, Warning, TEXT("Adding Ingredient: %s"), *IngredientName);

		// Add the ingredient name to our internal list
		AddedIngredientNames.Add(IngredientName);

		// Update the UI list
		if (IngredientsList)
		{
			UTextBlock* NewIngredientText = NewObject<UTextBlock>(this);
			if (NewIngredientText)
			{
				// Ideally, get a display name from ItemData table here
				NewIngredientText->SetText(FText::FromString(IngredientName));
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

		// TODO: Check if the current ingredients match a recipe and enable/disable CookButton
		// Placeholder: Enable cook button if we have at least one ingredient
		if (CookButton && AddedIngredientNames.Num() > 0)
		{
			CookButton->SetIsEnabled(true); // Enable based on recipe check later
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Add Ingredient Clicked, but NearbyIngredient is not valid or not sliced."));
	}
}

void UCookingWidget::OnCookClicked()
{
	// TODO: Verify AddedIngredientNames against recipes

	UE_LOG(LogTemp, Warning, TEXT("Cook Clicked! Ingredients: %s"), *FString::Join(AddedIngredientNames, TEXT(", ")));

	// Placeholder: Clear the list and disable cook button after cooking
	if (IngredientsList)
	{
		IngredientsList->ClearChildren();
	}
	AddedIngredientNames.Empty(); // Clear the internal list as well

	if (CookButton)
	{
		CookButton->SetIsEnabled(false);
	}
	// Add ingredient button remains disabled until a new one is detected
} 