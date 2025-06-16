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
#include "Inventory/InvenItemEnum.h" // Include for EInventoryItemType
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
#include "Components/AudioComponent.h" // Make sure this is included, though already in .h it's good practice for .cpp if directly used for creation
#include "Cooking/CookingMethodBase.h" // Added for cooking methods
#include "Cooking/CookingMethodBoiling.h" // NEW: Include for boiling cooking method
#include "Cooking/GrillingMinigame.h" // NEW: Include for grilling minigame
#include "Cooking/FryingRhythmMinigame.h" // NEW: Include for frying minigame
#include "Cooking/TimerBasedCookingMinigame.h" // NEW: Include for timer-based minigame
#include "Cooking/RhythmCookingMinigame.h" // NEW: Include for rhythm minigame
#include "CookingCameraShake.h" // Include for cooking camera shake
#include "GameFramework/PlayerController.h" // Include for player controller access
#include "Audio/CookingAudioManager.h" // NEW: Include for cooking audio manager

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

	// --- Create Fire Audio Component ---
	FireAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("FireAudioComponent"));
	FireAudioComponent->SetupAttachment(RootComponent); // Or FirewoodMesh if preferred
	FireAudioComponent->bAutoActivate = false; // Don't play on spawn
	// --- End Fire Audio Component ---

	// --- Create Audio Manager Component ---
	AudioManager = CreateDefaultSubobject<UCookingAudioManager>(TEXT("AudioManager"));
	// --- End Audio Manager Component ---

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

	// --- NEW: Initialize Minigame System ---
	bUseMinigameSystem = true; // 기본적으로 미니게임 시스템 사용
	bMinigameAffectsQuality = true; // 미니게임 결과가 품질에 영향
	
	// 미니게임 등록은 RegisterMinigameClasses()에서 처리 (BeginPlay에서 호출됨)
	UE_LOG(LogTemp, Log, TEXT("AInteractablePot: Minigame system initialized"));
	// --- End Minigame System ---
}

// Called when the game starts or when spawned
void AInteractablePot::BeginPlay()
{
	Super::BeginPlay();

	InitializeCookingMethod(); // Initialize the cooking method

	// NEW: 미니게임 클래스들을 등록
	RegisterMinigameClasses();

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

	if (FireAudioComponent && FireLoopSound)
	{
		FireAudioComponent->SetSound(FireLoopSound);
	}
	else
	{
		if (!FireLoopSound)
		{
			UE_LOG(LogTemp, Warning, TEXT("AInteractablePot: FireLoopSound is not assigned in Blueprint defaults for %s."), *GetName());
		}
		if (!FireAudioComponent)
		{
			UE_LOG(LogTemp, Error, TEXT("AInteractablePot: FireAudioComponent is null in BeginPlay for %s."), *GetName());
		}
	}
}

void AInteractablePot::InitializeCookingMethod()
{
	if (DefaultCookingMethodClass)
	{
		CurrentCookingMethod = NewObject<UCookingMethodBase>(this, DefaultCookingMethodClass);
		if (CurrentCookingMethod)
		{
			UE_LOG(LogTemp, Log, TEXT("InteractablePot: Successfully initialized cooking method: %s"), *CurrentCookingMethod->GetClass()->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("InteractablePot: Failed to create instance of DefaultCookingMethodClass for %s."), *GetName());
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("InteractablePot: DefaultCookingMethodClass is not set for %s. No cooking method will be active."), *GetName());
	}
}

