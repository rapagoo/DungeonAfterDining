// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/InventoryItemActor.h"
#include "Components/StaticMeshComponent.h"
#include "Inventory/InvenItemStruct.h" // Corrected include path
// #include "DataAssets/Inventory/DataAsset_ItemLookup.h" // Removed include, file not found
#include "Kismet/GameplayStatics.h"
#include "ProceduralMeshComponent.h" // Include for Procedural Mesh
#include "KismetProceduralMeshLibrary.h" // Include for mesh conversion
#include "Engine/DataTable.h"
#include "Engine/StaticMesh.h"
#include "Inventory/SlotStruct.h"     // For FSlotStruct (Item Property Type)
#include "Engine/CollisionProfile.h" // Correct include for UCollisionProfile
#include "PhysicsEngine/BodySetup.h" // Correct path for UBodySetup
#include "Sound/SoundBase.h" // For cutting sound
#include "Particles/ParticleSystem.h" // For cutting effects
#include "NiagaraSystem.h" // For Niagara cutting effects
#include "NiagaraFunctionLibrary.h" // For spawning Niagara effects
#include "EngineUtils.h" // For TActorIterator
#include "Components/BoxComponent.h" // For UBoxComponent
#include "Interactables/InteractableTable.h" // For table interaction
#include "UI/Inventory/CookingWidget.h" // For cooking widget updates

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

    // NEW: Initialize cutting state
    bIsCut = false;
    CurrentCuttingStyle = ECuttingStyle::ECS_None;

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

