// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/InventoryItemActor.h"
#include "Components/SceneComponent.h"            // For USceneComponent
#include "Components/StaticMeshComponent.h"      // For UStaticMeshComponent
#include "ProceduralMeshComponent.h"           // For UProceduralMeshComponent
#include "Engine/StaticMesh.h"                 // For UStaticMesh
#include "Engine/DataTable.h"                  // For UDataTable and FDataTableRowHandle
#include "Inventory/InvenItemStruct.h" // For FInventoryItemStruct (Item Details)
#include "Inventory/SlotStruct.h"     // For FSlotStruct (Item Property Type)
#include "Engine/CollisionProfile.h" // Correct include for UCollisionProfile
#include "PhysicsEngine/BodySetup.h" // Correct path for UBodySetup
#include "Kismet/GameplayStatics.h" // For gameplay static functions
#include "Components/PrimitiveComponent.h" // Needed for collision/physics related operations
#include "KismetProceduralMeshLibrary.h" // Needed for slicing operation
#include "Materials/MaterialInterface.h" // Needed for material operations during slicing

// Sets default values
AInventoryItemActor::AInventoryItemActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false; // Optimize: Disable tick if not needed

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = DefaultSceneRoot;

	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	StaticMeshComponent->SetupAttachment(RootComponent);
    // Ensure the static mesh also uses complex collision for accurate slicing start/end points if needed
    // StaticMeshComponent->bUseComplexAsSimpleCollision = true; // Consider implications

	// Create the Procedural Mesh Component
    ProceduralMeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMeshComponent"));
    ProceduralMeshComponent->SetupAttachment(RootComponent);
    ProceduralMeshComponent->bUseComplexAsSimpleCollision = true; // Important for accurate slicing/collision
    ProceduralMeshComponent->SetVisibility(false); // Initially hidden
    ProceduralMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision); // Initially no collision

    // Default CapMaterial might be null, can be set in Blueprint Defaults
    CapMaterial = nullptr; 

    // Default state is not sliced
    bIsSliced = false;

    if (ItemData) {
        UpdateMeshFromData();
    }
}

