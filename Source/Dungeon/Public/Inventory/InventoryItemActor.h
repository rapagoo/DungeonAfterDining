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

	// Procedural mesh for the other half created after slicing. Initially null.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UProceduralMeshComponent> OtherHalfProceduralMeshComponent; // Use TObjectPtr for safety

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

	// Material to use for the cut surface
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Slicing", meta = (AllowPrivateAccess = "true"))
	UMaterialInterface* CapMaterial;

	// Flag to indicate if the item has been sliced into a procedural mesh
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Slicing", meta = (AllowPrivateAccess = "true"))
	bool bIsSliced = false;

	// Updates the procedural mesh component based on the Item data
	void UpdateMeshFromData();

	// Event called after ItemData has been set and mesh updated
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
	void OnItemDataUpdated(); // Restored event

	// Flag to enable physics on the next tick after dropping
	bool bEnablePhysicsRequested = false;

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

	// Function called to slice this item
	UFUNCTION(BlueprintCallable, Category = "Slicing")
	virtual void SliceItem(const FVector& PlanePosition, const FVector& PlaneNormal);

	// Returns whether the item has been sliced
	UFUNCTION(BlueprintPure, Category = "Slicing")
	bool IsSliced() const { return bIsSliced; }

	// Called when properties are changed in the editor AFTER construction
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

};
