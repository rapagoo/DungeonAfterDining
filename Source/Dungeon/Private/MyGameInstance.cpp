#include "MyGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Characters/WarriorHeroCharacter.h" // Include your character header
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

	// Get the Character/Pawn from the Controller
	AWarriorHeroCharacter* MyCharacter = Cast<AWarriorHeroCharacter>(PC->GetPawn());
	if (!MyCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("UMyGameInstance::HandlePreLoadMap: Could not get Character from PlayerController. Inventory NOT saved."));
		return;
	}

	// Get the InventoryComponent from the Character
	UInventoryComponent* InventoryComponent = MyCharacter->GetInventoryComponent(); // Assuming GetInventoryComponent() getter exists
	if (!InventoryComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("UMyGameInstance::HandlePreLoadMap: Could not get InventoryComponent from Character. Inventory NOT saved."));
		return;
	}

	// --- Save Inventory to GameInstance Temp Array ---
	TempInventoryToTransfer.Empty(); // Clear previous temp data
	TempInventoryToTransfer = InventoryComponent->InventorySlots;
	UE_LOG(LogTemp, Log, TEXT("UMyGameInstance::HandlePreLoadMap: Saved %d inventory items from '%s' to GameInstance TempInventoryToTransfer."),
		TempInventoryToTransfer.Num(),
		*MyCharacter->GetName());
} 