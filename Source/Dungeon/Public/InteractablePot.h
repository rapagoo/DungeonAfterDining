#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h" // Assuming InteractableTable inherits from AActor, or adjust base class if needed
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h" // Include for Interaction Box
#include "Particles/ParticleSystemComponent.h"
#include "Engine/DataTable.h" // Include for UDataTable
#include "Interactables/InteractableTable.h"
#include "Inventory/CookingRecipeStruct.h" // Include for FCookingRecipeStruct (Adjust path as needed)
#include "Inventory/InvenItemStruct.h" // Include for item definition lookup
#include "Particles/ParticleSystem.h" // Include for UParticleSystem
#include "Materials/MaterialInstanceDynamic.h" // Include for MID
#include "ProceduralMeshComponent.h" // Include for ProceduralMeshComponent (Ensure this path is correct)
#include "NiagaraComponent.h" // Include for Niagara Component
#include "NiagaraSystem.h"    // Include for Niagara System asset
#include "Components/AudioComponent.h"
#include "Cooking/CookingMethodBase.h" // Added for cooking methods
#include "Camera/CameraShakeBase.h" // Include for camera shake
#include "InteractablePot.generated.h"

// Forward declaration for CookingWidget if needed for delegate binding or direct reference
class UCookingWidget;

// Forward declaration for SoundBase
class USoundBase;

// Forward declaration for MaterialInstanceDynamic
class UMaterialInstanceDynamic;

// Forward declaration for CookingCameraShake
class UCookingCameraShake;

