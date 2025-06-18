// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"         // For UDataTable
#include "Components/SceneComponent.h" // Include for USceneComponent
#include "Components/StaticMeshComponent.h" // Needed for StaticMeshComponent
#include "ProceduralMeshComponent.h" // Use direct include as per standard
#include "Inventory/SlotStruct.h" // Include FSlotStruct definition
#include "Inventory/InvenItemStruct.h" // Include the actual DataTable Row Struct definition
#include "Inventory/InvenItemEnum.h" // Include for ECuttingStyle enum
#include "InventoryItemActor.generated.h"

// Forward declarations
// class UDataTable; // Included
// class UStaticMeshComponent; // Included
// class UProceduralMeshComponent; // Included
struct FInventoryItemStruct; // Forward declare the struct type
class UMaterialInterface;

UCLASS()
class DUNGEON_API AInventoryItemActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AInventoryItemActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called after the actor has been spawned
	virtual void PostActorCreated() override;

	// Called when an instance of this class is placed or updated in the editor
	virtual void OnConstruction(const FTransform& Transform) override;

	// Default root component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* DefaultSceneRoot;

	// Original static mesh for the item before slicing
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* StaticMeshComponent;

	// Procedural mesh used for slicing (becomes one half after slicing)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UProceduralMeshComponent* ProceduralMeshComponent;

	// NOTE: OtherHalfProceduralMeshComponent removed - each slice now creates independent actors

	// REMOVED: Temporary Static Mesh Component (Now using StaticMeshComponent directly as source)
	// UPROPERTY(Transient) 
	// UStaticMeshComponent* TemporarySourceMeshComponent;

	// The inventory slot data for this item
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory", meta = (ExposeOnSpawn = true))
	FSlotStruct Item;

	// The Data Table containing item definitions
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Data")
	TSoftObjectPtr<UDataTable> InventoryDataTable;

	// Pointer to the ItemData struct, looked up from ItemID. Not a UPROPERTY.
	FInventoryItemStruct* ItemData;

	// Flag to indicate if the item has been cut (for new cutting system)
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Cutting", meta = (AllowPrivateAccess = "true"))
	bool bIsCut = false;

	// Updates the procedural mesh component based on the Item data
	void UpdateMeshFromData();

	// Event called after ItemData has been set and mesh updated
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
	void OnItemDataUpdated(); // Restored event

	// Flag to enable physics on the next tick after dropping
	bool bEnablePhysicsRequested = false;

public:
	// NEW: Button-based cutting system functions
	// Function to cut item with specific cutting style
	UFUNCTION(BlueprintCallable, Category = "Cutting")
	bool CutItemWithStyle(ECuttingStyle CuttingStyle);

	// Function to check if item can be cut with specific style
	UFUNCTION(BlueprintPure, Category = "Cutting")
	bool CanBeCutWithStyle(ECuttingStyle CuttingStyle) const;

	// Function to get available cutting styles for this item
	UFUNCTION(BlueprintPure, Category = "Cutting")
	TArray<ECuttingStyle> GetAvailableCuttingStyles() const;

	// Current cutting style of this item
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Cutting")
	ECuttingStyle CurrentCuttingStyle = ECuttingStyle::ECS_None;

protected:

	// NEW: Cutting animation system
	// Timer handle for cutting animation
	FTimerHandle CuttingAnimationTimer;
	
	// Current cutting animation state
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Cutting Animation")
	bool bIsCuttingInProgress = false;
	
	// Number of cutting sounds played
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Cutting Animation")
	int32 CuttingSoundCount = 0;
	
	// Target cutting style for current animation
	ECuttingStyle PendingCuttingStyle = ECuttingStyle::ECS_None;
	
	// Cutting sound effect (assign in Blueprint or DataTable)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cutting Animation")
	class USoundBase* CuttingSound;
	
	// Cutting effect (particles/niagara - assign in Blueprint)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cutting Animation")
	class UParticleSystem* CuttingEffect;
	
	// Niagara cutting effect alternative
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cutting Animation")
	class UNiagaraSystem* CuttingNiagaraEffect;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Returns the item data stored in this actor
	UFUNCTION(BlueprintPure, Category = "Inventory")
	FSlotStruct GetItemData() const { return Item; }

	// Allows external objects to set the item data for this actor and update visuals
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetItemData(const FSlotStruct& NewItem);

	// Function to request physics enable on next tick
	void RequestEnablePhysics();

	// Returns whether the item has been cut
	UFUNCTION(BlueprintPure, Category = "Cutting")
	bool IsCut() const { return bIsCut; }

#if WITH_EDITOR
	// Called when properties are changed in the editor AFTER construction
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent);
#endif

private:
	// Internal function to play cutting animation step
	void PlayCuttingAnimationStep();
	
	// Internal function to complete cutting animation
	void CompleteCuttingAnimation();

};