// Called when the game starts or when spawned
void AInventoryItemActor::BeginPlay()
{
	Super::BeginPlay();
	
	// Removed ItemData lookup logic, assuming it's handled elsewhere or defaults are set
	// UpdateMesh(); // If UpdateMesh relies on ItemData, call it after lookup

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
	if (!ProceduralMeshComponent || !StaticMeshComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("AInventoryItemActor [%s]: UpdateMeshFromData: ProceduralMeshComponent or StaticMeshComponent is missing!"), *GetNameSafe(this));
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
    StaticMeshComponent->SetStaticMesh(MeshToSet);

    // Copy from the temporary source component to the procedural mesh component
    if (MeshToSet) // Only copy if we successfully loaded a mesh
    {
         UE_LOG(LogTemp, Log, TEXT("AInventoryItemActor [%s]: Copying mesh from StaticMeshComponent ('%s') to ProceduralMeshComponent."),
                 *GetNameSafe(this),
                 *MeshToSet->GetName());

        UKismetProceduralMeshLibrary::CopyProceduralMeshFromStaticMeshComponent(
            StaticMeshComponent,
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

// Slices the item mesh
void AInventoryItemActor::SliceItem(const FVector& PlanePosition, const FVector& PlaneNormal)
{
    // --- Prevent slicing if already sliced ---
    if (bIsSliced || OtherHalfProceduralMeshComponent != nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("AInventoryItemActor [%s]: SliceItem called, but item is already sliced. Ignoring."), *GetNameSafe(this));
        return;
    }

    if (!ProceduralMeshComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("SliceItem failed: ProceduralMeshComponent is null on %s."), *GetName());
        return;
    }

    // --- Hide StaticMeshComponent if it exists and PMC is being used ---
    if (StaticMeshComponent && ProceduralMeshComponent->GetNumSections() > 0)
    {
        StaticMeshComponent->SetVisibility(false);
        StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        UE_LOG(LogTemp, Log, TEXT("AInventoryItemActor [%s]: Hiding StaticMeshComponent."), *GetNameSafe(this));
    }
    // --- Ensure the primary PMC is visible and interactable before slicing ---
    ProceduralMeshComponent->SetVisibility(true);
    // ProceduralMeshComponent->SetCollisionProfileName(FName("PhysicsActor")); // Set later before physics
    ProceduralMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    ProceduralMeshComponent->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);

    // Use world space plane position for slicing
    FVector WorldPlanePosition = PlanePosition; // Assuming trace hit location is already in world space
    FVector WorldPlaneNormal = PlaneNormal;     // Assuming trace hit normal is already in world space

    // --- Destroy the PREVIOUS OtherHalf component if it exists from a prior slice ---
    if (OtherHalfProceduralMeshComponent)
    {
         UE_LOG(LogTemp, Log, TEXT("AInventoryItemActor [%s]: Destroying previous OtherHalfProceduralMeshComponent before new slice."), *GetNameSafe(this));
         OtherHalfProceduralMeshComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
         OtherHalfProceduralMeshComponent->UnregisterComponent();
         OtherHalfProceduralMeshComponent->DestroyComponent();
         OtherHalfProceduralMeshComponent = nullptr;
    }

    // --- Temporarily disable physics on the component to be sliced ---
    bool bWasSimulatingPhysics = ProceduralMeshComponent->IsSimulatingPhysics();
    if (bWasSimulatingPhysics)
    {
        ProceduralMeshComponent->SetSimulatePhysics(false);
         UE_LOG(LogTemp, Log, TEXT("AInventoryItemActor [%s]: Temporarily disabled physics before slicing."), *GetNameSafe(this));
    }

    // Ensure the procedural mesh has physics data ready for slicing by enabling collision
    ProceduralMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    // Ensure the physics body instance is created if it wasn't already
    if (ProceduralMeshComponent->GetBodyInstance() != nullptr && !ProceduralMeshComponent->GetBodyInstance()->IsValidBodyInstance())
    {
         ProceduralMeshComponent->RecreatePhysicsState();
         UE_LOG(LogTemp, Log, TEXT("AInventoryItemActor [%s]: Recreated physics state for ProceduralMeshComponent before slicing."), *GetNameSafe(this));
    } else if (ProceduralMeshComponent->GetBodyInstance() == nullptr) {
         UE_LOG(LogTemp, Warning, TEXT("AInventoryItemActor [%s]: ProceduralMeshComponent BodyInstance is null before slicing."), *GetNameSafe(this));
         // Optionally try recreating state here too?
         // ProceduralMeshComponent->RecreatePhysicsState();
    }

    // Perform the actual slice using SliceProceduralMesh
    UE_LOG(LogTemp, Log, TEXT("AInventoryItemActor [%s]: Calling SliceProceduralMesh."), *GetNameSafe(this));
    double StartTime = FPlatformTime::Seconds();

    UProceduralMeshComponent* TempOtherHalf = nullptr;

    UKismetProceduralMeshLibrary::SliceProceduralMesh(
        ProceduralMeshComponent,
        WorldPlanePosition,
        WorldPlaneNormal,
        true,
        TempOtherHalf,
        EProcMeshSliceCapOption::CreateNewSectionForCap,
        CapMaterial
    );

    double EndTime = FPlatformTime::Seconds();
    UE_LOG(LogTemp, Log, TEXT("AInventoryItemActor [%s]: SliceProceduralMesh completed in %.4f seconds."), *GetNameSafe(this), EndTime - StartTime);


    // Check if the slice was successful and generated the other half
    if (TempOtherHalf && TempOtherHalf->GetNumSections() > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("AInventoryItemActor [%s]: Slice successful. TempOtherHalf component created with %d sections."), *GetNameSafe(this), TempOtherHalf->GetNumSections());

        // --- Assign and Configure the NEW OtherHalf component ---
        OtherHalfProceduralMeshComponent = TempOtherHalf;
        if (!OtherHalfProceduralMeshComponent->GetAttachParent()) {
             // Try KeepWorldTransform first, as SliceProceduralMesh might create the component in world space relative to the original
             OtherHalfProceduralMeshComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform); 
        }
        OtherHalfProceduralMeshComponent->RegisterComponent(); // Ensure it's registered
        OtherHalfProceduralMeshComponent->SetVisibility(true);

        // --- Configure Physics for the NEW OtherHalf component ---
        FName CollisionProfileName = FName("PhysicsActor"); // Use the profile that simulates physics
        OtherHalfProceduralMeshComponent->SetCollisionProfileName(CollisionProfileName);
        UE_LOG(LogTemp, Log, TEXT("AInventoryItemActor [%s]: Set OtherHalf Collision Profile to '%s'. Current Profile: %s"),
            *GetNameSafe(this),
            *CollisionProfileName.ToString(),
            *OtherHalfProceduralMeshComponent->GetCollisionProfileName().ToString());

        OtherHalfProceduralMeshComponent->SetSimulatePhysics(true); // Enable physics simulation
        // Optional: Add impulse if needed
        FVector ImpulseDirection = WorldPlaneNormal.GetSafeNormal(); // Use slice normal for direction
        float ImpulseStrength = 100.0f; // Adjust as needed
        OtherHalfProceduralMeshComponent->AddImpulse(ImpulseDirection * ImpulseStrength, NAME_None, true); // Push away from the cut

        // Log final state of OtherHalf
         UE_LOG(LogTemp, Log, TEXT("AInventoryItemActor [%s]: New OtherHalf State: Visible=%d, Enabled=%d, Profile=%s, SimPhys=%d, Sections=%d"),
               *GetNameSafe(this),
               OtherHalfProceduralMeshComponent->IsVisible(),
               OtherHalfProceduralMeshComponent->IsCollisionEnabled(),
               *OtherHalfProceduralMeshComponent->GetCollisionProfileName().ToString(),
               OtherHalfProceduralMeshComponent->IsSimulatingPhysics(),
               OtherHalfProceduralMeshComponent->GetNumSections());


        // --- Re-configure Physics for the ORIGINAL PMC component ---
        // Ensure the original part also uses the physics profile and simulates physics
        ProceduralMeshComponent->SetCollisionProfileName(CollisionProfileName); // Match profile
         UE_LOG(LogTemp, Log, TEXT("AInventoryItemActor [%s]: Set Original PMC Collision Profile to '%s' AFTER Slice. Current Profile: %s"),
               *GetNameSafe(this),
               *CollisionProfileName.ToString(),
               *ProceduralMeshComponent->GetCollisionProfileName().ToString());

        // **** THIS IS THE FIX for original part not simulating physics ****
        ProceduralMeshComponent->SetSimulatePhysics(true); // <<<< RE-ENABLE PHYSICS
        // ******************************************************************

        // Add opposite impulse to the original part
        ProceduralMeshComponent->AddImpulse(-ImpulseDirection * ImpulseStrength, NAME_None, true); // Push other way

        // Log final state of Original PMC
        UE_LOG(LogTemp, Log, TEXT("AInventoryItemActor [%s]: Original PMC State After Slice: Visible=%d, Enabled=%d, Profile=%s, SimPhys=%d, Sections=%d"),
               *GetNameSafe(this),
               ProceduralMeshComponent->IsVisible(),
               ProceduralMeshComponent->IsCollisionEnabled(),
               *ProceduralMeshComponent->GetCollisionProfileName().ToString(),
               ProceduralMeshComponent->IsSimulatingPhysics(),
               ProceduralMeshComponent->GetNumSections()); // Log section count


        // Mark as sliced
        bIsSliced = true;
    }
    else // Slice failed or created an empty component
    {
        UE_LOG(LogTemp, Error, TEXT("AInventoryItemActor [%s]: SliceProceduralMesh failed or did not generate OtherHalf component."), *GetNameSafe(this));
        if (TempOtherHalf)
        {
            TempOtherHalf->DestroyComponent();
             UE_LOG(LogTemp, Warning, TEXT("AInventoryItemActor [%s]: Destroyed empty/invalid temporary OtherHalf component."), *GetNameSafe(this));
        }
         // Re-enable physics on original component if it was disabled before the failed slice attempt
         if (bWasSimulatingPhysics)
         {
             ProceduralMeshComponent->SetSimulatePhysics(true);
             // Maybe recreate state here too if slice fails?
             ProceduralMeshComponent->RecreatePhysicsState();
             ProceduralMeshComponent->WakeAllRigidBodies();
             UE_LOG(LogTemp, Log, TEXT("AInventoryItemActor [%s]: Re-enabled physics on original PMC after failed slice."), *GetNameSafe(this));
         }
    }
}

