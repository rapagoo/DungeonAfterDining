// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/InventoryItemActor.h"
//#include "Components/StaticMeshComponent.h" // Now included in header
//#include "Components/ProceduralMeshComponent.h" // Standard path commented out
#include "ProceduralMeshComponent.h" // Direct path as per user preference
#include "KismetProceduralMeshLibrary.h" // Needed for CopyProceduralMeshFromStaticMeshComponent
//#include "Components/PrimitiveComponent.h" // Removed, not needed for BodySetup approach
#include "Engine/DataTable.h"
#include "Engine/StaticMesh.h"
#include "Inventory/InvenItemStruct.h" // For FInventoryItemStruct (Data Table Row Type)
#include "Inventory/SlotStruct.h"     // For FSlotStruct (Item Property Type)
#include "Engine/CollisionProfile.h" // Correct include for UCollisionProfile
#include "PhysicsEngine/BodySetup.h" // Correct path for UBodySetup

// Sets default values
AInventoryItemActor::AInventoryItemActor()
{
 	// Set this actor to call Tick() every frame.
	PrimaryActorTick.bCanEverTick = true; // Enable Tick

	// Create the Procedural Mesh Component and set as root
	ProceduralMeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMeshComponent"));
	RootComponent = ProceduralMeshComponent;
    // Set a default blocking profile initially to prevent falling through floor on spawn
	ProceduralMeshComponent->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
	ProceduralMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ProceduralMeshComponent->SetSimulatePhysics(false); // Start with physics off

	// Create the temporary source Static Mesh Component
	TemporarySourceMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TemporarySourceMeshComponent"));
	TemporarySourceMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision); // No collision needed
	TemporarySourceMeshComponent->SetVisibility(false); // Not visible
	TemporarySourceMeshComponent->SetupAttachment(RootComponent); // Attach to root, although not strictly necessary if only used for data

    if (ItemData) {
        UpdateMeshFromData();
    }
}

// Called when the game starts or when spawned
void AInventoryItemActor::BeginPlay()
{
	Super::BeginPlay();

    // Ensure mesh is updated if it wasn't in PostActorCreated
    if (!ItemData && !Item.ItemID.RowName.IsNone()) {
        // If ItemID is set but ItemData wasn't loaded yet (e.g., deferred), try setting it now.
        SetItemData(Item);
    } else if (ItemData) {
         // If ItemData was already set (e.g., in editor or PostActorCreated), ensure mesh reflects it.
         UpdateMeshFromData();
    } else {
         UE_LOG(LogTemp, Log, TEXT("AInventoryItemActor [%s]: BeginPlay: No valid ItemID or ItemData to initialize mesh."), *GetNameSafe(this));
    }
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

    // After copying or clearing, ensure collision settings are suitable for physics
    if (ProceduralMeshComponent)
    {
        UBodySetup* BodySetup = ProceduralMeshComponent->GetBodySetup();
        UE_LOG(LogTemp, Log, TEXT("AInventoryItemActor [%s]: GetBodySetup result: %s"), *GetNameSafe(this), BodySetup ? TEXT("Valid") : TEXT("NULL")); // Log BodySetup validity
        if (BodySetup)
        {
            BodySetup->CollisionTraceFlag = ECollisionTraceFlag::CTF_UseSimpleAsComplex;
            // Use the new custom profile defined in Project Settings
            ProceduralMeshComponent->SetCollisionProfileName(FName("DroppedItem")); // Use the new profile name
            ProceduralMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            // REMOVED: Channel responses are now handled by the "DroppedItem" profile
            // ProceduralMeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); 

            // Log applied settings (Profile should now show "DroppedItem")
            UE_LOG(LogTemp, Log, TEXT("AInventoryItemActor [%s]: Applied collision settings (Profile: %s, Enabled: %s)"), 
                *GetNameSafe(this), 
                *ProceduralMeshComponent->GetCollisionProfileName().ToString(), 
                ProceduralMeshComponent->IsCollisionEnabled() ? TEXT("True") : TEXT("False")); // Log applied settings

            // Mark collision state as dirty to apply changes
            ProceduralMeshComponent->MarkRenderStateDirty();
        }
        else
        {
             UE_LOG(LogTemp, Warning, TEXT("AInventoryItemActor [%s]: Could not get BodySetup from ProceduralMeshComponent for collision setup."), *GetNameSafe(this));
        }
    }
}

void AInventoryItemActor::RequestEnablePhysics()
{
    bEnablePhysicsRequested = true;
}