// REMOVED: Old SliceItem function - replaced with new CutItemWithStyle system

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

		// Choose which component simulates physics based on cut state
		if (bIsCut)
		{
			 // If already cut, physics should be handled by the proc meshes
             // This block can ensure they are still simulating if needed.
			 if (ProceduralMeshComponent && !ProceduralMeshComponent->IsSimulatingPhysics())
             {
                 ProceduralMeshComponent->SetSimulatePhysics(true);
                 UE_LOG(LogTemp, Verbose, TEXT("AInventoryItemActor [%s]: Tick - Ensuring ProceduralMeshComponent simulates physics (already cut)."), *GetNameSafe(this));
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

// REMOVED SetStaticMesh function definition as declaration was removed from header
/*
void AInventoryItemActor::SetStaticMesh(UStaticMesh* NewStaticMesh)
{
    // Use the correct component name declared in the header
    if (!StaticMeshComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("AInventoryItemActor::SetStaticMesh - StaticMeshComponent is null!"));
        return;
    }
    if (!ProceduralMeshComponent) // This check is fine
    {
         UE_LOG(LogTemp, Error, TEXT("AInventoryItemActor::SetStaticMesh - ProceduralMeshComponent is null!"));
         return;
    }

    // Use the correct component name
    StaticMeshComponent->SetStaticMesh(NewStaticMesh);

    if (NewStaticMesh)
    {
        // Copy mesh data from the correct component
        UKismetProceduralMeshLibrary::CopyProceduralMeshFromStaticMeshComponent(
            StaticMeshComponent, // Use the correct source component
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
*/

#if WITH_EDITOR
// Called when properties are changed in the editor AFTER construction
void AInventoryItemActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	UObject::PostEditChangeProperty(PropertyChangedEvent); // Call UObject's implementation directly

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

    // NEW: Check if cutting-related properties changed
    // (No specific actions needed for now)
}
#endif

// NEW: Button-based cutting system implementation
bool AInventoryItemActor::CutItemWithStyle(ECuttingStyle CuttingStyle)
{
    if (!CanBeCutWithStyle(CuttingStyle))
    {
        UE_LOG(LogTemp, Warning, TEXT("AInventoryItemActor [%s]: Cannot cut with style %d"), 
               *GetNameSafe(this), (int32)CuttingStyle);
        return false;
    }

    if (!ItemData)
    {
        UE_LOG(LogTemp, Error, TEXT("AInventoryItemActor [%s]: ItemData is null, cannot cut"), 
               *GetNameSafe(this));
        return false;
    }

    // Check if cutting is already in progress
    if (bIsCuttingInProgress)
    {
        UE_LOG(LogTemp, Warning, TEXT("AInventoryItemActor [%s]: Cutting already in progress"), 
               *GetNameSafe(this));
        return false;
    }

    // Verify the target mesh exists
    TSoftObjectPtr<UStaticMesh> NewMeshPtr;
    switch (CuttingStyle)
    {
        case ECuttingStyle::ECS_Diced:
            NewMeshPtr = ItemData->DicedMesh;
            break;
        case ECuttingStyle::ECS_Julienne:
            NewMeshPtr = ItemData->JulienneMesh;
            break;
        case ECuttingStyle::ECS_Minced:
            NewMeshPtr = ItemData->MincedMesh;
            break;
        default:
            UE_LOG(LogTemp, Error, TEXT("AInventoryItemActor [%s]: Invalid cutting style %d"), 
                   *GetNameSafe(this), (int32)CuttingStyle);
            return false;
    }

    // Preload the mesh to ensure it exists
    UStaticMesh* NewMesh = NewMeshPtr.LoadSynchronous();
    if (!NewMesh)
    {
        UE_LOG(LogTemp, Error, TEXT("AInventoryItemActor [%s]: Failed to load mesh for cutting style %d"), 
               *GetNameSafe(this), (int32)CuttingStyle);
        return false;
    }

    // Start cutting animation
    bIsCuttingInProgress = true;
    CuttingSoundCount = 0;
    PendingCuttingStyle = CuttingStyle;

    // Start the cutting animation timer (play sound every 0.3 seconds for 5 times)
    GetWorld()->GetTimerManager().SetTimer(
        CuttingAnimationTimer,
        this,
        &AInventoryItemActor::PlayCuttingAnimationStep,
        0.3f, // Interval between cuts
        true  // Loop
    );

    UE_LOG(LogTemp, Log, TEXT("AInventoryItemActor [%s]: Started cutting animation for style %d"), 
           *GetNameSafe(this), (int32)CuttingStyle);

    return true;
}

bool AInventoryItemActor::CanBeCutWithStyle(ECuttingStyle CuttingStyle) const
{
    if (!ItemData || CuttingStyle == ECuttingStyle::ECS_None || bIsCuttingInProgress)
    {
        return false;
    }

    // Check if the appropriate mesh exists for this cutting style
    switch (CuttingStyle)
    {
        case ECuttingStyle::ECS_Diced:
            return !ItemData->DicedMesh.IsNull();
        case ECuttingStyle::ECS_Julienne:
            return !ItemData->JulienneMesh.IsNull();
        case ECuttingStyle::ECS_Minced:
            return !ItemData->MincedMesh.IsNull();
        default:
            return false;
    }
}

TArray<ECuttingStyle> AInventoryItemActor::GetAvailableCuttingStyles() const
{
    TArray<ECuttingStyle> AvailableStyles;

    if (!ItemData || bIsCuttingInProgress)
    {
        return AvailableStyles;
    }

    // Check each cutting style and add if mesh is available
    if (!ItemData->DicedMesh.IsNull())
        AvailableStyles.Add(ECuttingStyle::ECS_Diced);
    
    if (!ItemData->JulienneMesh.IsNull())
        AvailableStyles.Add(ECuttingStyle::ECS_Julienne);
    
    if (!ItemData->MincedMesh.IsNull())
        AvailableStyles.Add(ECuttingStyle::ECS_Minced);

    return AvailableStyles;
}

// NEW: Cutting animation implementation
void AInventoryItemActor::PlayCuttingAnimationStep()
{
    CuttingSoundCount++;
    
    // Play cutting sound
    if (CuttingSound && GetWorld())
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), CuttingSound, GetActorLocation());
        UE_LOG(LogTemp, Log, TEXT("AInventoryItemActor [%s]: Playing cutting sound %d/5"), 
               *GetNameSafe(this), CuttingSoundCount);
    }
    
    // Spawn cutting effect
    if (CuttingEffect && GetWorld())
    {
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), CuttingEffect, GetActorLocation());
    }
    else if (CuttingNiagaraEffect && GetWorld())
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), CuttingNiagaraEffect, GetActorLocation());
    }
    
    // Check if we've played 5 cutting sounds
    if (CuttingSoundCount >= 5)
    {
        // Stop the timer and complete the cutting
        GetWorld()->GetTimerManager().ClearTimer(CuttingAnimationTimer);
        CompleteCuttingAnimation();
    }
}