void AInventoryItemActor::RequestEnablePhysics()
{
    UE_LOG(LogTemp, Log, TEXT("AInventoryItemActor [%s]: RequestEnablePhysics called. Setting bEnablePhysicsRequested = true."), *GetNameSafe(this));
    // Only set the flag. The actual enabling will happen in Tick.
    bEnablePhysicsRequested = true;
    // Ensure Tick is enabled if it wasn't already (important if Tick was disabled for optimization)
    if (!PrimaryActorTick.IsTickFunctionEnabled())
    {
        SetActorTickEnabled(true);
        UE_LOG(LogTemp, Log, TEXT("AInventoryItemActor [%s]: Tick function was disabled, re-enabling for physics request."), *GetNameSafe(this));
    }
}

// Called every frame
void AInventoryItemActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Handle physics enabling request
	if (bEnablePhysicsRequested)
	{
		bEnablePhysicsRequested = false; // Consume the request

		// Choose which component simulates physics based on sliced state
		if (bIsSliced)
		{
			 // If already sliced, physics should be handled by the proc meshes (likely enabled in SliceItem)
             // This block can ensure they are still simulating if needed.
			 if (ProceduralMeshComponent && !ProceduralMeshComponent->IsSimulatingPhysics())
             {
                 ProceduralMeshComponent->SetSimulatePhysics(true);
                 UE_LOG(LogTemp, Verbose, TEXT("AInventoryItemActor [%s]: Tick - Ensuring ProceduralMeshComponent simulates physics (already sliced)."), *GetNameSafe(this));
             }
             if (OtherHalfProceduralMeshComponent && !OtherHalfProceduralMeshComponent->IsSimulatingPhysics())
             {
                 OtherHalfProceduralMeshComponent->SetSimulatePhysics(true);
                  UE_LOG(LogTemp, Verbose, TEXT("AInventoryItemActor [%s]: Tick - Ensuring OtherHalfProceduralMeshComponent simulates physics (already sliced)."), *GetNameSafe(this));
             }
		}
		else
		{
			// If not sliced, enable physics on the StaticMeshComponent
			if (StaticMeshComponent && !StaticMeshComponent->IsSimulatingPhysics())
			{
				UE_LOG(LogTemp, Log, TEXT("AInventoryItemActor [%s]: Tick - Enabling physics on StaticMeshComponent."), *GetNameSafe(this));
				// Ensure compatible collision settings BEFORE enabling physics simulation
				StaticMeshComponent->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
				StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				StaticMeshComponent->SetSimulatePhysics(true);
			}
			else if (!StaticMeshComponent)
			{
				UE_LOG(LogTemp, Warning, TEXT("AInventoryItemActor [%s]: Tick - Could not enable physics on StaticMeshComponent (null?)."), *GetNameSafe(this));
			}
            else if (StaticMeshComponent && StaticMeshComponent->IsSimulatingPhysics())
            {
                 // Already simulating, do nothing
                 UE_LOG(LogTemp, Verbose, TEXT("AInventoryItemActor [%s]: Tick - Physics enable requested, but StaticMeshComponent already simulating."), *GetNameSafe(this));
            }
		}
	}
}