// Called every frame
void AInteractablePot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// NEW: Update active minigame
	if (CurrentMinigame && CurrentMinigame->IsGameActive())
	{
		CurrentMinigame->UpdateMinigame(DeltaTime);
	}

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

			// --- Trigger Camera Shake for Ingredient Addition ---
			if (IngredientAdditionCameraShakeClass)
			{
				APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
				if (PlayerController)
				{
					PlayerController->ClientStartCameraShake(IngredientAdditionCameraShakeClass);
					UE_LOG(LogTemp, Log, TEXT("AInteractablePot: Triggered ingredient addition camera shake"));
				}
			}
			// ---------------------------------------------------

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
		UE_LOG(LogTemp, Warning, TEXT("Pot is already cooking, has finished, or item is burnt."));
		return;
	}

	if (AddedIngredientIDs.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("No ingredients in the pot to cook."));
		if (CookingFailSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, CookingFailSound, GetActorLocation());
		}
		NotifyWidgetUpdate(); // Update UI to show no ingredients
		return;
	}

	if (!CurrentCookingMethod)
	{
		UE_LOG(LogTemp, Error, TEXT("StartCooking: No CurrentCookingMethod available for %s!"), *GetName());
		if (CookingFailSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, CookingFailSound, GetActorLocation());
		}
		ClearIngredientsAndData();
		return;
	}

	FName ResultItemID = NAME_None;
	float DeterminedCookingDuration = CookingDuration; // Default, will be overridden by method

	bool bCanCook = CurrentCookingMethod->ProcessIngredients(AddedIngredientIDs, RecipeDataTable, ItemDataTable, ResultItemID, DeterminedCookingDuration);

	if (bCanCook && ResultItemID != NAME_None)
	{
		// NEW: Check if player actually owns the recipe for this result item
		bool bPlayerOwnsRecipe = CheckPlayerOwnsRecipe(ResultItemID);
		if (!bPlayerOwnsRecipe)
		{
			UE_LOG(LogTemp, Warning, TEXT("Player does not own the recipe for %s. Cannot cook."), *ResultItemID.ToString());
			if (CookingFailSound)
			{
				UGameplayStatics::PlaySoundAtLocation(this, CookingFailSound, GetActorLocation());
			}
			// Don't clear ingredients - let player try again or get the recipe
			NotifyWidgetUpdate();
			return;
		}

		CurrentCookedResultID = ResultItemID;
		CookingDuration = DeterminedCookingDuration; // Update pot's cooking duration with the one from the method

		bIsCooking = true;
		bIsCookingComplete = false;
		bIsBurnt = false;
		CookingStartTime = GetWorld()->GetTimeSeconds(); // Record start time for material interpolation

		// NEW: Start minigame or traditional timing system
		if (bUseMinigameSystem)
		{
			// 미니게임 사용 시에는 일반 요리 타이머를 설정하지 않음
			// 미니게임이 끝나면 EndCookingMinigame에서 OnCookingComplete를 호출
			UE_LOG(LogTemp, Log, TEXT("Cooking started with MINIGAME for item: %s. Duration: %.2f seconds via %s"), *CurrentCookedResultID.ToString(), CookingDuration, *CurrentCookingMethod->GetCookingMethodName().ToString());
		}
		else
		{
			// 기존 시스템에서만 요리 타이머 설정
			GetWorldTimerManager().SetTimer(CookingTimerHandle, this, &AInteractablePot::OnCookingComplete, CookingDuration, false);
			UE_LOG(LogTemp, Log, TEXT("Cooking started with TIMER for item: %s. Duration: %.2f seconds via %s"), *CurrentCookedResultID.ToString(), CookingDuration, *CurrentCookingMethod->GetCookingMethodName().ToString());
		}
		
		if (StartCookingSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, StartCookingSound, GetActorLocation());
		}

		// Initialize MIDs for cooking visuals if not already done (should be done on add ingredient)
		// For now, we assume MIDs are ready from AddIngredient. The Tick function will handle interpolation.

		if (CookingSteamParticles)
		{
			CookingSteamParticles->Activate();
		}
		if (FireEffectComponent && FireNiagaraSystem) // Check if FireNiagaraSystem is assigned
		{
			FireEffectComponent->Activate();
			UE_LOG(LogTemp, Log, TEXT("AInteractablePot: Fire Effect Activated for cooking."));
		}
		if (FireAudioComponent && FireLoopSound) // Check if FireLoopSound is assigned
        {
            FireAudioComponent->Play();
			UE_LOG(LogTemp, Log, TEXT("AInteractablePot: Fire Audio Played for cooking."));
        }

		// NEW: Start minigame or traditional timing system
		if (bUseMinigameSystem)
		{
			StartCookingMinigame();
			// 미니게임 사용 시에는 일반 요리 타이머를 설정하지 않음
			// 미니게임이 끝나면 EndCookingMinigame에서 OnCookingComplete를 호출
		}
		else
		{
			// NEW: Start timing minigame events (기존 시스템)
			SuccessfulTimingEvents = 0;
			FailedTimingEvents = 0;
			if (TimingEventInterval > 0.0f && MaxTimingEvents > 0)
			{
				GetWorldTimerManager().SetTimer(TimingEventTriggerTimer, this, &AInteractablePot::TriggerTimingEvent, TimingEventInterval, false);
				UE_LOG(LogTemp, Log, TEXT("AInteractablePot: Started timing event system. First event in %.2f seconds"), TimingEventInterval);
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to start cooking. No valid recipe found by %s or method failed."), *CurrentCookingMethod->GetCookingMethodName().ToString());
		CurrentCookedResultID = NAME_None; // Ensure no leftover result ID
		if (CookingFailSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, CookingFailSound, GetActorLocation());
		}
		// Optionally clear ingredients if recipe fails, or leave them for player to try again/remove.
		// For now, let's clear them to signify a failed attempt that consumes items.
		ClearIngredientsAndData(); // This will also notify the widget
		return; // Explicit return to avoid issues after ClearIngredientsAndData
	}

	NotifyWidgetUpdate(); // Update UI
}