// Called every frame
void AInventoryItemActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    if (bEnablePhysicsRequested)
    {
        UProceduralMeshComponent* ProcMesh = Cast<UProceduralMeshComponent>(GetRootComponent());
        if (ProcMesh && !ProcMesh->IsSimulatingPhysics()) // Check if not already simulating
        {
            // Log current state before enabling physics
            UE_LOG(LogTemp, Log, TEXT("AInventoryItemActor [%s]: Tick - Enabling physics. Current State: Profile=%s, Enabled=%s, WorldStaticResponse=%d"), 
                *GetNameSafe(this), 
                *ProcMesh->GetCollisionProfileName().ToString(), 
                ProcMesh->IsCollisionEnabled() ? TEXT("True") : TEXT("False"),
                ProcMesh->GetCollisionResponseToChannel(ECC_WorldStatic)); // Log current settings

            // Ensure it blocks the ground just in case
            ProcMesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block); 
            ProcMesh->SetSimulatePhysics(true);
            UE_LOG(LogTemp, Log, TEXT("AInventoryItemActor [%s]: Enabled physics in Tick after request."), *GetNameSafe(this));
            // Optionally disable tick after physics is enabled if no longer needed
            // SetActorTickEnabled(false); 
        }
        bEnablePhysicsRequested = false; // Reset the flag regardless of success
    }
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

    // REMOVED: Do not enable physics here. Physics should be enabled externally only when dropped.
    // UProceduralMeshComponent* ProcMesh = Cast<UProceduralMeshComponent>(GetRootComponent());
    // if (ProcMesh)
    // {
    //     ProcMesh->SetSimulatePhysics(true);
    //     UE_LOG(LogTemp, Log, TEXT("AInventoryItemActor [%s]: Enabled physics in SetItemData after setup."), *GetNameSafe(this));
    // }
    // else
    // {
    //     UE_LOG(LogTemp, Warning, TEXT("AInventoryItemActor [%s]: Could not get ProcMesh in SetItemData to enable physics."), *GetNameSafe(this));
    // }
}

// Called when the actor is done spawning
void AInventoryItemActor::PostActorCreated()
{
    Super::PostActorCreated();

    // Defer mesh update until BeginPlay if ItemData is not yet set
    if (ItemData) {
        UpdateMeshFromData();
    }
}

void AInventoryItemActor::SetStaticMesh(UStaticMesh* NewStaticMesh)
{
    // Use the correct component name declared in the header
    if (!TemporarySourceMeshComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("AInventoryItemActor::SetStaticMesh - TemporarySourceMeshComponent is null!"));
        return;
    }
    if (!ProceduralMeshComponent) // This check is fine
    {
         UE_LOG(LogTemp, Error, TEXT("AInventoryItemActor::SetStaticMesh - ProceduralMeshComponent is null!"));
         return;
    }

    // Use the correct component name
    TemporarySourceMeshComponent->SetStaticMesh(NewStaticMesh);

    if (NewStaticMesh)
    {
        // Copy mesh data from the correct component
        UKismetProceduralMeshLibrary::CopyProceduralMeshFromStaticMeshComponent(
            TemporarySourceMeshComponent, // Use the correct source component
            0, // LOD Index
            ProceduralMeshComponent,
            true // Create Collision
        );
         UE_LOG(LogTemp, Log, TEXT("AInventoryItemActor [%s]: SetStaticMesh: Copied mesh from StaticMesh '%s' to ProceduralMeshComponent."), *GetNameSafe(this), *NewStaticMesh->GetName());
    }
    else
    {
         UE_LOG(LogTemp, Log, TEXT("AInventoryItemActor [%s]: SetStaticMesh: NewStaticMesh is null, clearing ProceduralMeshComponent."), *GetNameSafe(this));
        // If NewStaticMesh is null, clear the procedural mesh
        ProceduralMeshComponent->ClearAllMeshSections();
    }

    // After copying or clearing, ensure collision settings are suitable for physics
    if (ProceduralMeshComponent)
    {
        UBodySetup* BodySetup = ProceduralMeshComponent->GetBodySetup();
        if (BodySetup)
        {
            BodySetup->CollisionTraceFlag = ECollisionTraceFlag::CTF_UseSimpleAsComplex; // Use simple collision for physics
            ProceduralMeshComponent->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName); // Ensure physics profile
            ProceduralMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); // Ensure collision is enabled for physics
            ProceduralMeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); // Also ensure it blocks Visibility traces here

            // Mark collision state as dirty to apply changes
            ProceduralMeshComponent->MarkRenderStateDirty();
        }
        else
        {
             UE_LOG(LogTemp, Warning, TEXT("AInventoryItemActor [%s]: Could not get BodySetup from ProceduralMeshComponent for collision setup."), *GetNameSafe(this));
        }
    }
}
