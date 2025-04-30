#include "InteractablePot.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Inventory/InventoryItemActor.h" // Assuming AInventoryItemActor exists and has GetItemID()
#include "UI/Inventory/CookingWidget.h" // Include the Cooking Widget header
#include "Engine/DataTable.h" // Already included, but good practice
#include "Inventory/CookingRecipeStruct.h" // Make sure this is included
#include "Inventory/InvenItemStruct.h" // Include the definition for FInvenItemStruct
#include "Characters/WarriorHeroCharacter.h" // Include player character header
#include "Inventory/InventoryComponent.h"   // Include inventory component header
#include "Inventory/InventoryItemActor.h"
#include "Inventory/SlotStruct.h" // Needed for FSlotStruct
#include "Components/BoxComponent.h" // Include for Interaction Box
#include "Engine/StaticMesh.h" // Needed for UStaticMesh
#include "Math/UnrealMathUtility.h" // Needed for FMath::RandRange
#include "Components/WidgetComponent.h"
#include "Inventory/InvenItemStruct.h" // Added include
#include "Inventory/InventoryComponent.h" // Ensure this is included if needed for FSlotStruct/AddItem
#include "Characters/WarriorHeroCharacter.h" // For casting Player Pawn

// Sets default values
AInteractablePot::AInteractablePot()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create the Pot Mesh Component
	PotMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PotMesh"));
	RootComponent = PotMesh; // Set the mesh as the root component

	// --- Interaction Volume Setup --- 
	InteractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionVolume"));
	InteractionVolume->SetupAttachment(RootComponent);
	InteractionVolume->SetBoxExtent(FVector(75.0f, 75.0f, 75.0f)); // Default size, adjust in BP
	InteractionVolume->SetCollisionProfileName(FName("InteractableObject")); // IMPORTANT: Use the same profile name as InteractableTable / Character's interaction query
	InteractionVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly); // Only needed for detection, not physics
	InteractionVolume->SetGenerateOverlapEvents(true); // Ensure it can generate overlaps if character uses overlap detection
	// --- End Interaction Volume Setup --- 

	// Create the Ingredient Detection Volume (For items inside the pot)
	IngredientDetectionVolume = CreateDefaultSubobject<USphereComponent>(TEXT("IngredientDetectionVolume"));
	IngredientDetectionVolume->SetupAttachment(RootComponent);
	IngredientDetectionVolume->SetSphereRadius(50.0f); // Smaller radius, likely inside the pot mesh, adjust in BP
	IngredientDetectionVolume->SetCollisionProfileName(TEXT("Trigger")); // Collision profile for ingredients (might differ from interaction)
	IngredientDetectionVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	IngredientDetectionVolume->SetGenerateOverlapEvents(true); // Needed for OnIngredientOverlapBegin (if used)

	// Create the Cooking Effect Particle System Component
	CookingEffectParticles = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("CookingEffectParticles"));
	CookingEffectParticles->SetupAttachment(RootComponent);
	CookingEffectParticles->bAutoActivate = false; // Don't activate initially

	CookingWidgetRef = nullptr; // Initialize widget reference

	// DataTables are now expected to be assigned via Blueprint defaults.
	// Removed FObjectFinder logic.
}

// Called when the game starts or when spawned
void AInteractablePot::BeginPlay()
{
	Super::BeginPlay();

	// Bind the overlap function
	IngredientDetectionVolume->OnComponentBeginOverlap.AddDynamic(this, &AInteractablePot::OnIngredientOverlapBegin);

}

// Called every frame
void AInteractablePot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AInteractablePot::OnIngredientOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// DEPRECATED: Logic moved to explicit AddIngredient called from player interaction or UI
	UE_LOG(LogTemp, Log, TEXT("AInteractablePot::OnIngredientOverlapBegin called but logic is deprecated."));
}