void AInventoryItemActor::SetItemData(const FSlotStruct& NewItem)
{
	// 1. Store the input slot data
	this->Item = NewItem;

	// 2. Load the DataTable (ensure InventoryDataTable is valid)
	UDataTable* LoadedDataTable = InventoryDataTable.LoadSynchronous();
	if (!LoadedDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("InventoryDataTable is not valid in AInventoryItemActor::SetItemData for %s"), *GetName());
		ItemData = nullptr; // Ensure ItemData is null if table fails
		return; // Cannot proceed without the data table
	}

	// 3. Find the row in the DataTable using Item.ItemID.RowName
	// Use RowName from FDataTableRowHandle
	if (Item.ItemID.RowName.IsNone()) // Check RowName for None
	{
		UE_LOG(LogTemp, Warning, TEXT("ItemID.RowName is None in AInventoryItemActor::SetItemData for %s"), *GetName());
		ItemData = nullptr; // Cannot lookup with None RowName
	}
	else
	{
		static const FString ContextString(TEXT("SetItemData Lookup"));
		// Pass RowName (FName) to FindRow
		ItemData = LoadedDataTable->FindRow<FInventoryItemStruct>(Item.ItemID.RowName, ContextString);

		if (!ItemData)
		{
			// Use RowName.ToString() for logging
			UE_LOG(LogTemp, Error, TEXT("Could not find ItemID.RowName '%s' in InventoryDataTable for %s"), *Item.ItemID.RowName.ToString(), *GetName());
		}
	}

	// 4. Update the mesh based on the (potentially newly found) ItemData
	UpdateMeshFromData();

	// 5. Call the Blueprint event (optional, depends on your logic)
	OnItemDataUpdated();
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

