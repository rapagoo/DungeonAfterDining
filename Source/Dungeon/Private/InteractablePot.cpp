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
#include "Inventory/InventoryComponent.h" // Ensure this is included if needed for FSlotStruct/AddItem
#include "Characters/WarriorHeroCharacter.h" // For casting Player Pawn
#include "Sound/SoundBase.h" // Include for USoundBase check
#include "NiagaraComponent.h" // Include Niagara Component header
#include "NiagaraFunctionLibrary.h" // Include Niagara Function Library for spawning systems if needed, and managing components
#include "Materials/MaterialInstanceDynamic.h" // Include for MID

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

	// --- Initialize New Variables ---
	bIsCooking = false;
	bIsCookingComplete = false;
	bIsBurnt = false;
	CurrentCookedResultID = NAME_None;
	CookingStartTime = 0.0f;
	BurningStartTime = 0.0f;
	CookingMaterialParamName = FName("CookAmount"); // Default name, adjustable in BP
	CookedMaterialParamValue = 1.0f; // Default value, adjustable in BP
	BurntMaterialParamValue = 2.0f; // Default value, adjustable in BP
	InitialMaterialParamValue = 0.0f; // Default start value
	// --- End Initialization ---
}

// Called when the game starts or when spawned
void AInteractablePot::BeginPlay()
{
	Super::BeginPlay();

	// Bind the overlap function
	// IngredientDetectionVolume->OnComponentBeginOverlap.AddDynamic(this, &AInteractablePot::OnIngredientOverlapBegin);

	// Ensure Fire Effect component uses the assigned Niagara system AND SETS ITS SCALE
	if (FireEffectComponent) // FireEffectComponent가 유효한지 먼저 확인
	{
		if (FireNiagaraSystem)
		{
			FireEffectComponent->SetAsset(FireNiagaraSystem);
			FireEffectComponent->SetRelativeScale3D(FireEffectScale); 
			UE_LOG(LogTemp, Log, TEXT("AInteractablePot: Fire effect asset set and scale applied: %s"), *FireEffectScale.ToString());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("AInteractablePot: FireNiagaraSystem is NOT assigned in Blueprint defaults. Fire effect will not play or scale correctly."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("AInteractablePot: FireEffectComponent is null in BeginPlay. Cannot set asset or scale."));
	}
}

// Called every frame
void AInteractablePot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Only proceed if there are MIDs to update
	if (IngredientMIDMap.IsEmpty())
	{
		return;
	}

	float CurrentTime = GetWorld()->GetTimeSeconds();
	float TargetParamValue = InitialMaterialParamValue;
	float InterpAlpha = 0.0f;
	bool bUpdateMaterial = false;

	// --- Material Parameter Update Logic ---
	if (bIsCooking && CookingTimerHandle.IsValid())
	{
		// Interpolate towards CookedMaterialParamValue during cooking
		float ElapsedCookingTime = CurrentTime - CookingStartTime;
		InterpAlpha = FMath::Clamp(ElapsedCookingTime / CookingDuration, 0.0f, 1.0f);
		TargetParamValue = FMath::Lerp(InitialMaterialParamValue, CookedMaterialParamValue, InterpAlpha);
		bUpdateMaterial = true;
	}
	else if (bIsCookingComplete && BurningTimerHandle.IsValid() && !bIsBurnt) // Check bIsCookingComplete as well
	{
		// Interpolate towards BurntMaterialParamValue after cooking is complete, before burning
		float ElapsedBurningTime = CurrentTime - BurningStartTime;
		InterpAlpha = FMath::Clamp(ElapsedBurningTime / BurningDuration, 0.0f, 1.0f);
		TargetParamValue = FMath::Lerp(CookedMaterialParamValue, BurntMaterialParamValue, InterpAlpha);
		bUpdateMaterial = true;
	}
	else if (bIsBurnt)
	{
		// Ensure burnt state is fully set (Might have been set already in OnBurningComplete, but Tick ensures it)
		TargetParamValue = BurntMaterialParamValue;
		// Check if it needs updating (e.g., if OnBurningComplete was missed or delayed)
		UMaterialInstanceDynamic* FirstMID = IngredientMIDMap.begin().Value().Get(); // Get first MID to check value
		if (FirstMID)
		{
			float CurrentValue;
			if (FirstMID->GetScalarParameterValue(CookingMaterialParamName, CurrentValue) && !FMath::IsNearlyEqual(CurrentValue, TargetParamValue))
			{
				bUpdateMaterial = true;
			}
		}
	}
	else if (bIsCookingComplete && !BurningTimerHandle.IsValid() && !bIsBurnt) // If cooking is complete but burning timer isn't running yet or finished early
	{
		// Ensure cooked state is fully set
		TargetParamValue = CookedMaterialParamValue;
		// Check if it needs updating
		UMaterialInstanceDynamic* FirstMID = IngredientMIDMap.begin().Value().Get();
		if (FirstMID)
		{
			float CurrentValue;
			if (FirstMID->GetScalarParameterValue(CookingMaterialParamName, CurrentValue) && !FMath::IsNearlyEqual(CurrentValue, TargetParamValue))
			{
				bUpdateMaterial = true;
			}
		}
	}
	// Else: Not cooking, not complete, not burnt -> Do nothing, meshes should be at InitialMaterialParamValue or cleared

	// Apply the parameter value to all ingredient MIDs if an update is needed
	if (bUpdateMaterial)
	{
		for (auto const& [MeshComp, MID] : IngredientMIDMap)
		{
			if (MeshComp && MID) // Check validity
			{
				MID->SetScalarParameterValue(CookingMaterialParamName, TargetParamValue);
			}
		}
	}
	// --- End Material Parameter Update ---
}