void AInteractablePot::OnCookingComplete()
{
	UE_LOG(LogTemp, Log, TEXT("AInteractablePot: Cooking complete for %s! Ready for collection."), *CurrentCookedResultID.ToString());

	// --- Update State ---
	bIsCooking = false;
	bIsCookingComplete = true;
	bIsBurnt = false; 
	GetWorldTimerManager().ClearTimer(CookingTimerHandle);
	GetWorldTimerManager().ClearTimer(TimingEventTriggerTimer); // Clear timing event timer
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
		
		// NEW: Reduce burning time based on failed timing events
		float ActualBurningDuration = BurningDuration;
		if (FailedTimingEvents > 0 && BurningTimeReduction > 0.0f)
		{
			float TotalReduction = FailedTimingEvents * BurningTimeReduction;
			ActualBurningDuration = FMath::Max(1.0f, BurningDuration - TotalReduction); // Minimum 1 second
			UE_LOG(LogTemp, Warning, TEXT("AInteractablePot: Burning time reduced by %.2f seconds due to %d failed timing events (%.2f -> %.2f seconds)"), 
				TotalReduction, FailedTimingEvents, BurningDuration, ActualBurningDuration);
		}
		
		GetWorldTimerManager().SetTimer(BurningTimerHandle, this, &AInteractablePot::OnBurningComplete, ActualBurningDuration, false);
		UE_LOG(LogTemp, Log, TEXT("AInteractablePot: Starting burning timer (%.2f seconds)."), ActualBurningDuration);
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
	GetWorldTimerManager().ClearTimer(TimingEventTriggerTimer); // Clear timing event timer

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

	// Stop fire sound
	if (FireAudioComponent && FireAudioComponent->IsPlaying())
	{
		FireAudioComponent->Stop();
		UE_LOG(LogTemp, Log, TEXT("AInteractablePot: Stopping fire sound in ClearIngredientsAndData."));
	}

	if (bNotifyWidget)
	{
		NotifyWidgetUpdate(); 
	}
}

