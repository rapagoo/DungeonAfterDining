// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/WarriorBaseCharacter.h"
#include "GameplayTagContainer.h"
#include "WarriorHeroCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UDataAsset_InputConfig;
class UInputMappingContext;
class USceneCaptureComponent2D;
class USphereComponent;
class AInteractableTable;
class UCookingWidget;
struct FInputActionValue;
class UHeroCombatComponent;
class UHeroUIComponent;
class UInventoryComponent;
class AInventoryItemActor;

/**
 * 
 */
UCLASS()
class DUNGEON_API AWarriorHeroCharacter : public AWarriorBaseCharacter
{
	GENERATED_BODY()

public:	
	AWarriorHeroCharacter();

	//~ Begin PawnCombatInterface Interface.
	virtual UPawnCombatComponent* GetPawnCombatComponent() const override;
	//~ End PawnCombatInterface Interface

	//~ Begin IPawnUIInterface Interface.
	virtual UPawnUIComponent* GetPawnUIComponent() const override;
	virtual UHeroUIComponent* GetHeroUIComponent() const override;
	//~ End IPawnUIInterface Interface

	// Returns the cooking mode status
	UFUNCTION(BlueprintPure, Category = "State")
	bool IsInCookingMode() const;

	// Function to handle placing an item onto a table (implementation in .cpp)
	UFUNCTION(BlueprintCallable, Category = "Inventory | Interaction")
	void PlaceItemOnTable(int32 SlotIndex);

	// Getter for the currently overlapping interactable table
	UFUNCTION(BlueprintPure, Category = "Interaction")
	AInteractableTable* GetCurrentInteractableTable() const { return CurrentInteractableTable; }

protected:
	//~ Begin APawn Interface.
	virtual void PossessedBy(AController* NewController) override;
	//~ End APawn Interface

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void BeginPlay() override;

	// Inventory Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UInventoryComponent* InventoryComponent;

	// Interaction Sphere Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	USphereComponent* InteractionSphere;

	// Currently overlapping interactable table
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	AInteractableTable* CurrentInteractableTable;

	// State flag for cooking mode
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	bool bIsInCookingMode = false;

	// Interaction Sphere Overlap Handlers
	UFUNCTION()
	void OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// Input handler for toggling cooking mode (bound to C key)
	void Input_ToggleCookingModePressed();

	// Input handlers for slicing action
	void Input_SliceStart();
	void Input_SliceEnd();

	// Performs the actual slice on the item
	void PerformSlice(AInventoryItemActor* ItemToSlice, const FVector& PlanePosition, const FVector& PlaneNormal);

	// The Blueprint class of the cooking widget to spawn
	UPROPERTY(EditDefaultsOnly, Category = "UI | Cooking")
	TSubclassOf<UCookingWidget> CookingWidgetClass;

	// Pointer to the currently active cooking widget instance
	UPROPERTY(Transient) // Use Transient as it's managed during gameplay
	TWeakObjectPtr<UCookingWidget> CurrentCookingWidget;

<<<<<<< HEAD
	// Particle effect to play when slicing
	// Marked as deprecated, replace with Niagara. No longer exposed to editor/blueprints.
	// UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking")
	// UParticleSystem* SliceEffect; // Removed UPROPERTY

	// Niagara system for slicing effect
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking")
	UNiagaraSystem* SliceNiagaraEffect;

	// Sound effect to play when slicing
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cooking")
	USoundBase* SliceSound;

=======
>>>>>>> parent of 01b607d (썰기 기능에 이펙트 및 사운드 추가할 수 있도록 변경)
private:

#pragma region Components

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	USceneCaptureComponent2D* SceneCaptureComponent2D;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UHeroCombatComponent* HeroCombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI", meta = (AllowPrivateAccess = "true"))
	UHeroUIComponent* HeroUIComponent;

#pragma endregion

#pragma region Inputs

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CharacterData", meta = (AllowPrivateAccess = "true"))
	UDataAsset_InputConfig* InputConfigDataAsset;

	void Input_Move(const FInputActionValue& InputActionValue);
	void Input_Look(const FInputActionValue& InputActionValue);

	void Input_SwitchTargetTriggered(const FInputActionValue& InputActionValue);
	void Input_SwitchTargetCompleted(const FInputActionValue& InputActionValue);

	FVector2D SwitchDirection = FVector2D::ZeroVector;

	void Input_AbilityInputPressed(FGameplayTag InInputTag);
	void Input_AbilityInputReleased(FGameplayTag InInputTag);

#pragma endregion

	// Input Mapping Context for cooking mode
	UPROPERTY(EditDefaultsOnly, Category = "Input | Cooking")
	UInputMappingContext* CookingMappingContext;

	// Input Action for slicing
	UPROPERTY(EditDefaultsOnly, Category = "Input | Cooking")
	class UInputAction* SliceAction;

	// The default class of item actor to spawn when placing items on a table
	UPROPERTY(EditDefaultsOnly, Category = "Inventory | Interaction")
	TSubclassOf<AInventoryItemActor> DefaultItemActorClass;

	// Variables to store slice start/end points
	FVector SliceStartWorldLocation = FVector::ZeroVector;
	FVector SliceEndWorldLocation = FVector::ZeroVector;
	FVector2D SliceStartScreenPosition = FVector2D::ZeroVector;
	bool bIsDraggingSlice = false;

    // Pointer to the item actor being dragged over at the start of a slice attempt
    UPROPERTY() // Keep track of the actor reference
    AInventoryItemActor* SlicedItemCandidate = nullptr;

public:
	FORCEINLINE UHeroCombatComponent* GetHeroCombatComponent() const { return HeroCombatComponent; }

	// Return Inventory Component
	UInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Function to get the slicing start location (if available)
	UFUNCTION(BlueprintPure, Category = "Slicing")
	FVector GetSliceStartLocation() const { return SliceStartWorldLocation; }

	// Function to get the slicing end location (if available)
	UFUNCTION(BlueprintPure, Category = "Slicing")
	FVector GetSliceEndLocation() const { return SliceEndWorldLocation; }
};