void AInteractablePot::OnIngredientOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// DEPRECATED: Logic moved to explicit AddIngredient called from player interaction or UI
	UE_LOG(LogTemp, Log, TEXT("AInteractablePot::OnIngredientOverlapBegin called but logic is deprecated."));
}

bool AInteractablePot::AddIngredient(FName IngredientID)
{
	// Prevent adding ingredients if cooking, completed, or burnt
	if (bIsCooking || bIsCookingComplete || bIsBurnt)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot add ingredients while cooking, after completion, or if burnt."));
		return false;
	}

	if (IngredientID != NAME_None && ItemDataTable)
	{
		const FString ContextString(TEXT("Finding Ingredient Mesh"));
		FInventoryItemStruct* ItemData = ItemDataTable->FindRow<FInventoryItemStruct>(IngredientID, ContextString, true);

		UStaticMesh* IngredientMeshAsset = ItemData ? ItemData->Mesh.LoadSynchronous() : nullptr;

		if (IngredientMeshAsset)
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
			UStaticMeshComponent* NewIngredientMeshComp = NewObject<UStaticMeshComponent>(this); // Create component owned by this actor
			if (NewIngredientMeshComp)
			{
				NewIngredientMeshComp->SetStaticMesh(IngredientMeshAsset); // Use the loaded mesh
				NewIngredientMeshComp->SetupAttachment(PotMesh); // Attach to the pot mesh

				// Apply a small random offset so meshes don't perfectly overlap
				// Adjust ranges as needed based on pot/ingredient size
				FVector RandomOffset = FVector(
					FMath::RandRange(-IngredientSpawnXYOffsetRange, IngredientSpawnXYOffsetRange), // Use UPROPERTY variable
					FMath::RandRange(-IngredientSpawnXYOffsetRange, IngredientSpawnXYOffsetRange), // Use UPROPERTY variable
					0.0f
				);
				// Consider adding a base Z offset if needed so items don't spawn at the pot's origin
				FVector RelativeLocation = FVector(RandomOffset.X, RandomOffset.Y, IngredientSpawnZOffset); // Use the UPROPERTY variable for Z offset

				NewIngredientMeshComp->SetRelativeLocation(RelativeLocation);
				NewIngredientMeshComp->SetRelativeScale3D(IngredientSpawnScale); // Use the UPROPERTY variable for scale
				NewIngredientMeshComp->SetCollisionProfileName(TEXT("NoCollision")); // Ingredients inside probably don't need collision
				NewIngredientMeshComp->RegisterComponent(); // IMPORTANT: Register the new component

				// --- Create and Store Dynamic Material Instance (MID) ---
				UMaterialInterface* BaseMaterial = NewIngredientMeshComp->GetMaterial(0); // Assuming material is at index 0
				if (BaseMaterial)
				{
					UMaterialInstanceDynamic* MID = NewIngredientMeshComp->CreateDynamicMaterialInstance(0, BaseMaterial);
					if (MID)
					{
						MID->SetScalarParameterValue(CookingMaterialParamName, InitialMaterialParamValue); // Initialize parameter
						IngredientMIDMap.Add(NewIngredientMeshComp, MID); // Store the MID
						UE_LOG(LogTemp, Log, TEXT("Created MID for ingredient: %s"), *IngredientID.ToString());
					}
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("Failed to create MID for ingredient: %s"), *IngredientID.ToString());
					}
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("Ingredient mesh for %s has no material at index 0."), *IngredientID.ToString());
				}
				// --- End MID Creation ---

				IngredientMeshComponents.Add(NewIngredientMeshComp); // Still track the component itself
				UE_LOG(LogTemp, Log, TEXT("Spawned mesh for ingredient: %s"), *IngredientID.ToString());
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to create UStaticMeshComponent for ingredient: %s"), *IngredientID.ToString());
			}

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
	if (bIsCooking || bIsCookingComplete || bIsBurnt)
	{
		UE_LOG(LogTemp, Warning, TEXT("AInteractablePot::StartCooking - Already cooking, completed, or burnt."));
		return;
	}

	if (AddedIngredientIDs.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No ingredients in the pot to cook."));
		return;
	}

	// Check if ingredients match a known recipe
	FName PotentialResultID = CheckRecipeInternal(); // Check recipe *before* potentially requiring player knowledge

	if (PotentialResultID == NAME_None)
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid recipe based on ingredients. Cannot start cooking."));
		if (CookingFailSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, CookingFailSound, GetActorLocation());
		}
		// Optionally, clear ingredients here if invalid recipe means they are wasted
		// ClearIngredientsAndData(); // Uncomment if desired
		return; // Don't start cooking if recipe is invalid from the start
	}

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
				// Check if the player has the specific recipe item that unlocks the PotentialResultID
				// TArray<FSlotStruct> PlayerRecipes; // Removed GetItemsOfType call
				// PlayerInventory->GetItemsOfType(EInventoryItemType::EIT_Recipe, PlayerRecipes); // Removed GetItemsOfType call

				// Iterate through all inventory slots instead
				for (const FSlotStruct& Slot : PlayerInventory->InventorySlots)
				{
					// Check if the slot contains a Recipe item and the ItemID is valid
					if (Slot.ItemType == EInventoryItemType::EIT_Recipe && !Slot.ItemID.RowName.IsNone())
					{
						const FString ContextString(TEXT("Checking Recipe Knowledge"));
						FInventoryItemStruct* RecipeItemData = ItemDataTable->FindRow<FInventoryItemStruct>(Slot.ItemID.RowName, ContextString);

						// Check if this recipe item unlocks the specific dish we are about to cook
						if (RecipeItemData && RecipeItemData->UnlocksRecipeID == PotentialResultID)
						{
							bPlayerKnowsRecipe = true;
							UE_LOG(LogTemp, Log, TEXT("Player knows recipe %s because they have item %s."), *PotentialResultID.ToString(), *Slot.ItemID.RowName.ToString());
							break; // Found the required recipe, no need to check further
						}
					}
				}

				if (!bPlayerKnowsRecipe)
				{
					UE_LOG(LogTemp, Log, TEXT("Player does not know the recipe for %s."), *PotentialResultID.ToString());
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
		CurrentCookedResultID = PotentialResultID; // Store the result ID for later
		UE_LOG(LogTemp, Log, TEXT("Starting to cook recipe for: %s"), *CurrentCookedResultID.ToString());

		// --- Set State and Start Timers ---
		bIsCooking = true;
		bIsCookingComplete = false;
		bIsBurnt = false;
		GetWorldTimerManager().ClearTimer(BurningTimerHandle); // Ensure burning timer is stopped
		CookingStartTime = GetWorld()->GetTimeSeconds();
		GetWorldTimerManager().SetTimer(CookingTimerHandle, this, &AInteractablePot::OnCookingComplete, CookingDuration, false);
		// --- End State and Timers ---

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

		NotifyWidgetUpdate(); // Update UI to show cooking state
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Player attempted to cook %s, but does not have the required recipe item."), *PotentialResultID.ToString());
		if (CookingFailSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, CookingFailSound, GetActorLocation());
		}
		// Optionally clear ingredients if attempting without knowing recipe wastes them
		// ClearIngredientsAndData(); // Uncomment if desired
	}
}

