#include "MyGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Characters/WarriorHeroCharacter.h" // Include your character header
#include "Dungeon/ClimbingSystemCharacter.h" // Include ClimbingSystemCharacter header
#include "Inventory/InventoryComponent.h"    // Include inventory component header
#include "Misc/CoreDelegates.h"               // Corrected include path for PreLoadMap delegate

void UMyGameInstance::Init()
{
	Super::Init();

	// Bind our function to the PreLoadMap delegate
	if (!IsDedicatedServerInstance()) // Only bind on clients/listen servers where local player exists
	{
		FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UMyGameInstance::HandlePreLoadMap);
	}
}

void UMyGameInstance::HandlePreLoadMap(const FString& MapName)
{
	UE_LOG(LogTemp, Log, TEXT("UMyGameInstance::HandlePreLoadMap: Preparing to load map '%s'. Saving inventory to GameInstance Temp array..."), *MapName);

	// Get the local player controller
	APlayerController* PC = GetFirstLocalPlayerController();
	if (!PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("UMyGameInstance::HandlePreLoadMap: Could not get PlayerController. Inventory NOT saved."));
		return;
	}

	// Get the Pawn from the Controller
	APawn* MyPawn = PC->GetPawn();
	if (!MyPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("UMyGameInstance::HandlePreLoadMap: Could not get Pawn from PlayerController. Inventory NOT saved."));
		return;
	}

	UInventoryComponent* InventoryComponent = nullptr;
	FString CharacterName = MyPawn->GetName();

	// Try to get InventoryComponent from AWarriorHeroCharacter
	if (AWarriorHeroCharacter* WarriorChar = Cast<AWarriorHeroCharacter>(MyPawn))
	{
		InventoryComponent = WarriorChar->GetInventoryComponent();
	}
	// If not a WarriorHeroCharacter, try to get it from AClimbingSystemCharacter
	else if (AClimbingSystemCharacter* ClimbingChar = Cast<AClimbingSystemCharacter>(MyPawn))
	{
		InventoryComponent = ClimbingChar->GetInventoryComponent();
	}

	if (!InventoryComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("UMyGameInstance::HandlePreLoadMap: Could not get InventoryComponent from Pawn: %s. Inventory NOT saved."), *CharacterName);
		return;
	}

	// --- Save Inventory to GameInstance Temp Array ---
	TempInventoryToTransfer.Empty(); // Clear previous temp data
	TempInventoryToTransfer = InventoryComponent->InventorySlots;
	UE_LOG(LogTemp, Log, TEXT("UMyGameInstance::HandlePreLoadMap: Saved %d inventory items from '%s' to GameInstance TempInventoryToTransfer."),
		TempInventoryToTransfer.Num(),
		*CharacterName);
} 