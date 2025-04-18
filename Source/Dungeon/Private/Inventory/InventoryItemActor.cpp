// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/InventoryItemActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DataTable.h"
#include "Engine/StaticMesh.h"
#include "Inventory/InvenItemStruct.h" // For FInventoryItemStruct (Data Table Row Type)
#include "Inventory/SlotStruct.h"     // For FSlotStruct (Item Property Type)

// Sets default values
AInventoryItemActor::AInventoryItemActor()
{
 	// Set this actor to call Tick() every frame. Typically off for construction script logic.
	PrimaryActorTick.bCanEverTick = false; 

	// Create the Static Mesh Component and set as root
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	RootComponent = StaticMeshComponent;
}

// Called when the game starts or when spawned
void AInventoryItemActor::BeginPlay()
{
	Super::BeginPlay();
	// Logic moved to OnConstruction
}

// Called when an instance of this class is placed or updated in the editor
void AInventoryItemActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    if (!StaticMeshComponent) 
    {
        UE_LOG(LogTemp, Error, TEXT("AInventoryItemActor [%s]: OnConstruction: StaticMeshComponent is missing!"), *GetName());
        return;
    }

    // Default to clearing the mesh
    UStaticMesh* MeshToSet = nullptr;
    FName ItemRowName = Item.ItemID.RowName; // Get the selected row name

    // Attempt to load the main data table assigned to the actor
    UDataTable* ItemTable = InventoryDataTable.LoadSynchronous();

    if (ItemTable && ItemRowName.IsValid() && !ItemRowName.IsNone())
    {
        const FString ContextString = GetName() + TEXT(" Construction");
        FInventoryItemStruct* ItemData = ItemTable->FindRow<FInventoryItemStruct>(ItemRowName, ContextString);

        if (ItemData)
        {
            // Row found, try to load the mesh assigned in the data table row
            MeshToSet = ItemData->Mesh.LoadSynchronous();
            if (!MeshToSet)
            {
                // Log if mesh loading failed, but row was found
                UE_LOG(LogTemp, Warning, TEXT("AInventoryItemActor [%s]: Row '%s' found, but failed to load Mesh asset '%s' from DataTable '%s'. Check Mesh path in DT."), 
                    *GetName(), *ItemRowName.ToString(), *ItemData->Mesh.ToString(), *ItemTable->GetName());
            }
            else
            {
                 UE_LOG(LogTemp, Log, TEXT("AInventoryItemActor [%s]: Row '%s' found. Attempting to set mesh '%s'."), 
                    *GetName(), *ItemRowName.ToString(), *MeshToSet->GetName());
            }
        }
        else
        {
            // Log if the row name wasn't found in the specified table
            UE_LOG(LogTemp, Warning, TEXT("AInventoryItemActor [%s]: Row '%s' not found in DataTable '%s'. Check Row Name selection."), 
                *GetName(), *ItemRowName.ToString(), *ItemTable->GetName());
        }
    }
    else if (!ItemRowName.IsValid() || ItemRowName.IsNone())
    {
         // Log if no valid Row Name is selected in the ItemID handle
         UE_LOG(LogTemp, Log, TEXT("AInventoryItemActor [%s]: No valid ItemID.RowName selected in Details panel."), *GetName()); 
    }
    else // !ItemTable
    {
         // Log if the main InventoryDataTable property is not set on the actor
         UE_LOG(LogTemp, Warning, TEXT("AInventoryItemActor [%s]: InventoryDataTable property not set in Details panel."), *GetName());
    }

    // Apply the result (either the loaded mesh or nullptr)
    // Only update if the mesh is actually different to prevent unnecessary updates
    if (StaticMeshComponent->GetStaticMesh() != MeshToSet) 
    {
       UE_LOG(LogTemp, Log, TEXT("AInventoryItemActor [%s]: Setting Static Mesh to '%s'. Previous was '%s'."), 
            *GetName(), 
            MeshToSet ? *MeshToSet->GetName() : TEXT("nullptr"), 
            StaticMeshComponent->GetStaticMesh() ? *StaticMeshComponent->GetStaticMesh()->GetName() : TEXT("nullptr"));
       StaticMeshComponent->SetStaticMesh(MeshToSet);
    }
}

// Called every frame
void AInventoryItemActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