UCLASS()
class DUNGEON_API AInteractablePot : public AInteractableTable // Inherit from AInteractableTable or AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AInteractablePot();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Function called by the character to add an ingredient to the pot
	// Returns true if ingredient was successfully added
	UFUNCTION(BlueprintCallable, Category = "Cooking")
	bool AddIngredient(FName IngredientID);

	// Function called by the Cooking Widget or Interaction to start the cooking process
	UFUNCTION(BlueprintCallable, Category = "Cooking")
	void StartCooking();

	// NEW: Function called by Player Interaction to collect the finished item
	UFUNCTION(BlueprintCallable, Category = "Cooking")
	bool CollectCookedItem();

	// Notifies the CookingWidget to update its display
	void NotifyWidgetUpdate();

	// --- Public Getters/Setters ---
	UFUNCTION(BlueprintPure, Category = "Cooking")
	const TArray<FName>& GetAddedIngredientIDs() const;

	// --- State Getters ---
	UFUNCTION(BlueprintPure, Category = "Cooking|State")
	bool IsCooking() const;

	UFUNCTION(BlueprintPure, Category = "Cooking|State")
	bool IsCookingComplete() const; // Ready to be collected

	UFUNCTION(BlueprintPure, Category = "Cooking|State")
	bool IsBurnt() const;
	// --- End State Getters ---

	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetCookingWidget(UCookingWidget* InWidget);

	UFUNCTION(BlueprintPure, Category = "UI")
	UCookingWidget* GetCookingWidget() const;

	/** Returns the ItemDataTable for read-only access. */
	UFUNCTION(BlueprintPure, Category = "Cooking|Data") // Blueprint에서도 필요하면 호출 가능하도록 설정
	UDataTable* GetItemDataTable() const { return ItemDataTable.Get(); }

	// NEW: Checks if the player owns the recipe for the given result item ID
	bool CheckPlayerOwnsRecipe(FName ResultItemID);

	// --- Timing Minigame Functions ---
	// NEW: Handle successful timing event
	UFUNCTION(BlueprintCallable, Category = "Cooking|Minigame")
	void OnTimingEventSuccess();

	// NEW: Handle failed timing event
	UFUNCTION(BlueprintCallable, Category = "Cooking|Minigame")
	void OnTimingEventFailure();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// --- Core Components ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PotMesh; // Use TObjectPtr

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> FirewoodMesh; // Use TObjectPtr

	// Interaction volume for the player to detect the pot
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> InteractionVolume; // Use TObjectPtr

	// Overlap volume to detect ingredients (Inside the pot) - Deprecated for adding, might be useful for visuals?
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> IngredientDetectionVolume; // Use TObjectPtr

	// --- Visual Effects ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraComponent> FireEffectComponent; // Use TObjectPtr

	// Niagara system asset to use for the fire effect
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Visuals")
	TObjectPtr<UNiagaraSystem> FireNiagaraSystem; // Use TObjectPtr

	// NEW: Scale for the fire Niagara effect
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Visuals", meta = (ToolTip = "The scale to apply to the fire Niagara effect."))
	FVector FireEffectScale = FVector(1.0f);

	// Particle effect for cooking (steam, bubbles, etc.) - Using Cascade for this one for now
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UParticleSystemComponent> CookingSteamParticles; // Use TObjectPtr

	// --- Data Tables ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Data")
	TObjectPtr<UDataTable> RecipeDataTable; // Use TObjectPtr

	// Data Table containing item definitions (for mesh lookup, recipe checks)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Data", meta = (RequiredAssetDataTags = "RowStructure=/Script/Dungeon.InventoryItemStruct")) // Corrected RowStructure path
	TObjectPtr<UDataTable> ItemDataTable; // Use TObjectPtr

	// --- Cooking State ---
	// List of ingredient IDs currently in the pot
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cooking|State")
	TArray<FName> AddedIngredientIDs;

	// Array to hold references to spawned ingredient meshes inside the pot
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> IngredientMeshComponents; // Use TObjectPtr for safety

	// NEW: Map to store MIDs for each ingredient mesh component
	UPROPERTY()
	TMap<TObjectPtr<UStaticMeshComponent>, TObjectPtr<UMaterialInstanceDynamic>> IngredientMIDMap;

	// Reference to the Cooking Widget (Managed via SetCookingWidget/GetCookingWidget)
	UPROPERTY() // Category removed as it's not exposed
	TObjectPtr<UCookingWidget> CookingWidgetRef; // Use TObjectPtr

	// --- Timers ---
	// Timer handle for cooking duration
	FTimerHandle CookingTimerHandle;

	// NEW: Timer handle for burning duration (after cooking completes)
	FTimerHandle BurningTimerHandle;

	// --- Cooking Parameters & State ---
	// Duration of the cooking process
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Timing")
	float CookingDuration = 5.0f;

	// NEW: Duration after cooking before the item burns
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Timing")
	float BurningDuration = 10.0f;

	// NEW: Is the pot currently in the cooking process?
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Cooking|State")
	bool bIsCooking = false;

	// NEW: Has the cooking process completed (item is ready)?
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Cooking|State")
	bool bIsCookingComplete = false;

	// NEW: Has the item burnt after being left too long?
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Cooking|State")
	bool bIsBurnt = false;

	// NEW: Stores the Item ID of the successfully cooked item (set when cooking starts)
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Cooking|State")
	FName CurrentCookedResultID = NAME_None;

	// NEW: Time when cooking started (used for material interpolation)
	float CookingStartTime = 0.0f;
	// NEW: Time when burning started (used for material interpolation)
	float BurningStartTime = 0.0f;

	// --- Cooking Method ---
	UPROPERTY(EditDefaultsOnly, Category = "Cooking|Setup", meta = (DisplayName = "Default Cooking Method"))
	TSubclassOf<UCookingMethodBase> DefaultCookingMethodClass;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Cooking|State", meta = (ToolTip = "The active cooking method instance."))
	TObjectPtr<UCookingMethodBase> CurrentCookingMethod;

	// --- Ingredient Spawning Parameters ---
	// Scale to apply to spawned ingredient meshes relative to the pot
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Spawning")
	FVector IngredientSpawnScale = FVector(0.2f);

	// Base Z offset to apply to spawned ingredient meshes relative to the pot's pivot
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Spawning")
	float IngredientSpawnZOffset = 10.0f;

	// Range (+/-) for the random X/Y offset applied to spawned ingredient meshes
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Spawning")
	float IngredientSpawnXYOffsetRange = 15.0f;

	// --- Material Parameters ---
	// NEW: Name of the Scalar Parameter in the ingredient material to control the 'cooked' look
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Material")
	FName CookingMaterialParamName = FName("CookAmount");

	// NEW: Target value for the material parameter when fully cooked
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Material")
	float CookedMaterialParamValue = 1.0f;

	// NEW: Target value for the material parameter when burnt
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Material")
	float BurntMaterialParamValue = 2.0f;

	// NEW: Initial value for the material parameter
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Material")
	float InitialMaterialParamValue = 0.0f;

	// --- Sound Effects ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Sound")
	TObjectPtr<USoundBase> StartCookingSound; // Use TObjectPtr

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Sound")
	TObjectPtr<USoundBase> CookingSuccessSound; // Use TObjectPtr (Played when OnCookingComplete is called)

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Sound")
	TObjectPtr<USoundBase> CookingFailSound; // Use TObjectPtr (Used for invalid recipe or unknown recipe attempt)

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Sound")
	TObjectPtr<USoundBase> AddIngredientSound; // Use TObjectPtr

	// NEW: Sound played when the player collects the item
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Sound")
	TObjectPtr<USoundBase> CollectItemSound; // Use TObjectPtr

	// NEW: Sound played when the item burns
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Sound")
	TObjectPtr<USoundBase> ItemBurntSound; // Use TObjectPtr

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Sound")
	TObjectPtr<USoundBase> FireLoopSound;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components") 
	TObjectPtr<UAudioComponent> FireAudioComponent;

	// --- Camera Shake Effects ---
	// Camera shake class for ingredient addition
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|CameraShake")
	TSubclassOf<UCameraShakeBase> IngredientAdditionCameraShakeClass;

	// Camera shake class for cooking start
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|CameraShake")
	TSubclassOf<UCameraShakeBase> CookingStartCameraShakeClass;

	// NEW: Camera shake class for timing event success
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|CameraShake")
	TSubclassOf<UCameraShakeBase> TimingEventSuccessCameraShakeClass;

	// --- Protected Functions ---
	// Function called when an ingredient actor overlaps the detection volume (DEPRECATED for adding items)
	UFUNCTION()
	void OnIngredientOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// Called when cooking timer finishes
	UFUNCTION()
	void OnCookingComplete();

	// NEW: Called when the burning timer finishes
	UFUNCTION()
	void OnBurningComplete();

	// DEPRECATED: Old recipe checking logic. Functionality moved to CookingMethod classes.
	// FName CheckRecipeInternal(); 

	// RENAMED & MODIFIED: Clears visual meshes, MIDs, ingredient data, stops timers, and resets state.
	// Parameter controls whether to notify the widget (useful to avoid redundant calls)
	void ClearIngredientsAndData(bool bNotifyWidget = true);

	// Initializes the CurrentCookingMethod based on DefaultCookingMethodClass
	void InitializeCookingMethod();

	// NEW: Trigger a timing event during cooking (Internal function)
	void TriggerTimingEvent();