void AInteractablePot::OnCookingComplete()
{
	UE_LOG(LogTemp, Log, TEXT("AInteractablePot: Cooking complete for %s! Ready for collection."), *CurrentCookedResultID.ToString());

	// --- Update State ---
	bIsCooking = false;
	bIsCookingComplete = true;
	bIsBurnt = false; 
	GetWorldTimerManager().ClearTimer(CookingTimerHandle); 
	// --- End Update State ---

    // --- Finalize Cooked Material Appearance ---
	for (auto const& [MeshComp, MID] : IngredientMIDMap)
	{
		if (MID) MID->SetScalarParameterValue(CookingMaterialParamName, CookedMaterialParamValue);
	}
    // --- End Finalize Material ---

	// --- Deactivate Cooking Effects ---  // 효과 비활성화 로직 제거 또는 주석 처리
	// if (CookingSteamParticles && CookingSteamParticles->IsActive()) 
	// {
	// 	CookingSteamParticles->DeactivateSystem(); 
	// }
	// if (FireEffectComponent && FireEffectComponent->IsActive()) 
	// {
	// 	FireEffectComponent->Deactivate(); 
	// }
	// -------------------------------

	// --- Play Cooking Success Sound ---
	if (CookingSuccessSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, CookingSuccessSound, GetActorLocation());
	}
	// --- End Play Sound ---

	// --- Start Burning Timer ---
	if (CurrentCookedResultID != NAME_None && BurningDuration > 0.0f)
	{
		BurningStartTime = GetWorld()->GetTimeSeconds();
		GetWorldTimerManager().SetTimer(BurningTimerHandle, this, &AInteractablePot::OnBurningComplete, BurningDuration, false);
		UE_LOG(LogTemp, Log, TEXT("AInteractablePot: Starting burning timer (%.2f seconds)."), BurningDuration);
	} 
	else if (CurrentCookedResultID == NAME_None) 
	{
        UE_LOG(LogTemp, Error, TEXT("AInteractablePot::OnCookingComplete - Cooking finished but CurrentCookedResultID is None. This indicates an issue in StartCooking logic."));
        ClearIngredientsAndData(); 
        if (CookingFailSound) UGameplayStatics::PlaySoundAtLocation(this, CookingFailSound, GetActorLocation());
    } 
	else 
	{
		UE_LOG(LogTemp, Log, TEXT("AInteractablePot: Burning disabled (BurningDuration <= 0). Item will remain ready."));
	}
	// --- End Burning Timer ---

	NotifyWidgetUpdate(); 
}

