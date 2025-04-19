// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"         // For UDataTable
#include "Inventory/SlotStruct.h" // Include FSlotStruct definition
#include "InventoryItemActor.generated.h"

// Forward declarations
class UStaticMeshComponent;
class UDataTable;

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

	// Called when an instance of this class is placed or updated in the editor
	virtual void OnConstruction(const FTransform& Transform) override;

	// The Static Mesh Component to display the item's mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* StaticMeshComponent;

	// The inventory slot data for this item
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory", meta = (ExposeOnSpawn = true))
	FSlotStruct Item; 

	// The Data Table containing item definitions
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Data")
	TSoftObjectPtr<UDataTable> InventoryDataTable; 

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Returns the item data stored in this actor
	UFUNCTION(BlueprintPure, Category = "Inventory")
	FSlotStruct GetItemData() const { return Item; }

	// Allows external objects to set the item data for this actor
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetItemData(const FSlotStruct& NewItem) { Item = NewItem; /* Potentially call RefreshWidgetDisplay or similar if this actor has UI */ }

};