private:
	// --- Timing Minigame Variables ---
	// NEW: Timer for when to trigger timing events during cooking
	FTimerHandle TimingEventTriggerTimer;

	// NEW: Number of successful timing events this cooking session
	UPROPERTY()
	int32 SuccessfulTimingEvents = 0;

	// NEW: Number of failed timing events this cooking session
	UPROPERTY()
	int32 FailedTimingEvents = 0;

	// NEW: How often timing events should trigger during cooking (in seconds)
	UPROPERTY(EditDefaultsOnly, Category = "Cooking|Minigame", meta = (ClampMin = "1.0", ClampMax = "10.0"))
	float TimingEventInterval = 3.0f;

	// NEW: How many timing events should happen during one cooking session
	UPROPERTY(EditDefaultsOnly, Category = "Cooking|Minigame", meta = (ClampMin = "1", ClampMax = "5"))
	int32 MaxTimingEvents = 2;

	// NEW: Time penalty for each failed timing event (in seconds)
	UPROPERTY(EditDefaultsOnly, Category = "Cooking|Minigame", meta = (ClampMin = "0.0", ClampMax = "5.0"))
	float TimingEventFailurePenalty = 1.0f;

	// NEW: Burning time reduction for each failed timing event (in seconds)
	UPROPERTY(EditDefaultsOnly, Category = "Cooking|Minigame", meta = (ClampMin = "0.0", ClampMax = "3.0"))
	float BurningTimeReduction = 0.5f;

}; 