// NEW: Checks if the player owns the recipe for the given result item ID
bool AInteractablePot::CheckPlayerOwnsRecipe(FName ResultItemID)
{
	if (ResultItemID == NAME_None || !ItemDataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("CheckPlayerOwnsRecipe: Invalid ResultItemID or ItemDataTable."));
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("CheckPlayerOwnsRecipe: Looking for recipe that unlocks result item: %s"), *ResultItemID.ToString());

	// Get the player character and inventory
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Error, TEXT("CheckPlayerOwnsRecipe: Could not get PlayerController."));
		return false;
	}

	AWarriorHeroCharacter* PlayerChar = Cast<AWarriorHeroCharacter>(PlayerController->GetPawn());
	if (!PlayerChar)
	{
		UE_LOG(LogTemp, Error, TEXT("CheckPlayerOwnsRecipe: Could not cast Player Pawn to AWarriorHeroCharacter."));
		return false;
	}

	UInventoryComponent* PlayerInventory = PlayerChar->GetInventoryComponent();
	if (!PlayerInventory)
	{
		UE_LOG(LogTemp, Error, TEXT("CheckPlayerOwnsRecipe: Could not get InventoryComponent from PlayerCharacter."));
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("CheckPlayerOwnsRecipe: Checking inventory slots. Total slots: %d"), PlayerInventory->InventorySlots.Num());

	// Search through all inventory slots for recipe items
	for (int32 SlotIndex = 0; SlotIndex < PlayerInventory->InventorySlots.Num(); SlotIndex++)
	{
		const FSlotStruct& Slot = PlayerInventory->InventorySlots[SlotIndex];
		
		// Skip empty slots
		if (Slot.ItemID.RowName.IsNone() || Slot.Quantity <= 0)
		{
			continue;
		}

		UE_LOG(LogTemp, Warning, TEXT("CheckPlayerOwnsRecipe: Slot %d contains: %s (Qty: %d, Type: %s)"), 
			SlotIndex, 
			*Slot.ItemID.RowName.ToString(), 
			Slot.Quantity,
			*UEnum::GetValueAsString(Slot.ItemType));

		// Check if this slot contains a recipe item
		if (Slot.ItemType == EInventoryItemType::EIT_Recipe)
		{
			UE_LOG(LogTemp, Warning, TEXT("CheckPlayerOwnsRecipe: Found recipe item: %s, checking its UnlocksRecipeID..."), *Slot.ItemID.RowName.ToString());
			
			// Get the recipe item data from the data table
			const FString ContextString(TEXT("Checking Recipe Item UnlocksRecipeID"));
			FInventoryItemStruct* RecipeItemData = ItemDataTable->FindRow<FInventoryItemStruct>(Slot.ItemID.RowName, ContextString, true);
			
			if (RecipeItemData)
			{
				UE_LOG(LogTemp, Warning, TEXT("CheckPlayerOwnsRecipe: Recipe item %s unlocks: %s"), *Slot.ItemID.RowName.ToString(), *RecipeItemData->UnlocksRecipeID.ToString());
				
				// Check if this recipe unlocks the desired result item
				if (RecipeItemData->UnlocksRecipeID == ResultItemID)
				{
					UE_LOG(LogTemp, Warning, TEXT("CheckPlayerOwnsRecipe: FOUND! Recipe item %s unlocks %s"), *Slot.ItemID.RowName.ToString(), *ResultItemID.ToString());
					return true;
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("CheckPlayerOwnsRecipe: Could not find ItemData for recipe item %s in ItemDataTable"), *Slot.ItemID.RowName.ToString());
			}
		}
	}
	
	UE_LOG(LogTemp, Warning, TEXT("CheckPlayerOwnsRecipe: Player does NOT own a recipe that unlocks %s"), *ResultItemID.ToString());
	return false;
}

// Remove or comment out the old ClearIngredientsVisually definition if it exists
// void AInteractablePot::ClearIngredientsVisually() { ... } 