bool AInteractablePot::AddIngredient(FName IngredientID)
{
	if (IngredientID != NAME_None && ItemDataTable)
	{
		const FString ContextString(TEXT("Finding Ingredient Mesh"));
		FInventoryItemStruct* ItemData = ItemDataTable->FindRow<FInventoryItemStruct>(IngredientID, ContextString, true);

		UStaticMesh* IngredientMesh = ItemData ? ItemData->Mesh.LoadSynchronous() : nullptr;

		if (IngredientMesh)
		{
			AddedIngredientIDs.Add(IngredientID);
			UE_LOG(LogTemp, Log, TEXT("Added ingredient: %s"), *IngredientID.ToString());

			// Spawn the ingredient mesh
			UStaticMeshComponent* NewIngredientMesh = NewObject<UStaticMeshComponent>(this); // Create component owned by this actor
			if (NewIngredientMesh)
			{
				NewIngredientMesh->SetStaticMesh(IngredientMesh); // Use the loaded mesh
				NewIngredientMesh->SetupAttachment(PotMesh); // Attach to the pot mesh

				// Apply a small random offset so meshes don't perfectly overlap
				// Adjust ranges as needed based on pot/ingredient size
				FVector RandomOffset = FVector(
					FMath::RandRange(-IngredientSpawnXYOffsetRange, IngredientSpawnXYOffsetRange), // Use UPROPERTY variable
					FMath::RandRange(-IngredientSpawnXYOffsetRange, IngredientSpawnXYOffsetRange), // Use UPROPERTY variable
					0.0f
				);
				// Consider adding a base Z offset if needed so items don't spawn at the pot's origin
				FVector RelativeLocation = FVector(RandomOffset.X, RandomOffset.Y, IngredientSpawnZOffset); // Use the UPROPERTY variable for Z offset

				NewIngredientMesh->SetRelativeLocation(RelativeLocation);
				NewIngredientMesh->SetRelativeScale3D(IngredientSpawnScale); // Use the UPROPERTY variable for scale
				NewIngredientMesh->SetCollisionProfileName(TEXT("NoCollision")); // Ingredients inside probably don't need collision
				NewIngredientMesh->RegisterComponent(); // IMPORTANT: Register the new component

				IngredientMeshComponents.Add(NewIngredientMesh); // Add to our tracking array
				UE_LOG(LogTemp, Log, TEXT("Spawned mesh for ingredient: %s"), *IngredientID.ToString());
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to create UStaticMeshComponent for ingredient: %s"), *IngredientID.ToString());
			}


			// Notify the widget to update its display
			NotifyWidgetUpdate();
			return true;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Ingredient ID '%s' not found in ItemDataTable or has no Mesh assigned."), *IngredientID.ToString());
		}
	}
	return false;
}

void AInteractablePot::StartCooking()
{
	if (AddedIngredientIDs.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No ingredients in the pot to cook."));
		// Optional: Add feedback to player (sound, message)
		return;
	}

	FName ResultItemID = CheckRecipeInternal();

	if (ResultItemID != NAME_None)
	{
		UE_LOG(LogTemp, Log, TEXT("Starting to cook recipe for: %s"), *ResultItemID.ToString());
		// Activate cooking effects
		CookingEffectParticles->ActivateSystem();

		// Start the cooking timer
		GetWorldTimerManager().SetTimer(CookingTimerHandle, this, &AInteractablePot::OnCookingComplete, CookingDuration, false);

		// Optional: Disable adding more ingredients during cooking
		// Optional: Provide feedback that cooking has started
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid recipe."));
		// Optional: Provide failure feedback (sound, effect)
		// Optional: Clear ingredients or allow player to remove them? Decide based on game design.
		// AddedIngredientIDs.Empty(); // Example: Clear ingredients on failure
		// NotifyWidgetUpdate(); // Update UI if ingredients are cleared
	}
}

void AInteractablePot::OnCookingComplete()
{
	UE_LOG(LogTemp, Log, TEXT("AInteractablePot: Cooking complete!"));

	// Deactivate cooking effects
	CookingEffectParticles->DeactivateSystem();

	// Clear the timer handle
	GetWorldTimerManager().ClearTimer(CookingTimerHandle);

	FName ResultItemID = CheckRecipeInternal();

	if (ResultItemID != NAME_None)
	{
		UE_LOG(LogTemp, Log, TEXT("AInteractablePot: Successfully cooked: %s. Adding to player inventory."), *ResultItemID.ToString());

		APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
		if (PlayerController)
		{
            AWarriorHeroCharacter* PlayerChar = Cast<AWarriorHeroCharacter>(PlayerController->GetPawn());
            if (PlayerChar)
            {
                UInventoryComponent* PlayerInventory = PlayerChar->GetInventoryComponent();
                if (PlayerInventory)
                {
                    // Create the FSlotStruct for the item to add
                    FSlotStruct ItemToAdd;
                    const FString ContextString(TEXT("Finding Cooked Item Data"));
					FInventoryItemStruct* CookedItemData = ItemDataTable ? ItemDataTable->FindRow<FInventoryItemStruct>(ResultItemID, ContextString, true) : nullptr;

                    if (CookedItemData) // Check if we found item data for the result
                    {
                        ItemToAdd.ItemID.RowName = ResultItemID; 
                        ItemToAdd.Quantity = 1; // Assuming cooking always yields 1
                        // ItemToAdd.ItemID.DataTable = ItemDataTable; // Set if your AddItem requires it
                        
                        // Attempt to add using AddItem(FSlotStruct)
                        if (!PlayerInventory->AddItem(ItemToAdd)) // Assumes AddItem(FSlotStruct) exists
                        {
                            UE_LOG(LogTemp, Warning, TEXT("AInteractablePot: Failed to add cooked item '%s' to player inventory (Inventory full?). Dropping item instead."), *ResultItemID.ToString());
                            // TODO: Implement item dropping logic here if AddItem fails
                             // Get item data again to know what mesh to spawn for dropping
                             UStaticMesh* DroppedMesh = CookedItemData->Mesh.LoadSynchronous();
                            if (DroppedMesh)
                            {
                                FVector DropLocation = GetActorLocation() + GetActorForwardVector() * 100.0f + FVector(0,0,50.0f); // Example drop location slightly in front and above pot
                                FRotator DropRotation = FRotator::ZeroRotator;
                                FActorSpawnParameters SpawnParams;
                                SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
                                // Assume you have an AWorldItemActor class to represent dropped items
                                // AWorldItemActor* DroppedItem = GetWorld()->SpawnActor<AWorldItemActor>(AWorldItemActor::StaticClass(), DropLocation, DropRotation, SpawnParams);
                                // if (DroppedItem)
                                // {
                                //    DroppedItem->SetItemData(ResultItemID, 1); // Or pass the FInventoryItemStruct
                                // }
                                UE_LOG(LogTemp, Warning, TEXT("Item dropping not fully implemented yet."));
                            }

                        }
                        else
                        {
                             UE_LOG(LogTemp, Log, TEXT("AInteractablePot: Successfully added '%s' to player inventory."), *ResultItemID.ToString());
                             // Play success sound/effect?
                             ClearIngredientsVisually(); // Clear visual meshes
                             AddedIngredientIDs.Empty(); // Clear internal list
                             NotifyWidgetUpdate();      // Update UI
                        }
                    }
                     else
                    {
                        UE_LOG(LogTemp, Error, TEXT("AInteractablePot: Could not find item data for the cooked result ID '%s' in ItemDataTable."), *ResultItemID.ToString());
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("AInteractablePot: Could not get InventoryComponent from PlayerCharacter."));
                }
            }
            else
            {
                 UE_LOG(LogTemp, Error, TEXT("AInteractablePot: Could not cast Player Pawn to AWarriorHeroCharacter."));
            }
		}
        else
        {
            UE_LOG(LogTemp, Error, TEXT("AInteractablePot: Could not get PlayerController."));
        }
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("AInteractablePot: Cooking finished but CheckRecipeInternal returned NAME_None."));
	}

	AddedIngredientIDs.Empty();
	NotifyWidgetUpdate();
}


