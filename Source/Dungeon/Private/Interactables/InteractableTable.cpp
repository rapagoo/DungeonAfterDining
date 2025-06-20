// Fill out your copyright notice in the Description page of Project Settings.


#include "Interactables/InteractableTable.h"
#include "Camera/CameraActor.h"
#include "Components/SceneComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/BoxComponent.h"
#include "Inventory/InventoryItemActor.h"
#include "UI/Inventory/CookingWidget.h"
#include "Kismet/GameplayStatics.h"


// Sets default values
AInteractableTable::AInteractableTable()
{
	// Set this actor to call Tick() every frame.
	PrimaryActorTick.bCanEverTick = true; // Enable Tick

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = DefaultSceneRoot;

	// Create the ingredient detection area
	IngredientArea = CreateDefaultSubobject<UBoxComponent>(TEXT("IngredientArea"));
	IngredientArea->SetupAttachment(RootComponent);
	IngredientArea->SetBoxExtent(FVector(100.0f, 100.0f, 10.0f)); // Example size, adjust in BP
	IngredientArea->SetCollisionProfileName(FName("OverlapAllDynamic")); // Set collision to overlap dynamic objects
	IngredientArea->SetHiddenInGame(false); // Make visible for debugging initially

	// Set the collision profile for the root component to the custom preset
	// Make sure "InteractableObject" matches the preset name created in Project Settings!
	if (UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(RootComponent))
	{
		RootPrimitive->SetCollisionProfileName(FName("InteractableObject"));
	} else {
		// If using a basic USceneComponent as root, add a collision component manually
		// For example, add a UBoxComponent and set its profile instead.
		// This example assumes RootComponent itself can have collision (like a StaticMeshComponent)
		// Or, the BP derived from this C++ class will set collision on its components.
		UE_LOG(LogTemp, Warning, TEXT("AInteractableTable: RootComponent is not a UPrimitiveComponent. Collision profile not set in C++. Ensure collision is set in Blueprint or add a collision component."));
	}
}

// Called when the game starts or when spawned
void AInteractableTable::BeginPlay()
{
	Super::BeginPlay();
	ActiveCookingWidget = nullptr; // Ensure it starts null
}

// Called every frame
void AInteractableTable::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// --- DEPRECATED: Table no longer directly updates widget based on nearby items --- 
	/*
	// Only perform check if the cooking widget is active
	if (ActiveCookingWidget.IsValid() && IngredientArea)
	{
		TArray<AActor*> OverlappingActors;
		IngredientArea->GetOverlappingActors(OverlappingActors, AInventoryItemActor::StaticClass());

		AInventoryItemActor* FoundSlicedIngredient = nullptr;
		for (AActor* Actor : OverlappingActors)
		{
			AInventoryItemActor* ItemActor = Cast<AInventoryItemActor>(Actor);
			if (ItemActor && ItemActor->IsSliced())
			{
				FoundSlicedIngredient = ItemActor;
				break; // Found the first sliced ingredient, stop searching
			}
		}

		// Update the cooking widget with the found ingredient (or nullptr if none found)
		ActiveCookingWidget->UpdateNearbyIngredient(FoundSlicedIngredient);
	}
	*/
}

// Setter for the active cooking widget
void AInteractableTable::SetActiveCookingWidget(UCookingWidget* Widget)
{
	ActiveCookingWidget = Widget;
	if (Widget)
	{
		UE_LOG(LogTemp, Log, TEXT("AInteractableTable '%s': Associated with Cooking Widget '%s'"), *GetName(), *Widget->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("AInteractableTable '%s': Disassociated from Cooking Widget."), *GetName());
	}
}

void AInteractableTable::StartSlicingMinigame()
{
	if (ActiveCookingWidget.IsValid())
	{
		// We pass nullptr as the minigame because slicing doesn't have a formal minigame object.
		// The widget will use this to set up its UI for a "table interaction" (i.e. hide rhythm game elements).
		ActiveCookingWidget->OnMinigameStarted(nullptr);
		UE_LOG(LogTemp, Log, TEXT("AInteractableTable '%s': Called OnMinigameStarted on widget for slicing."), *GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AInteractableTable '%s': Cannot start slicing minigame UI, ActiveCookingWidget is not valid."), *GetName());
	}
}

