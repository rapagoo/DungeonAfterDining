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
#include "Interactables/InteractableTable.h" // Added for OwningTable
#include "Components/SceneComponent.h" // Added for PotLocationComponent
#include "Components/StaticMeshComponent.h" // Added for UStaticMeshComponent
#include "ProceduralMeshComponent.h" // Corrected include path

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

	NearbyIngredient = nullptr;
	AddedIngredientIDs.Empty();
	IngredientActorsInPot.Empty();
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
	// Log the state of conditions before the check
	AWarriorHeroCharacter* PlayerCharacter = Cast<AWarriorHeroCharacter>(GetOwningPlayerPawn());
	AInteractableTable* CurrentTable = PlayerCharacter ? PlayerCharacter->GetCurrentInteractableTable() : nullptr;

	UE_LOG(LogTemp, Log, TEXT("OnAddIngredientClicked: NearbyIngredient Valid? %s, PlayerCharacter Valid? %s, CurrentTable Valid? %s"), 
		NearbyIngredient.IsValid() ? TEXT("Yes") : TEXT("No"),
		IsValid(PlayerCharacter) ? TEXT("Yes") : TEXT("No"),
		IsValid(CurrentTable) ? TEXT("Yes") : TEXT("No"));

	if (NearbyIngredient.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("OnAddIngredientClicked: NearbyIngredient '%s' IsSliced? %s"), 
			*NearbyIngredient->GetName(), 
			NearbyIngredient->IsSliced() ? TEXT("Yes") : TEXT("No"));
	}

	// Check if we have a valid, sliced ingredient actor nearby AND the player character has a valid current table
	if (NearbyIngredient.IsValid() && NearbyIngredient->IsSliced() && CurrentTable)
	{
		AInventoryItemActor* IngredientToAdd = NearbyIngredient.Get();
		FSlotStruct IngredientData = IngredientToAdd->GetItemData();
		FName IngredientID = IngredientData.ItemID.RowName;

		UE_LOG(LogTemp, Warning, TEXT("Adding Ingredient ID: %s"), *IngredientID.ToString());

		AddedIngredientIDs.Add(IngredientID);

		if (IngredientsList)
		{
			UTextBlock* NewIngredientText = NewObject<UTextBlock>(this);
			if (NewIngredientText)
			{
				NewIngredientText->SetText(FText::FromName(IngredientID));
				IngredientsList->AddChildToVerticalBox(NewIngredientText);
			}
		}

		// Move the actor to the pot location using the table obtained from the character
		USceneComponent* PotLocationComp = CurrentTable->GetPotLocationComponent();
		if (PotLocationComp)
		{
			// Calculate target transform for the ACTOR's origin at the pot location
			FTransform TargetActorOriginTransform = PotLocationComp->GetComponentTransform();
			// Optional: Add slight random offset to location for visual variation
			FVector RandomOffset = FVector(FMath::RandRange(-2.0f, 2.0f), FMath::RandRange(-2.0f, 2.0f), FMath::RandRange(0.0f, 5.0f));
			TargetActorOriginTransform.AddToTranslation(RandomOffset);

			UE_LOG(LogTemp, Log, TEXT("Teleporting components of %s towards target origin: %s"), *IngredientToAdd->GetName(), *TargetActorOriginTransform.ToString());

			// --- Get the designated component to teleport ---
			UProceduralMeshComponent* MeshCompToTeleport = IngredientToAdd->GetPotTargetMeshComponent(); // Helper function needed in AInventoryItemActor

			if (MeshCompToTeleport && MeshCompToTeleport->IsSimulatingPhysics())
			{
				UE_LOG(LogTemp, Log, TEXT("  - Teleporting designated component: %s"), *MeshCompToTeleport->GetName());
				// Stop current physics movement before teleport
				MeshCompToTeleport->SetPhysicsLinearVelocity(FVector::ZeroVector);
				MeshCompToTeleport->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);

				// Calculate the component's target world transform based on its relative offset from the actor's target origin
				// IMPORTANT: Using the designated component's relative transform
				FTransform RelativeCompTransform = MeshCompToTeleport->GetRelativeTransform();
				FTransform TargetCompWorldTransform = RelativeCompTransform * TargetActorOriginTransform;

				// Teleport the physics body
				MeshCompToTeleport->SetWorldTransform(TargetCompWorldTransform, false, nullptr, ETeleportType::TeleportPhysics);

				// --- Log FINAL location and visibility AFTER teleport ---
				FVector FinalCompLoc = MeshCompToTeleport->GetComponentLocation();
				UE_LOG(LogTemp, Log, TEXT("Post-Teleport Check for %s Component:"), *MeshCompToTeleport->GetName());
				UE_LOG(LogTemp, Log, TEXT("  - Final Component World Location: %s (Visible: %s, SimPhys: %s)"),
					   *FinalCompLoc.ToString(),
					   MeshCompToTeleport->IsVisible() ? TEXT("Yes") : TEXT("No"),
					   MeshCompToTeleport->IsSimulatingPhysics() ? TEXT("Yes") : TEXT("No"));

				IngredientActorsInPot.Add(IngredientToAdd); // Still track the actor
				UE_LOG(LogTemp, Log, TEXT("Finished teleporting component for %s."), *IngredientToAdd->GetName());
			}
			else if (MeshCompToTeleport)
			{
				 UE_LOG(LogTemp, Warning, TEXT("  - Designated component %s exists but is not simulating physics. Cannot teleport physics body."), *MeshCompToTeleport->GetName());
				 // Optionally handle non-simulating move here if needed
			}
			else
			{
				 UE_LOG(LogTemp, Error, TEXT("Could not find designated PotTargetMeshComponent on IngredientToAdd %s."), *IngredientToAdd->GetName());
				 // Consider destroying the actor or handling the error
				 IngredientToAdd->Destroy(); // Example: Destroy if component is missing
			}

			// --- DO NOT Move the actor itself ---
			// IngredientToAdd->SetActorTransform(TargetActorOriginTransform, false, nullptr, ETeleportType::None);

			
			NearbyIngredient = nullptr;
			if (AddIngredientButton)
			{
				AddIngredientButton->SetIsEnabled(false);
			}

			FName TempResultID;
			bool bRecipeFound = CheckRecipe(TempResultID);
			if (CookButton)
			{
				CookButton->SetIsEnabled(bRecipeFound);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Could not find PotLocationComponent on CurrentTable %s. Destroying ingredient instead."), *GetNameSafe(CurrentTable));
			IngredientToAdd->Destroy();
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Add Ingredient Clicked, but NearbyIngredient is not valid/sliced or Player Character/CurrentTable is not valid."));
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

					// Destroy the ingredient actors that were moved to the pot
					for (TWeakObjectPtr<AInventoryItemActor> WeakActorPtr : IngredientActorsInPot)
					{
						if (WeakActorPtr.IsValid())
						{
							WeakActorPtr.Get()->Destroy();
						}
					}
					IngredientActorsInPot.Empty(); // Clear the tracking array

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