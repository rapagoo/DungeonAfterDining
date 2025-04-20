// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/InventoryItemActor.h"
// #include "Components/StaticMeshComponent.h" // Replaced
//#include "Components/ProceduralMeshComponent.h" // Reverting back to standard include path
#include "ProceduralMeshComponent.h"
#include "Engine/DataTable.h"
#include "Engine/StaticMesh.h"
#include "Inventory/InvenItemStruct.h" // For FInventoryItemStruct (Data Table Row Type)
#include "Inventory/SlotStruct.h"     // For FSlotStruct (Item Property Type)
#include "Engine/CollisionProfile.h" // Correct include for UCollisionProfile
#include "KismetProceduralMeshLibrary.h" // Needed for CopyProceduralMeshFromStaticMesh

// Sets default values
AInventoryItemActor::AInventoryItemActor()
{
 	// Set this actor to call Tick() every frame. Typically off for construction script logic.
	PrimaryActorTick.bCanEverTick = false; 

	// Create the Procedural Mesh Component and set as root
	ProceduralMeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMeshComponent"));
	RootComponent = ProceduralMeshComponent;

	// Default physics/collision for placeable items (usually no physics initially)
	ProceduralMeshComponent->SetSimulatePhysics(false); // Don't simulate physics when placed
	ProceduralMeshComponent->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName); // Allow interaction/trace
	ProceduralMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); 
}

// Called when the game starts or when spawned
void AInventoryItemActor::BeginPlay()
{
	Super::BeginPlay();
	// Initial mesh update is handled by OnConstruction or SetItemData
}

// Called when an instance of this class is placed or updated in the editor
void AInventoryItemActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    UpdateMeshFromData(); // Call the new update function
}

void AInventoryItemActor::UpdateMeshFromData() // Renamed from UpdateStaticMesh
{
	if (!ProceduralMeshComponent) 
    {
        UE_LOG(LogTemp, Error, TEXT("AInventoryItemActor [%s]: UpdateMeshFromData: ProceduralMeshComponent is missing!"), *GetName());
        return;
    }

	if (!ItemData)
	{
        UE_LOG(LogTemp, Warning, TEXT("AInventoryItemActor [%s]: UpdateMeshFromData: ItemData is null. Clearing mesh."), *GetName());
		ProceduralMeshComponent->ClearAllMeshSections(); // Clear mesh if no data
        return;
    }

    UStaticMesh* MeshToSet = nullptr;
	if (ItemData->Mesh.IsNull())
    {
        UE_LOG(LogTemp, Warning, TEXT("AInventoryItemActor [%s]: UpdateMeshFromData: Mesh in ItemData is null. Clearing mesh."), *GetName());
		ProceduralMeshComponent->ClearAllMeshSections(); // Clear mesh if reference is null
		return;
    }
    else
    {
        // Synchronously load the static mesh
        MeshToSet = ItemData->Mesh.LoadSynchronous();
        if (!MeshToSet)
        {
            UE_LOG(LogTemp, Error, TEXT("AInventoryItemActor [%s]: UpdateMeshFromData: Failed to load Static Mesh from reference '%s'. Clearing mesh."), 
                *GetName(), 
                *ItemData->Mesh.ToString());
			ProceduralMeshComponent->ClearAllMeshSections(); // Clear mesh if load fails
			return;
        }
    }

	// Copy the static mesh geometry to the procedural mesh component
	UE_LOG(LogTemp, Log, TEXT("AInventoryItemActor [%s]: Copying mesh data from Static Mesh '%s' to ProceduralMeshComponent."), 
             *GetName(), 
             MeshToSet ? *MeshToSet->GetName() : TEXT("nullptr"));

	UKismetProceduralMeshLibrary::CopyProceduralMeshFromStaticMeshComponent(MeshToSet, 0, ProceduralMeshComponent, true); // Corrected function name
}

// Called every frame
void AInventoryItemActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Set item data and update the mesh
void AInventoryItemActor::SetItemData(const FSlotStruct& NewItem)
{
	Item = NewItem;
	
	// Reset ItemData pointer initially
	ItemData = nullptr;

	// Check if the provided ItemID has a valid DataTable and RowName
	if (Item.ItemID.DataTable && !Item.ItemID.RowName.IsNone())
	{
		// Attempt to find the row in the DataTable
		ItemData = Item.ItemID.DataTable->FindRow<FInventoryItemStruct>(Item.ItemID.RowName, TEXT("SetItemData"));

		if (!ItemData)
		{
			// Log an error if the row was not found
			UE_LOG(LogTemp, Error, TEXT("AInventoryItemActor [%s]: SetItemData: Could not find Row '%s' in DataTable '%s'."), 
				*GetName(), 
				*Item.ItemID.RowName.ToString(), 
				*Item.ItemID.DataTable->GetName());
		}
	}
	else
	{
		// Log a warning if ItemID is not properly set
		UE_LOG(LogTemp, Warning, TEXT("AInventoryItemActor [%s]: SetItemData: ItemID is not valid (DataTable or RowName missing)."), *GetName());
	}

	UpdateMeshFromData(); // Update visual representation
	OnItemDataUpdated(); // Call the notification function (defined in .h, potentially implemented in BP)
}