// NEW FUNCTION: Called when the Burning Timer finishes
void AInteractablePot::OnBurningComplete()
{
	// Prevent running if already burnt or not in a valid state
	if (bIsBurnt || !bIsCookingComplete)
	{
		UE_LOG(LogTemp, Warning, TEXT("AInteractablePot::OnBurningComplete called in invalid state (Burnt: %d, Complete: %d)"), bIsBurnt, bIsCookingComplete);
		GetWorldTimerManager().ClearTimer(BurningTimerHandle); // Clear timer anyway
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("AInteractablePot: Item %s has burnt!"), *CurrentCookedResultID.ToString());

	// --- Update State ---
	bIsCooking = false; // Should already be false
	bIsCookingComplete = false; // No longer ready to collect
	bIsBurnt = true;
	GetWorldTimerManager().ClearTimer(BurningTimerHandle); // Clear burning timer
	GetWorldTimerManager().ClearTimer(CookingTimerHandle); // Clear cooking timer just in case
	// --- End Update State ---

	// --- Finalize Burnt Material Appearance ---
	for (auto const& [MeshComp, MID] : IngredientMIDMap)
	{
		if (MID) MID->SetScalarParameterValue(CookingMaterialParamName, BurntMaterialParamValue);
	}
	// --- End Finalize Material ---

	// --- Play Burnt Sound ---
	if (ItemBurntSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ItemBurntSound, GetActorLocation());
	}
	// --- End Play Sound ---

	// Clear everything as the item is now ruined
	ClearIngredientsAndData(false); // Pass false to not notify widget, as we do it below

	// Reset the result ID (Already done in ClearIngredientsAndData)
	// CurrentCookedResultID = NAME_None;

	NotifyWidgetUpdate(); // Update UI to show "Burnt" or empty state
}

