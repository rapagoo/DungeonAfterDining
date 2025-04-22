// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"         // For UDataTable
//#include "Components/ProceduralMeshComponent.h" // Standard path commented out
#include "ProceduralMeshComponent.h" // Direct path as per user preference
#include "Components/StaticMeshComponent.h" // Needed for TemporarySourceMeshComponent
#include "Inventory/SlotStruct.h" // Include FSlotStruct definition
#include "Inventory/InvenItemStruct.h" // Include the actual DataTable Row Struct definition
#include "InventoryItemActor.generated.h"

// Forward declarations
class UDataTable;
//class UStaticMeshComponent; // Replaced by full include
//class UProceduralMeshComponent; // Replaced by full include
struct FInventoryItemStruct; // Forward declare the struct type

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

	// The Procedural Mesh Component to display the item's mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UProceduralMeshComponent* ProceduralMeshComponent;

	// Temporary Static Mesh Component to act as a source for copying to Procedural Mesh
	// Not visible or interactable, just holds the mesh data temporarily.
	UPROPERTY(Transient) // Don't save this temporary component
	UStaticMeshComponent* TemporarySourceMeshComponent;

	// The inventory slot data for this item
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory", meta = (ExposeOnSpawn = true))
	FSlotStruct Item;

	// The Data Table containing item definitions
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Data")
	TSoftObjectPtr<UDataTable> InventoryDataTable;

	// Pointer to the ItemData struct, looked up from ItemID. Not a UPROPERTY.
	FInventoryItemStruct* ItemData; // Removed const, needs to be non-const if modified by lookup? Check cpp.

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

	// Allows setting the static mesh (primarily for internal use or specific cases)
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetStaticMesh(UStaticMesh* NewStaticMesh);

	// Function to request physics enable on next tick
	void RequestEnablePhysics();

};