// NEW: Timing minigame event functions
void AInteractablePot::TriggerTimingEvent()
{
	// Check if cooking is still active and we haven't exceeded max events
	if (!bIsCooking || SuccessfulTimingEvents + FailedTimingEvents >= MaxTimingEvents)
	{
		UE_LOG(LogTemp, Log, TEXT("AInteractablePot: Timing event trigger cancelled (Not cooking or max events reached)"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("AInteractablePot: Triggering timing event! (%d/%d events)"), 
		SuccessfulTimingEvents + FailedTimingEvents + 1, MaxTimingEvents);

	// Tell the widget to start a timing event
	if (CookingWidgetRef)
	{
		CookingWidgetRef->StartTimingEvent();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AInteractablePot: No widget available for timing event"));
		// Auto-fail the event if no widget
		OnTimingEventFailure();
	}

	// Schedule the next timing event if we haven't reached the max
	if (SuccessfulTimingEvents + FailedTimingEvents + 1 < MaxTimingEvents)
	{
		GetWorldTimerManager().SetTimer(TimingEventTriggerTimer, this, &AInteractablePot::TriggerTimingEvent, TimingEventInterval, false);
		UE_LOG(LogTemp, Log, TEXT("AInteractablePot: Scheduled next timing event in %.2f seconds"), TimingEventInterval);
	}
}

void AInteractablePot::OnTimingEventSuccess()
{
	UE_LOG(LogTemp, Log, TEXT("AInteractablePot: Timing event SUCCESS!"));
	SuccessfulTimingEvents++;

	// Optional: Add benefits for successful timing events
	// Could improve cooking quality, reduce cooking time, etc.
	
	// Play success sound if desired
	if (CookingSuccessSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, CookingSuccessSound, GetActorLocation());
	}

	// NEW: Trigger camera shake for timing event success
	if (TimingEventSuccessCameraShakeClass)
	{
		APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
		if (PlayerController)
		{
			PlayerController->ClientStartCameraShake(TimingEventSuccessCameraShakeClass);
			UE_LOG(LogTemp, Log, TEXT("AInteractablePot: Triggered timing event success camera shake"));
		}
	}
}

void AInteractablePot::OnTimingEventFailure()
{
	UE_LOG(LogTemp, Log, TEXT("AInteractablePot: Timing event FAILED!"));
	FailedTimingEvents++;

	// NEW: Apply penalty for failed timing events
	if (TimingEventFailurePenalty > 0.0f && CookingTimerHandle.IsValid())
	{
		// Get remaining time on cooking timer
		float RemainingTime = GetWorldTimerManager().GetTimerRemaining(CookingTimerHandle);
		if (RemainingTime > 0.0f)
		{
			// Clear current timer
			GetWorldTimerManager().ClearTimer(CookingTimerHandle);
			
			// Restart timer with additional penalty time
			float NewRemainingTime = RemainingTime + TimingEventFailurePenalty;
			GetWorldTimerManager().SetTimer(CookingTimerHandle, this, &AInteractablePot::OnCookingComplete, NewRemainingTime, false);
			
			UE_LOG(LogTemp, Warning, TEXT("AInteractablePot: Timing event failure! Cooking time extended by %.2f seconds (new remaining: %.2f)"), 
				TimingEventFailurePenalty, NewRemainingTime);
		}
	}
	
	// Play failure sound if desired
	if (CookingFailSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, CookingFailSound, GetActorLocation());
	}
}

