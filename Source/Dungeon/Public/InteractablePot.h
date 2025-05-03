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
#include "Inventory/InvenItemStruct.h" // Include for InventoryItemStruct
#include "InteractablePot.generated.h"

// Forward declaration for CookingWidget if needed for delegate binding or direct reference
class UCookingWidget;

// Forward declaration for SoundBase
class USoundBase;

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

	// Function called by the Cooking Widget to start the cooking process
	UFUNCTION(BlueprintCallable, Category = "Cooking")
	void StartCooking();

	// Notifies the CookingWidget to update its display
	void NotifyWidgetUpdate();

	// --- Public Getters/Setters ---
	UFUNCTION(BlueprintPure, Category = "Cooking")
	const TArray<FName>& GetAddedIngredientIDs() const;

	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetCookingWidget(UCookingWidget* InWidget);

	UFUNCTION(BlueprintPure, Category = "UI")
	UCookingWidget* GetCookingWidget() const;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Pot mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* PotMesh;

	// Overlap volume to detect ingredients (Inside the pot)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* IngredientDetectionVolume;

	// Interaction volume for the player to detect the pot
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* InteractionVolume;

	// Particle effect for cooking (steam, bubbles, etc.)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UParticleSystemComponent* CookingEffectParticles;

	// The Data Table containing cooking recipes
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Data")
	UDataTable* RecipeDataTable;

	// Data Table containing item definitions (for mesh lookup)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Data", meta = (RequiredAssetDataTags = "RowStructure=/Script/Dungeon.InventoryItemStruct")) // Corrected RowStructure path
	UDataTable* ItemDataTable;

	// List of ingredient IDs currently in the pot
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cooking") // Keep VisibleAnywhere for debugging?
	TArray<FName> AddedIngredientIDs;

	// Array to hold references to spawned ingredient meshes inside the pot
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> IngredientMeshComponents; // Use TObjectPtr for safety

	// Scale to apply to spawned ingredient meshes relative to the pot
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking")
	FVector IngredientSpawnScale = FVector(0.2f);

	// Base Z offset to apply to spawned ingredient meshes relative to the pot's pivot
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking")
	float IngredientSpawnZOffset = 10.0f; // Increased default value

	// Range (+/-) for the random X/Y offset applied to spawned ingredient meshes
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking")
	float IngredientSpawnXYOffsetRange = 15.0f; // Increased default value

	// Reference to the Cooking Widget (Managed via SetCookingWidget/GetCookingWidget)
	UPROPERTY() // Category removed as it's not exposed
	TObjectPtr<UCookingWidget> CookingWidgetRef; // Use TObjectPtr

	// Timer handle for cooking duration
	FTimerHandle CookingTimerHandle;

	// Duration of the cooking process
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking")
	float CookingDuration = 5.0f;

	// Function called when an ingredient actor overlaps the detection volume
	UFUNCTION()
	void OnIngredientOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// Called when cooking timer finishes
	UFUNCTION()
	void OnCookingComplete();

	// Checks if the current ingredients match a recipe (Implementation needed)
	// Returns the FName ID of the resulting item, or NAME_None if no valid recipe.
	FName CheckRecipeInternal();

	// Destroys the visual representations (StaticMeshComponents) of the added ingredients
	void ClearIngredientsVisually();

	// --- Sound Effects --- 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Sound")
	USoundBase* StartCookingSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Sound")
	USoundBase* CookingSuccessSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Sound")
	USoundBase* CookingFailSound; // Used for invalid recipe or unknown recipe

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking|Sound")
	USoundBase* AddIngredientSound;

}; 