FName AInteractablePot::CheckRecipeInternal()
{
	if (!RecipeDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("AInteractablePot::CheckRecipeInternal - RecipeDataTable is not set!"));
		return NAME_None;
	}

	if (AddedIngredientIDs.Num() == 0)
	{
		return NAME_None; 
	}

	const TArray<FName> RowNames = RecipeDataTable->GetRowNames();
	TArray<FName> SortedPotIngredients = AddedIngredientIDs;
	SortedPotIngredients.Sort([](const FName& A, const FName& B) {
		return A.ToString() < B.ToString();
	});

	for (const FName& RowName : RowNames)
	{
		FCookingRecipeStruct* RecipeRow = RecipeDataTable->FindRow<FCookingRecipeStruct>(RowName, TEXT("CheckRecipeInternal"));

		if (RecipeRow)
		{
            // Use correct member names: RequiredIngredients and ResultItem
			if (RecipeRow->RequiredIngredients.Num() != SortedPotIngredients.Num())
			{
				continue; 
			}

            if (RecipeRow->RequiredIngredients.Num() == 0)
            {
                continue;
            }

			TArray<FName> SortedRecipeIngredients = RecipeRow->RequiredIngredients; // Use RequiredIngredients
			SortedRecipeIngredients.Sort([](const FName& A, const FName& B) {
				return A.ToString() < B.ToString();
			});

			if (SortedPotIngredients == SortedRecipeIngredients) 
			{
                // Use ResultItem for the output
				UE_LOG(LogTemp, Log, TEXT("Recipe Match Found! Recipe: %s, Output: %s"), *RowName.ToString(), *RecipeRow->ResultItem.ToString()); // Use ResultItem
				return RecipeRow->ResultItem; // Use ResultItem
			}
		}
        else
        {
             UE_LOG(LogTemp, Warning, TEXT("AInteractablePot::CheckRecipeInternal - Could not find row %s or cast failed."), *RowName.ToString());
        }
	}

	UE_LOG(LogTemp, Log, TEXT("No matching recipe found for current ingredients."));
	return NAME_None;
}

void AInteractablePot::NotifyWidgetUpdate()
{
	if (CookingWidgetRef) // Check TObjectPtr directly
	{
		UE_LOG(LogTemp, Log, TEXT("AInteractablePot: Notifying widget %s to update."), *CookingWidgetRef->GetName());
		CookingWidgetRef->UpdateIngredientList(AddedIngredientIDs); 
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AInteractablePot: CookingWidgetRef is not set or invalid. Cannot update UI."));
	}
}

// --- Getter/Setter Implementations ---

const TArray<FName>& AInteractablePot::GetAddedIngredientIDs() const
{
    return AddedIngredientIDs;
}

void AInteractablePot::SetCookingWidget(UCookingWidget* InWidget)
{
    CookingWidgetRef = InWidget;
}

UCookingWidget* AInteractablePot::GetCookingWidget() const
{
    return CookingWidgetRef.Get(); // Use Get() for TObjectPtr
}

// New function definition
void AInteractablePot::ClearIngredientsVisually()
{
	for (UStaticMeshComponent* MeshComp : IngredientMeshComponents)
	{
		if (MeshComp && !MeshComp->IsBeingDestroyed())
		{
			MeshComp->DestroyComponent();
		}
	}
	IngredientMeshComponents.Empty(); // Clear the array after destroying components
} 