// NEW: Minigame system functions
void AInteractablePot::StartCookingMinigame()
{
	if (CurrentMinigame)
	{
		UE_LOG(LogTemp, Warning, TEXT("AInteractablePot::StartCookingMinigame - Minigame already active"));
		return;
	}

	if (!CurrentCookingMethod)
	{
		UE_LOG(LogTemp, Error, TEXT("AInteractablePot::StartCookingMinigame - No cooking method available"));
		return;
	}

	// 요리법 이름을 가져옴
	FString CookingMethodName = CurrentCookingMethod->GetCookingMethodName().ToString();
	UE_LOG(LogTemp, Log, TEXT("AInteractablePot::StartCookingMinigame - Starting minigame for method: %s"), *CookingMethodName);
	
	// 사용 가능한 미니게임 클래스들 로그 출력
	UE_LOG(LogTemp, Log, TEXT("AInteractablePot::StartCookingMinigame - Available minigame classes: %d"), MinigameClasses.Num());
	for (const auto& Pair : MinigameClasses)
	{
		UE_LOG(LogTemp, Log, TEXT("  - %s"), *Pair.Key);
	}

	// 해당 요리법에 맞는 미니게임 클래스 찾기
	TSubclassOf<UCookingMinigameBase>* MinigameClass = MinigameClasses.Find(CookingMethodName);
	if (!MinigameClass || !(*MinigameClass))
	{
		// 기본 미니게임이나 기존 시스템으로 폴백
		UE_LOG(LogTemp, Warning, TEXT("AInteractablePot::StartCookingMinigame - No minigame found for method %s, trying Default"), *CookingMethodName);
		
		// 기본 미니게임 시도
		MinigameClass = MinigameClasses.Find(TEXT("Default"));
		if (!MinigameClass || !(*MinigameClass))
		{
			UE_LOG(LogTemp, Warning, TEXT("AInteractablePot::StartCookingMinigame - No default minigame found, falling back to traditional system"));
			
			// 기존 타이밍 시스템으로 전환
			SuccessfulTimingEvents = 0;
			FailedTimingEvents = 0;
			if (TimingEventInterval > 0.0f && MaxTimingEvents > 0)
			{
				GetWorldTimerManager().SetTimer(TimingEventTriggerTimer, this, &AInteractablePot::TriggerTimingEvent, TimingEventInterval, false);
			}
			return;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("AInteractablePot::StartCookingMinigame - Found minigame class: %s"), *(*MinigameClass)->GetName());

	// 미니게임 인스턴스 생성
	CurrentMinigame = NewObject<UCookingMinigameBase>(this, *MinigameClass);
	if (!CurrentMinigame)
	{
		UE_LOG(LogTemp, Error, TEXT("AInteractablePot::StartCookingMinigame - Failed to create minigame instance"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("AInteractablePot::StartCookingMinigame - Created minigame instance successfully"));

	// 미니게임 시작
	CurrentMinigame->StartMinigame(CookingWidgetRef.Get(), this);
	
	// FryingRhythmMinigame인 경우 메트로놈 사운드 설정
	if (UFryingRhythmMinigame* FryingMinigame = Cast<UFryingRhythmMinigame>(CurrentMinigame))
	{
		if (MetronomeSound.LoadSynchronous())
		{
			FryingMinigame->SetMetronomeSound(MetronomeSound.Get());
			UE_LOG(LogTemp, Log, TEXT("AInteractablePot::StartCookingMinigame - Set metronome sound for frying minigame"));
		}
		
		// 메트로놈 볼륨도 설정
		FryingMinigame->SetMetronomeVolume(MetronomeTickVolume, MetronomeLastTickVolume);
		UE_LOG(LogTemp, Log, TEXT("AInteractablePot::StartCookingMinigame - Set metronome volumes: %.1f, %.1f"), 
			   MetronomeTickVolume, MetronomeLastTickVolume);
	}
	
	// NEW: 오디오 매니저에게 요리 시작 알림
	if (AudioManager)
	{
		FString MinigameTypeName = (*MinigameClass)->GetName();
		// 새로운 GameMode 연동 메서드 사용
		AudioManager->SaveGameModeBackgroundMusicAndStartMinigame(MinigameTypeName);
		UE_LOG(LogTemp, Log, TEXT("AInteractablePot::StartCookingMinigame - Started cooking audio for %s"), *MinigameTypeName);
	}
	
	// 위젯에 미니게임 시작 알림
	if (CookingWidgetRef)
	{
		CookingWidgetRef->OnMinigameStarted(CurrentMinigame.Get());
		UE_LOG(LogTemp, Log, TEXT("AInteractablePot::StartCookingMinigame - Notified widget about minigame start"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AInteractablePot::StartCookingMinigame - No cooking widget available"));
	}

	UE_LOG(LogTemp, Log, TEXT("AInteractablePot::StartCookingMinigame - Minigame started successfully"));
}

void AInteractablePot::EndCookingMinigame(ECookingMinigameResult Result)
{
	if (!CurrentMinigame)
	{
		UE_LOG(LogTemp, Warning, TEXT("AInteractablePot::EndCookingMinigame - No active minigame"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("AInteractablePot::EndCookingMinigame - Minigame ended with result: %d"), (int32)Result);

	// 결과에 따른 처리
	if (bMinigameAffectsQuality)
	{
		switch (Result)
		{
		case ECookingMinigameResult::Perfect:
			UE_LOG(LogTemp, Log, TEXT("AInteractablePot::EndCookingMinigame - Perfect result! No burning risk."));
			// 완벽한 결과는 타지 않도록 함
			GetWorldTimerManager().ClearTimer(BurningTimerHandle);
			break;
		case ECookingMinigameResult::Good:
			UE_LOG(LogTemp, Log, TEXT("AInteractablePot::EndCookingMinigame - Good result! Extended burning time."));
			// 좋은 결과는 타는 시간을 늘림
			if (BurningDuration > 0.0f)
			{
				BurningDuration *= 1.5f;
			}
			break;
		case ECookingMinigameResult::Poor:
		case ECookingMinigameResult::Failed:
			UE_LOG(LogTemp, Log, TEXT("AInteractablePot::EndCookingMinigame - Poor/Failed result! Reduced burning time."));
			// 나쁜 결과는 타는 시간을 줄임
			if (BurningDuration > 0.0f)
			{
				BurningDuration *= 0.5f;
			}
			break;
		default:
			// Average는 기본값 유지
			break;
		}
	}

	// 위젯에 미니게임 종료 알림
	if (CookingWidgetRef)
	{
		CookingWidgetRef->OnMinigameEnded((int32)Result);
	}

	// NEW: 오디오 매니저에게 요리 종료 알림
	if (AudioManager)
	{
		// 새로운 GameMode 연동 메서드 사용
		AudioManager->RestoreGameModeBackgroundMusic();
		UE_LOG(LogTemp, Log, TEXT("AInteractablePot::EndCookingMinigame - Restored GameMode background music"));
	}

	// 미니게임 인스턴스 정리
	CurrentMinigame = nullptr;

	// 일반적인 요리 완료 처리 호출
	OnCookingComplete();
}

void AInteractablePot::RegisterMinigameClasses()
{
	UE_LOG(LogTemp, Log, TEXT("AInteractablePot::RegisterMinigameClasses - Registering available minigames"));

	// 굽기 미니게임 등록
	MinigameClasses.Add(TEXT("Grilling"), UGrillingMinigame::StaticClass());
	
	// 튀기기 타이머 기반 미니게임 등록 (새로운 게임)
	MinigameClasses.Add(TEXT("Frying"), UTimerBasedCookingMinigame::StaticClass());
	
	// 끓이기 리듬 미니게임 등록 (기존 원 축소 게임)
	MinigameClasses.Add(TEXT("Boiling"), UFryingRhythmMinigame::StaticClass());
	
	// 리듬 미니게임 등록 (기본값으로도 사용)
	MinigameClasses.Add(TEXT("Rhythm"), URhythmCookingMinigame::StaticClass());
	MinigameClasses.Add(TEXT("Default"), URhythmCookingMinigame::StaticClass());

	// 등록된 미니게임들 로그 출력
	UE_LOG(LogTemp, Log, TEXT("AInteractablePot::RegisterMinigameClasses - Registered %d minigame classes:"), MinigameClasses.Num());
	for (const auto& Pair : MinigameClasses)
	{
		UE_LOG(LogTemp, Log, TEXT("  - %s: %s"), *Pair.Key, *Pair.Value->GetName());
	}
}