void AInventoryItemActor::CompleteCuttingAnimation()
{
    if (!ItemData || PendingCuttingStyle == ECuttingStyle::ECS_None)
    {
        UE_LOG(LogTemp, Error, TEXT("AInventoryItemActor [%s]: CompleteCuttingAnimation called with invalid data"), 
               *GetNameSafe(this));
        bIsCuttingInProgress = false;
        return;
    }

    // Get the appropriate mesh based on pending cutting style
    TSoftObjectPtr<UStaticMesh> NewMeshPtr;
    switch (PendingCuttingStyle)
    {
        case ECuttingStyle::ECS_Diced:
            NewMeshPtr = ItemData->DicedMesh;
            break;
        case ECuttingStyle::ECS_Julienne:
            NewMeshPtr = ItemData->JulienneMesh;
            break;
        case ECuttingStyle::ECS_Minced:
            NewMeshPtr = ItemData->MincedMesh;
            break;
        default:
            UE_LOG(LogTemp, Error, TEXT("AInventoryItemActor [%s]: Invalid pending cutting style %d"), 
                   *GetNameSafe(this), (int32)PendingCuttingStyle);
            bIsCuttingInProgress = false;
            return;
    }

    // Load the new mesh
    UStaticMesh* NewMesh = NewMeshPtr.LoadSynchronous();
    if (!NewMesh)
    {
        UE_LOG(LogTemp, Error, TEXT("AInventoryItemActor [%s]: Failed to load mesh for cutting style %d"), 
               *GetNameSafe(this), (int32)PendingCuttingStyle);
        bIsCuttingInProgress = false;
        return;
    }

    // Hide the old mesh components
    if (ProceduralMeshComponent)
    {
        ProceduralMeshComponent->SetVisibility(false);
        ProceduralMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    // Set the new mesh on StaticMeshComponent (use mesh's own materials)
    StaticMeshComponent->SetStaticMesh(NewMesh);
    
    // NEW: Don't copy original materials - let the new mesh use its own materials
    UE_LOG(LogTemp, Log, TEXT("AInventoryItemActor [%s]: Set new mesh with its own materials"), 
           *GetNameSafe(this));
    
    StaticMeshComponent->SetVisibility(true);
    StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

    // Update cutting style and state
    CurrentCuttingStyle = PendingCuttingStyle;
    bIsCut = true;
    bIsCuttingInProgress = false;
    PendingCuttingStyle = ECuttingStyle::ECS_None;
    CuttingSoundCount = 0;

    UE_LOG(LogTemp, Log, TEXT("AInventoryItemActor [%s]: Successfully completed cutting with style %d"), 
           *GetNameSafe(this), (int32)CurrentCuttingStyle);

    // Call Blueprint event for cutting completion (if you want to add more effects)
    OnItemDataUpdated();

    // NEW: Notify nearby cooking widgets that the item state has changed
    // This ensures the AddIngredient button gets updated after cutting
    UWorld* World = GetWorld();
    if (World)
    {
        // Find all CookingWidget instances and update them
        for (TActorIterator<class AInteractableTable> ActorItr(World); ActorItr; ++ActorItr)
        {
            AInteractableTable* Table = *ActorItr;
            if (Table && Table->GetActiveCookingWidget())
            {
                // Check if this item is within the table's detection area
                UBoxComponent* DetectionArea = Table->FindComponentByClass<UBoxComponent>();
                if (DetectionArea)
                {
                    TArray<AActor*> OverlappingActors;
                    DetectionArea->GetOverlappingActors(OverlappingActors, AInventoryItemActor::StaticClass());
                    
                    for (AActor* Actor : OverlappingActors)
                    {
                        if (Actor == this)
                        {
                            // This item is within the table's detection area, update the widget
                            Table->GetActiveCookingWidget()->UpdateNearbyIngredient(this);
                            UE_LOG(LogTemp, Log, TEXT("AInventoryItemActor [%s]: Updated CookingWidget after cutting completion"), 
                                   *GetNameSafe(this));
                            break;
                        }
                    }
                }
            }
        }
    }
}