// NEW FUNCTION: Called by player interaction when bIsCookingComplete is true
bool AInteractablePot::CollectCookedItem()
{
	if (!bIsCookingComplete || bIsBurnt)
	{
		UE_LOG(LogTemp, Warning, TEXT("AInteractablePot::CollectCookedItem - Cannot collect, item is not ready or is burnt. (Complete: %d, Burnt: %d)"), bIsCookingComplete, bIsBurnt);
		return false;
	}

	if (CurrentCookedResultID == NAME_None)
	{
		UE_LOG(LogTemp, Error, TEXT("AInteractablePot::CollectCookedItem - Cannot collect, CurrentCookedResultID is NAME_None even though cooking was complete. Clearing state."));
		ClearIngredientsAndData(); // Clear the invalid state
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("AInteractablePot: Player collecting cooked item: %s."), *CurrentCookedResultID.ToString());

	// --- Stop Burning Timer ---
	GetWorldTimerManager().ClearTimer(BurningTimerHandle);
	// --- End Stop Timer ---

	// --- Give Item to Player ---
	bool bAddedSuccessfully = false;
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (PlayerController)
	{
		AWarriorHeroCharacter* PlayerChar = Cast<AWarriorHeroCharacter>(PlayerController->GetPawn());
		if (PlayerChar)
		{
			UInventoryComponent* PlayerInventory = PlayerChar->GetInventoryComponent();
			if (PlayerInventory && ItemDataTable)
			{
				FSlotStruct ItemToAdd;
				const FString ContextString(TEXT("Finding Cooked Item Data for Collection"));
				FInventoryItemStruct* CookedItemData = ItemDataTable->FindRow<FInventoryItemStruct>(CurrentCookedResultID, ContextString, true);

				if (CookedItemData)
				{
					ItemToAdd.ItemID.RowName = CurrentCookedResultID;
					ItemToAdd.Quantity = 1; // Or potentially based on recipe output amount?
					ItemToAdd.ItemType = CookedItemData->ItemType;

					if (PlayerInventory->AddItem(ItemToAdd))
					{
						UE_LOG(LogTemp, Log, TEXT("AInteractablePot: Successfully added '%s' to player inventory."), *CurrentCookedResultID.ToString());
						bAddedSuccessfully = true;
						if (CollectItemSound)
						{
							UGameplayStatics::PlaySoundAtLocation(this, CollectItemSound, GetActorLocation());
						}
					}
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("AInteractablePot: Failed to add collected item '%s' to player inventory (Inventory full?)."), *CurrentCookedResultID.ToString());
						// Item remains in pot until collected successfully or burnt. Could add drop logic here instead.
						// Playing fail sound might be confusing here unless inventory full has its own feedback.
						// if (CookingFailSound) UGameplayStatics::PlaySoundAtLocation(this, CookingFailSound, GetActorLocation());
						return false; // Indicate collection failed (due to inventory space)
					}
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("AInteractablePot: Could not find item data for the cooked result ID '%s' in ItemDataTable during collection."), *CurrentCookedResultID.ToString());
					// Don't clear if item data is missing, potential data setup error
					return false; // Indicate collection failed (due to data error)
				}
			} else UE_LOG(LogTemp, Error, TEXT("AInteractablePot: Could not get InventoryComponent or ItemDataTable from PlayerCharacter during collection."));
		} else UE_LOG(LogTemp, Error, TEXT("AInteractablePot: Could not cast Player Pawn to AWarriorHeroCharacter during collection."));
	} else UE_LOG(LogTemp, Error, TEXT("AInteractablePot: Could not get PlayerController during collection."));
	// --- End Give Item ---

	// --- Clear Pot if Item Added Successfully ---
	if (bAddedSuccessfully)
	{
		ClearIngredientsAndData(false); // Don't notify widget yet
		// CurrentCookedResultID = NAME_None; // Already done in ClearIngredientsAndData
		// Reset state flags fully (Already done in ClearIngredientsAndData)
		// bIsCooking = false;
		// bIsCookingComplete = false;
		// bIsBurnt = false;
		NotifyWidgetUpdate(); // Notify widget AFTER clearing
		return true; // Indicate successful collection
	}
	// --- End Clear Pot ---

	return false; // Indicate collection failed for reasons other than inventory full (e.g., couldn't get player components)
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
		UE_LOG(LogTemp, Log, TEXT("AInteractablePot::CheckRecipeInternal - No ingredients added."));
		return NAME_None;
	}

	// Sort the ingredients currently in the pot for comparison
	TArray<FName> SortedPotIngredients = AddedIngredientIDs;
	SortedPotIngredients.Sort([](const FName& A, const FName& B) {
		return A.ToString() < B.ToString();
	});

	const TArray<FName> RowNames = RecipeDataTable->GetRowNames();
	for (const FName& RowName : RowNames)
	{
		FCookingRecipeStruct* RecipeRow = RecipeDataTable->FindRow<FCookingRecipeStruct>(RowName, TEXT("CheckRecipeInternal"));

		// Check if row exists and the number of ingredients match
		if (RecipeRow && RecipeRow->RequiredIngredients.Num() == SortedPotIngredients.Num() && RecipeRow->RequiredIngredients.Num() > 0)
		{
			// Sort the ingredients from the recipe row for comparison
			TArray<FName> SortedRecipeIngredients = RecipeRow->RequiredIngredients;
			SortedRecipeIngredients.Sort([](const FName& A, const FName& B) {
				return A.ToString() < B.ToString();
			});

			// Compare the sorted lists
			if (SortedPotIngredients == SortedRecipeIngredients)
			{
				UE_LOG(LogTemp, Log, TEXT("Recipe Match Found! Recipe Row: %s, Output Item: %s"), *RowName.ToString(), *RecipeRow->ResultItem.ToString());
				return RecipeRow->ResultItem; // Return the ID of the item produced by the recipe
			}
		}
		// Optional: Log if row lookup failed, but usually not necessary unless debugging DataTable issues
		// else if (!RecipeRow) {
		//     UE_LOG(LogTemp, Warning, TEXT("AInteractablePot::CheckRecipeInternal - Could not find row %s or cast failed."), *RowName.ToString());
		// }
	}

	UE_LOG(LogTemp, Log, TEXT("No matching recipe found for current ingredients."));
	return NAME_None; // Return NAME_None if no recipe matches
}