// Called when properties are changed in the editor AFTER construction
void AInventoryItemActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    // Get the name of the property that changed
    FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

    // If the ItemID (or its DataTable/RowName) or the InventoryDataTable changes, update ItemData and mesh
    if (PropertyName == GET_MEMBER_NAME_CHECKED(AInventoryItemActor, Item) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(FDataTableRowHandle, DataTable) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(FDataTableRowHandle, RowName) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(AInventoryItemActor, InventoryDataTable))
    {
         UE_LOG(LogTemp, Log, TEXT("AInventoryItemActor [%s]: PostEditChangeProperty detected change in ItemID or DataTable. Updating..."), *GetNameSafe(this));
        // Ensure the ItemID uses the assigned InventoryDataTable if its own DataTable is null
         if (InventoryDataTable.IsValid() && Item.ItemID.DataTable == nullptr)
         {
             Item.ItemID.DataTable = InventoryDataTable.LoadSynchronous();
         }

         // Look up the FInventoryItemStruct based on the (potentially updated) Item.ItemID
         if (Item.ItemID.DataTable && !Item.ItemID.RowName.IsNone())
         {             ItemData = Item.ItemID.DataTable->FindRow<FInventoryItemStruct>(Item.ItemID.RowName, TEXT("PostEditChangeProperty"));
         }
         else
         {
             ItemData = nullptr; // Clear if ItemID is invalid
         }
         UpdateMeshFromData(); // Update the mesh display
    }

     // If CapMaterial changed, we don't need to do anything immediately,
     // but it will be used the next time SliceItem is called.
    if (PropertyName == GET_MEMBER_NAME_CHECKED(AInventoryItemActor, CapMaterial))
    {
        UE_LOG(LogTemp, Log, TEXT("AInventoryItemActor [%s]: CapMaterial changed."), *GetNameSafe(this));
    }
}
