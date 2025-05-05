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
#include "Sound/SoundBase.h" // Include for USoundBase check
#include "NiagaraComponent.h" // Include Niagara Component header
#include "NiagaraFunctionLibrary.h" // Include Niagara Function Library for spawning systems if needed, and managing components

// Sets default values
AInteractablePot::AInteractablePot()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create the Pot Mesh Component
	PotMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PotMesh"));
	RootComponent = PotMesh; // Set the mesh as the root component

	// --- Create Firewood Mesh ---
	FirewoodMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FirewoodMesh"));
	FirewoodMesh->SetupAttachment(RootComponent);
	// Position the firewood below the pot in the Blueprint editor
	// --- End Firewood Mesh ---

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

	// Create the Cooking Steam Particle System Component (Cascade)
	CookingSteamParticles = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("CookingSteamParticles")); // Corrected name
	CookingSteamParticles->SetupAttachment(RootComponent);
	CookingSteamParticles->bAutoActivate = false; // Don't activate initially

	// --- Create Fire Effect Niagara Component ---
	FireEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FireEffectComponent"));
	FireEffectComponent->SetupAttachment(FirewoodMesh ? FirewoodMesh : RootComponent); // Attach to firewood mesh if available, otherwise root
	FireEffectComponent->bAutoActivate = false; // Don't activate initially
	// Note: The actual Niagara System asset (FireNiagaraSystem) is assigned in the Blueprint defaults.
	// --- End Fire Effect Niagara Component ---

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

	// Ensure Fire Effect component uses the assigned Niagara system
	if (FireNiagaraSystem && FireEffectComponent)
	{
		FireEffectComponent->SetAsset(FireNiagaraSystem);
	}
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

			// --- Play Add Ingredient Sound --- 
			if (AddIngredientSound)
			{
				UGameplayStatics::PlaySoundAtLocation(this, AddIngredientSound, GetActorLocation());
			}
			// ---------------------------------

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
		return;
	}

	// Check if ingredients match a known recipe
	FName ResultItemID = CheckRecipeInternal(); // This is the ID of the *output item* (e.g., PotatoSoup)

	if (ResultItemID != NAME_None)
	{
		// --- Check if Player Knows the Recipe ---
		bool bPlayerKnowsRecipe = false;
		APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
		if (PlayerController)
		{
			AWarriorHeroCharacter* PlayerChar = Cast<AWarriorHeroCharacter>(PlayerController->GetPawn());
			if (PlayerChar)
			{
				UInventoryComponent* PlayerInventory = PlayerChar->GetInventoryComponent();
				if (PlayerInventory && ItemDataTable)
				{
					for (const FSlotStruct& Slot : PlayerInventory->InventorySlots)
					{
						if (Slot.ItemType == EInventoryItemType::EIT_Recipe && !Slot.ItemID.RowName.IsNone())
						{
							const FString ContextString(TEXT("Checking Recipe Knowledge"));
							FInventoryItemStruct* RecipeItemData = ItemDataTable->FindRow<FInventoryItemStruct>(Slot.ItemID.RowName, ContextString);

							if (RecipeItemData && RecipeItemData->UnlocksRecipeID == ResultItemID)
							{
								bPlayerKnowsRecipe = true;
								UE_LOG(LogTemp, Log, TEXT("Player knows recipe %s because they have item %s."), *ResultItemID.ToString(), *Slot.ItemID.RowName.ToString());
								break;
							}
						}
					}
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("AInteractablePot::StartCooking - PlayerInventory or ItemDataTable is invalid. Cannot check recipe knowledge."));
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("AInteractablePot::StartCooking - Could not cast Player Pawn to AWarriorHeroCharacter."));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("AInteractablePot::StartCooking - Could not get PlayerController."));
		}
		// --- End Recipe Knowledge Check ---

		if (bPlayerKnowsRecipe)
		{
			UE_LOG(LogTemp, Log, TEXT("Starting to cook recipe for: %s"), *ResultItemID.ToString());

			// --- Activate Cooking Effects ---
			if (CookingSteamParticles) CookingSteamParticles->ActivateSystem(); // Activate steam
			if (FireEffectComponent) FireEffectComponent->Activate(); // Activate fire
			// -----------------------------

			// --- Play Start Cooking Sound ---
			if (StartCookingSound)
			{
				UGameplayStatics::PlaySoundAtLocation(this, StartCookingSound, GetActorLocation());
			}
			// --------------------------------

			// Start the cooking timer
			GetWorldTimerManager().SetTimer(CookingTimerHandle, this, &AInteractablePot::OnCookingComplete, CookingDuration, false);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Player attempted to cook %s, but does not have the required recipe item."), *ResultItemID.ToString());
			if (CookingFailSound)
			{
				UGameplayStatics::PlaySoundAtLocation(this, CookingFailSound, GetActorLocation());
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid recipe based on ingredients."));
		if (CookingFailSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, CookingFailSound, GetActorLocation());
		}
	}
}