void AInteractablePot::NotifyWidgetUpdate()
{
	if (CookingWidgetRef) // Check TObjectPtr directly
	{
		UE_LOG(LogTemp, Log, TEXT("AInteractablePot: Notifying widget %s to update state."), *CookingWidgetRef->GetName());
		// Pass all necessary state information to the widget
		CookingWidgetRef->UpdateWidgetState(AddedIngredientIDs, bIsCooking, bIsCookingComplete, bIsBurnt, CurrentCookedResultID);
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

// Getters for current state (useful for UI or Interaction logic)
bool AInteractablePot::IsCooking() const { return bIsCooking; }
bool AInteractablePot::IsCookingComplete() const { return bIsCookingComplete; }
bool AInteractablePot::IsBurnt() const { return bIsBurnt; }

void AInteractablePot::SetCookingWidget(UCookingWidget* InWidget)
{
	CookingWidgetRef = InWidget;
}

UCookingWidget* AInteractablePot::GetCookingWidget() const
{
	return CookingWidgetRef.Get(); // Use Get() for TObjectPtr
}

// MODIFIED & RENAMED: Clears visual meshes, MIDs, ingredient data, stops timers, and resets state.
void AInteractablePot::ClearIngredientsAndData(bool bNotifyWidget /*= true*/)
{
	UE_LOG(LogTemp, Log, TEXT("AInteractablePot: Clearing ingredients and resetting state. NotifyWidget: %d"), bNotifyWidget);

	// Destroy visual meshes
	for (UStaticMeshComponent* MeshComp : IngredientMeshComponents)
	{
		if (MeshComp && !MeshComp->IsBeingDestroyed())
		{
			MeshComp->DestroyComponent();
		}
	}
	IngredientMeshComponents.Empty();
	IngredientMIDMap.Empty(); 

	AddedIngredientIDs.Empty();
	CurrentCookedResultID = NAME_None; 

	GetWorldTimerManager().ClearTimer(CookingTimerHandle);
	GetWorldTimerManager().ClearTimer(BurningTimerHandle);

	// --- Deactivate Cooking Effects on Clear --- // 추가된 로직
	if (CookingSteamParticles && CookingSteamParticles->IsActive())
	{
		CookingSteamParticles->DeactivateSystem();
		UE_LOG(LogTemp, Log, TEXT("AInteractablePot: Deactivating steam particles in ClearIngredientsAndData."));
	}
	if (FireEffectComponent && FireEffectComponent->IsActive())
	{
		FireEffectComponent->Deactivate();
		UE_LOG(LogTemp, Log, TEXT("AInteractablePot: Deactivating fire effect in ClearIngredientsAndData."));
	}
	// --- End Deactivate Cooking Effects ---

	bIsCooking = false;
	bIsCookingComplete = false;
	bIsBurnt = false;
	CookingStartTime = 0.0f;
    BurningStartTime = 0.0f;

	if (bNotifyWidget)
	{
		NotifyWidgetUpdate(); 
	}
}

// Remove or comment out the old ClearIngredientsVisually definition if it exists
// void AInteractablePot::ClearIngredientsVisually() { ... } 