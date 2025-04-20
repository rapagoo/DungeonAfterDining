// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/InventoryItemActor.h"
//#include "Components/StaticMeshComponent.h" // Now included in header
//#include "Components/ProceduralMeshComponent.h" // Standard path commented out
#include "ProceduralMeshComponent.h" // Direct path as per user preference
#include "KismetProceduralMeshLibrary.h" // Needed for CopyProceduralMeshFromStaticMeshComponent
#include "Engine/DataTable.h"
#include "Engine/StaticMesh.h"
#include "Inventory/InvenItemStruct.h" // For FInventoryItemStruct (Data Table Row Type)
#include "Inventory/SlotStruct.h"     // For FSlotStruct (Item Property Type)
#include "Engine/CollisionProfile.h" // Correct include for UCollisionProfile

// Sets default values
AInventoryItemActor::AInventoryItemActor()
{
 	// Set this actor to call Tick() every frame. Typically off for construction script logic.
	PrimaryActorTick.bCanEverTick = false;

	// Create the Procedural Mesh Component and set as root
	ProceduralMeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMeshComponent"));
	RootComponent = ProceduralMeshComponent;
	ProceduralMeshComponent->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName); // Or appropriate profile
	ProceduralMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ProceduralMeshComponent->SetSimulatePhysics(false); // Usually false initially for items

	// Create the temporary source Static Mesh Component
	TemporarySourceMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TemporarySourceMeshComponent"));
	TemporarySourceMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision); // No collision needed
	TemporarySourceMeshComponent->SetVisibility(false); // Not visible
	TemporarySourceMeshComponent->SetupAttachment(RootComponent); // Attach to root, although not strictly necessary if only used for data
}

// Called when the game starts or when spawned
void AInventoryItemActor::BeginPlay()
{
	Super::BeginPlay();
	// Initial mesh update can be handled by OnConstruction or SetItemData
}

// Called when an instance of this class is placed or updated in the editor
void AInventoryItemActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    // Try to load data table row based on Item.ItemID if DataTable is assigned
    if (InventoryDataTable.IsValid() && Item.ItemID.DataTable == nullptr) {
        Item.ItemID.DataTable = InventoryDataTable.LoadSynchronous(); // Auto-assign if not set in ItemID?
    }
    // Look up item data and update mesh in construction script
    if (Item.ItemID.DataTable && !Item.ItemID.RowName.IsNone())
    {
        ItemData = Item.ItemID.DataTable->FindRow<FInventoryItemStruct>(Item.ItemID.RowName, TEXT("OnConstruction"));
    }
    else
    {
        ItemData = nullptr;
    }
    UpdateMeshFromData(); // Call the update function
}

// Renamed from UpdateStaticMesh, uses TemporarySourceMeshComponent now
void AInventoryItemActor::UpdateMeshFromData()
{
	if (!ProceduralMeshComponent || !TemporarySourceMeshComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("AInventoryItemActor [%s]: UpdateMeshFromData: ProceduralMeshComponent or TemporarySourceMeshComponent is missing!"), *GetNameSafe(this));
        return;
    }

    UStaticMesh* MeshToSet = nullptr;

    // Check if ItemData is valid (was looked up in SetItemData or OnConstruction)
	if (ItemData)
    {
        if (!ItemData->Mesh.IsNull())
        {
            MeshToSet = ItemData->Mesh.LoadSynchronous();
            if (!MeshToSet)
            {
                UE_LOG(LogTemp, Error, TEXT("AInventoryItemActor [%s]: UpdateMeshFromData: Failed to load Static Mesh from reference '%s' in ItemData. Clearing mesh."),
                    *GetNameSafe(this),
                    *ItemData->Mesh.ToString());
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("AInventoryItemActor [%s]: UpdateMeshFromData: Mesh reference in ItemData is null. Clearing mesh."), *GetNameSafe(this));
        }
    }
    else // ItemData is null
    {
        UE_LOG(LogTemp, Warning, TEXT("AInventoryItemActor [%s]: UpdateMeshFromData: ItemData is null. Clearing mesh."), *GetNameSafe(this));
        // Ensure MeshToSet remains nullptr
    }

    // Set the loaded mesh (or nullptr) on the temporary source component
    TemporarySourceMeshComponent->SetStaticMesh(MeshToSet);

    // Copy from the temporary source component to the procedural mesh component
    if (MeshToSet) // Only copy if we successfully loaded a mesh
    {
         UE_LOG(LogTemp, Log, TEXT("AInventoryItemActor [%s]: Copying mesh from TemporarySourceMeshComponent ('%s') to ProceduralMeshComponent."),
                 *GetNameSafe(this),
                 *MeshToSet->GetName());

        UKismetProceduralMeshLibrary::CopyProceduralMeshFromStaticMeshComponent(
            TemporarySourceMeshComponent,
            0,                            // LODIndex
            ProceduralMeshComponent,      // Target ProcMeshComponent
            true                          // Create collision
        );
    }
    else // Clear the procedural mesh if no valid mesh was found/loaded
    {
        UE_LOG(LogTemp, Log, TEXT("AInventoryItemActor [%s]: Clearing ProceduralMeshComponent as no valid mesh was set."), *GetNameSafe(this));
        ProceduralMeshComponent->ClearAllMeshSections();
    }
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

    // Assign the actor's default DataTable to the ItemID handle if it's not already set
    // This assumes the RowName is valid for the actor's default table.
    if (Item.ItemID.DataTable == nullptr && InventoryDataTable.IsValid() && !Item.ItemID.RowName.IsNone()) {
        Item.ItemID.DataTable = InventoryDataTable.LoadSynchronous();
        UE_LOG(LogTemp, Log, TEXT("AInventoryItemActor [%s]: SetItemData: Assigned InventoryDataTable to ItemID.DataTable."), *GetNameSafe(this));
    }

	// Check if the provided ItemID has a valid DataTable and RowName
	if (Item.ItemID.DataTable && !Item.ItemID.RowName.IsNone())
	{
		// Attempt to find the row in the DataTable
        // Note: FindRow returns a non-const pointer, so ItemData should be non-const FInventoryItemStruct*
		ItemData = Item.ItemID.DataTable->FindRow<FInventoryItemStruct>(Item.ItemID.RowName, TEXT("SetItemData"));

		if (!ItemData)
		{
			// Log an error if the row was not found
			UE_LOG(LogTemp, Error, TEXT("AInventoryItemActor [%s]: SetItemData: Could not find Row '%s' in DataTable '%s'."),
				*GetNameSafe(this),
				*Item.ItemID.RowName.ToString(),
				*Item.ItemID.DataTable->GetName());
		}
        else
        {
             UE_LOG(LogTemp, Log, TEXT("AInventoryItemActor [%s]: SetItemData: Found ItemData for Row '%s'."), *GetNameSafe(this), *Item.ItemID.RowName.ToString());
        }
	}
	else
	{
		// Log a warning if ItemID is not properly set
		UE_LOG(LogTemp, Warning, TEXT("AInventoryItemActor [%s]: SetItemData: ItemID is not valid (DataTable or RowName missing). Item.ItemID.RowName: %s"), *GetNameSafe(this), *Item.ItemID.RowName.ToString());
	}

	UpdateMeshFromData(); // Update visual representation
	OnItemDataUpdated(); // Call the notification event
}