void AInteractablePot::OnCookingComplete()
{
	UE_LOG(LogTemp, Log, TEXT("AInteractablePot: Cooking complete!"));

	// --- Deactivate Cooking Effects ---
	if (CookingSteamParticles) CookingSteamParticles->DeactivateSystem(); // Deactivate steam
	if (FireEffectComponent) FireEffectComponent->Deactivate(); // Deactivate fire
	// -------------------------------

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
					FSlotStruct ItemToAdd;
					const FString ContextString(TEXT("Finding Cooked Item Data"));
					FInventoryItemStruct* CookedItemData = ItemDataTable ? ItemDataTable->FindRow<FInventoryItemStruct>(ResultItemID, ContextString, true) : nullptr;

					if (CookedItemData)
					{
						ItemToAdd.ItemID.RowName = ResultItemID;
						ItemToAdd.Quantity = 1;
						ItemToAdd.ItemType = CookedItemData->ItemType;

						if (!PlayerInventory->AddItem(ItemToAdd))
						{
							UE_LOG(LogTemp, Warning, TEXT("AInteractablePot: Failed to add cooked item '%s' to player inventory (Inventory full?). Dropping item logic needed."), *ResultItemID.ToString());
							if (CookingFailSound)
							{
								UGameplayStatics::PlaySoundAtLocation(this, CookingFailSound, GetActorLocation());
							}
							// Implement item dropping here if needed
						}
						else
						{
							UE_LOG(LogTemp, Log, TEXT("AInteractablePot: Successfully added '%s' to player inventory."), *ResultItemID.ToString());
							if (CookingSuccessSound)
							{
								UGameplayStatics::PlaySoundAtLocation(this, CookingSuccessSound, GetActorLocation());
							}
							ClearIngredientsVisually(); // Clear visual meshes & effects on success
							AddedIngredientIDs.Empty();
							NotifyWidgetUpdate();
							return; // Exit early on success after clearing
						}
					}
					else
					{
						UE_LOG(LogTemp, Error, TEXT("AInteractablePot: Could not find item data for the cooked result ID '%s' in ItemDataTable."), *ResultItemID.ToString());
						if (CookingFailSound) UGameplayStatics::PlaySoundAtLocation(this, CookingFailSound, GetActorLocation());
					}
				}
				else UE_LOG(LogTemp, Error, TEXT("AInteractablePot: Could not get InventoryComponent from PlayerCharacter."));
			}
			else UE_LOG(LogTemp, Error, TEXT("AInteractablePot: Could not cast Player Pawn to AWarriorHeroCharacter."));
		}
		else UE_LOG(LogTemp, Error, TEXT("AInteractablePot: Could not get PlayerController."));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("AInteractablePot: Cooking finished but CheckRecipeInternal returned NAME_None (Invalid Recipe)."));
		if (CookingFailSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, CookingFailSound, GetActorLocation());
		}
	}

	// Clear ingredients visually and data if cooking failed or item couldn't be added
	AddedIngredientIDs.Empty();
	ClearIngredientsVisually(); // This now also handles deactivating effects if they were active
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
			if (RecipeRow->RequiredIngredients.Num() != SortedPotIngredients.Num())
			{
				continue; 
			}

			if (RecipeRow->RequiredIngredients.Num() == 0)
			{
				continue;
			}

			TArray<FName> SortedRecipeIngredients = RecipeRow->RequiredIngredients;
			SortedRecipeIngredients.Sort([](const FName& A, const FName& B) {
				return A.ToString() < B.ToString();
			});

			if (SortedPotIngredients == SortedRecipeIngredients) 
			{
				UE_LOG(LogTemp, Log, TEXT("Recipe Match Found! Recipe: %s, Output: %s"), *RowName.ToString(), *RecipeRow->ResultItem.ToString());
				return RecipeRow->ResultItem;
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

// Clears visual meshes and ensures effects are stopped
void AInteractablePot::ClearIngredientsVisually()
{
	for (UStaticMeshComponent* MeshComp : IngredientMeshComponents)
	{
		if (MeshComp && !MeshComp->IsBeingDestroyed())
		{
			MeshComp->DestroyComponent();
		}
	}
	IngredientMeshComponents.Empty();

	// Ensure effects are deactivated when clearing visuals
	if (CookingSteamParticles && CookingSteamParticles->IsActive())
	{ // Use IsActive() which returns true if component is active and system is running
		CookingSteamParticles->DeactivateSystem();
	}
	if (FireEffectComponent && FireEffectComponent->IsActive())
	{ // Use IsActive() for Niagara Component as well
		FireEffectComponent->Deactivate();
	}
} 