// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/WarriorHeroCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputSubsystems.h"
#include "DataAssets/Input/DataAsset_InputConfig.h"
#include "Components/Input/WarriorInputComponent.h"
#include "WarriorGameplayTags.h"
#include "AbilitySystem/WarriorAbilitySystemComponent.h"
#include "DataAssets/StartUpData/DataAsset_HeroStartUpData.h"
#include "Components/Combat/HeroCombatComponent.h"
#include "Components/UI/HeroUIComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Inventory/InventoryComponent.h"
#include "EnhancedInputComponent.h"
#include "Components/SphereComponent.h"
#include "Interactables/InteractableTable.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraActor.h"

#include "WarriorDebugHelper.h"

AWarriorHeroCharacter::AWarriorHeroCharacter()
{
	// Disable Tick function for performance if not needed
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->TargetArmLength = 200.f;
	CameraBoom->SocketOffset = FVector(0.f, 55.f, 65.f);
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Scene Capture Component 2D 생성 및 초기 설정
	SceneCaptureComponent2D = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCaptureComponent2D"));
	SceneCaptureComponent2D->SetupAttachment(GetRootComponent());
	
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 500.f, 0.f);
	GetCharacterMovement()->MaxWalkSpeed = 400.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	HeroCombatComponent = CreateDefaultSubobject<UHeroCombatComponent>(TEXT("HeroCombatComponent"));

	HeroUIComponent = CreateDefaultSubobject<UHeroUIComponent>(TEXT("HeroUIComponent"));

	// Temporarily disabled Inventory Component creation
	// InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));

	// Interaction Sphere for detecting interactable objects
	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(GetRootComponent());
	InteractionSphere->SetSphereRadius(150.0f); // Adjust radius as needed
	// Set the collision profile created in Project Settings for querying interactables
	InteractionSphere->SetCollisionProfileName(FName("InteractionQuery")); // Use the profile name from settings
	InteractionSphere->SetGenerateOverlapEvents(true);
    // Ensure it doesn't physically collide with anything by default
    InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    // Explicitly overlap with the custom Interactable channel (assuming it's setup in the profile)
    // If the profile handles this, these lines might be redundant but safe
    // InteractionSphere->SetCollisionResponseToChannel(ECC_GameTraceChannelX, ECR_Overlap); // Replace X with your Interactable channel index if needed
}

UPawnCombatComponent* AWarriorHeroCharacter::GetPawnCombatComponent() const
{
	return HeroCombatComponent;
}

UPawnUIComponent* AWarriorHeroCharacter::GetPawnUIComponent() const
{
	return HeroUIComponent;
}

UHeroUIComponent* AWarriorHeroCharacter::GetHeroUIComponent() const
{
	return HeroUIComponent;
}

void AWarriorHeroCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	if (!CharacterStartUpData.IsNull())
	{
		if (UDataAsset_StartUpDataBase* LoadedData = CharacterStartUpData.LoadSynchronous())
		{
			LoadedData->GiveToAbilitySystemComponent(WarriorAbilitySystemComponent);
		}
	}
}

void AWarriorHeroCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
        // Bind Inventory actions first
		if (InventoryComponent)
		{
			InventoryComponent->SetupInputBinding(EnhancedInputComponent);
			UE_LOG(LogTemp, Log, TEXT("InventoryComponent input binding setup called for %s"), *GetName());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("InventoryComponent is null on %s during SetupPlayerInputComponent"), *GetName());
		}

        // Existing Input Config setup
        checkf(InputConfigDataAsset, TEXT("Forgot to assign a valid data asset as input config"))
        ULocalPlayer* LocalPlayer = GetController<APlayerController>()->GetLocalPlayer();
        UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);
        check(Subsystem);
        Subsystem->AddMappingContext(InputConfigDataAsset->DefaultMappingContext, 0);

        UWarriorInputComponent* WarriorInputComponent = CastChecked<UWarriorInputComponent>(PlayerInputComponent);

        // Bind Movement and Look
        WarriorInputComponent->BindNativeInputAction(InputConfigDataAsset, WarriorGameplayTags::InputTag_Move, ETriggerEvent::Triggered, this, &ThisClass::Input_Move);
        WarriorInputComponent->BindNativeInputAction(InputConfigDataAsset, WarriorGameplayTags::InputTag_Look, ETriggerEvent::Triggered, this, &ThisClass::Input_Look);

        // Bind Target Switching
        WarriorInputComponent->BindNativeInputAction(InputConfigDataAsset, WarriorGameplayTags::InputTag_SwitchTarget, ETriggerEvent::Triggered, this, &ThisClass::Input_SwitchTargetTriggered);
        WarriorInputComponent->BindNativeInputAction(InputConfigDataAsset, WarriorGameplayTags::InputTag_SwitchTarget, ETriggerEvent::Completed, this, &ThisClass::Input_SwitchTargetCompleted);

        // Bind Gameplay Ability Actions
        WarriorInputComponent->BindAbilityInputAction(InputConfigDataAsset, this, &ThisClass::Input_AbilityInputPressed, &ThisClass::Input_AbilityInputReleased);

        // Bind Cooking Mode Toggle Action (Using C key, mapped to InputTag_Interact)
        const FGameplayTag InteractInputTag = WarriorGameplayTags::InputTag_Interact;
        if (InteractInputTag.IsValid() && InputConfigDataAsset) // InputConfigDataAsset null check added
        {
            UInputAction* InteractAction = nullptr; // Pointer to store the found action
            // Iterate through the AbilityInputActions array in the data asset
            for (const FWarriorInputActionConfig& ActionConfig : InputConfigDataAsset->AbilityInputActions)
            {
                if (ActionConfig.InputAction && ActionConfig.InputTag == InteractInputTag)
                {
                    InteractAction = ActionConfig.InputAction;
                    break; // Stop searching once found
                }
            }

            if (InteractAction) // Check if the action was found
            {
                EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Triggered, this, &AWarriorHeroCharacter::Input_ToggleCookingModePressed);
                UE_LOG(LogTemp, Log, TEXT("Bound InputTag_Interact to Input_ToggleCookingModePressed"));
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Input Action for InputTag_Interact not found in InputConfigDataAsset. Cooking mode toggle not bound."));
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("InputTag_Interact GameplayTag is invalid or InputConfigDataAsset is null. Cooking mode toggle not bound."));
        }

	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to cast PlayerInputComponent to UEnhancedInputComponent on %s"), *GetName());
	}
}

void AWarriorHeroCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	// Scene Capture Component 2D 설정 (블루프린트 "Show Only Actor Components" 구현)
	if (SceneCaptureComponent2D)
	{
		// 기본 설정
		SceneCaptureComponent2D->bCaptureEveryFrame = true;
		
		// Show Only Actor Components 설정 - 자신(Self)의 컴포넌트만 표시
		SceneCaptureComponent2D->ShowOnlyActorComponents(this);
		
		// 추가 설정
		SceneCaptureComponent2D->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDR;
	}

    // Bind overlap events for the interaction sphere
    if (InteractionSphere)
    {
        InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &AWarriorHeroCharacter::OnInteractionSphereBeginOverlap);
        InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &AWarriorHeroCharacter::OnInteractionSphereEndOverlap);
        UE_LOG(LogTemp, Log, TEXT("InteractionSphere overlap events bound."));
    }
    else
    {
         UE_LOG(LogTemp, Warning, TEXT("InteractionSphere is null in BeginPlay, overlap events not bound."));
    }
}

void AWarriorHeroCharacter::Input_Move(const FInputActionValue& InputActionValue)
{
	const FVector2D MovementVector = InputActionValue.Get<FVector2D>();

	const FRotator MovementRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);

	if (MovementVector.Y != 0.f)
	{
		const FVector ForwardDirection = MovementRotation.RotateVector(FVector::ForwardVector);

		AddMovementInput(ForwardDirection, MovementVector.Y);
	}

	if (MovementVector.X != 0.f)
	{
		const FVector RightDirection = MovementRotation.RotateVector(FVector::RightVector);

		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AWarriorHeroCharacter::Input_Look(const FInputActionValue& InputActionValue)
{
	const FVector2D LookAxisVector = InputActionValue.Get<FVector2D>();

	if (LookAxisVector.X != 0.f)
	{
		AddControllerYawInput(LookAxisVector.X);
	}

	if (LookAxisVector.Y != 0.f)
	{
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AWarriorHeroCharacter::Input_SwitchTargetTriggered(const FInputActionValue& InputActionValue)
{
	SwitchDirection = InputActionValue.Get<FVector2D>();
}

void AWarriorHeroCharacter::Input_SwitchTargetCompleted(const FInputActionValue& InputActionValue)
{
	FGameplayEventData Data;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		this,
		SwitchDirection.X > 0.f? WarriorGameplayTags::Player_Event_SwitchTarget_Right : WarriorGameplayTags::Player_Event_SwitchTarget_Left,
		Data
	);
}

void AWarriorHeroCharacter::Input_AbilityInputPressed(FGameplayTag InInputTag)
{
	WarriorAbilitySystemComponent->OnAbilityInputPressed(InInputTag);
}

void AWarriorHeroCharacter::Input_AbilityInputReleased(FGameplayTag InInputTag)
{
	WarriorAbilitySystemComponent->OnAbilityInputReleased(InInputTag);
}

void AWarriorHeroCharacter::OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    AInteractableTable* Table = Cast<AInteractableTable>(OtherActor);
    if (Table && !CurrentInteractableTable) // Check if it's a table and we aren't already focused on one
    {
        CurrentInteractableTable = Table;
        // TODO: Show UI Prompt (e.g., "Press C to Cook")
        UE_LOG(LogTemp, Log, TEXT("Interactable table in range: %s"), *Table->GetName());
    }
}

void AWarriorHeroCharacter::OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    AInteractableTable* Table = Cast<AInteractableTable>(OtherActor);
    if (Table && Table == CurrentInteractableTable) // Check if the exiting actor is our current target
    {
        CurrentInteractableTable = nullptr;
        // TODO: Hide UI Prompt
        UE_LOG(LogTemp, Log, TEXT("Interactable table out of range: %s"), *Table->GetName());
    }
}

void AWarriorHeroCharacter::Input_ToggleCookingModePressed()
{
    APlayerController* PlayerController = Cast<APlayerController>(GetController());
    if (!PlayerController) return;

    if (bIsInCookingMode)
    {
        // Exit Cooking Mode
        UE_LOG(LogTemp, Log, TEXT("Exiting Cooking Mode..."));
        PlayerController->SetViewTargetWithBlend(this, 0.5f); // Blend back to self
        
        // Restore game input mode
        FInputModeGameOnly InputModeData;
        PlayerController->SetInputMode(InputModeData);
        PlayerController->SetShowMouseCursor(false); // Hide cursor

        bIsInCookingMode = false;
    }
    else
    {   
        // Attempt to Enter Cooking Mode
        if (CurrentInteractableTable) 
        {
            ACameraActor* CookingCamera = CurrentInteractableTable->GetCookingCamera();
            if (CookingCamera)
            {
                UE_LOG(LogTemp, Log, TEXT("Entering Cooking Mode... Target Table: %s, Camera: %s"), *CurrentInteractableTable->GetName(), *CookingCamera->GetName());
                PlayerController->SetViewTargetWithBlend(CookingCamera, 0.5f); // Blend to table camera

                // Set input mode for cooking (needs mouse interaction)
                FInputModeGameAndUI InputModeData;
                // Optionally set a widget to focus if needed, otherwise focuses viewport
                // InputModeData.SetWidgetToFocus(nullptr); 
                InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock); // Allow mouse to move freely
                PlayerController->SetInputMode(InputModeData);
                PlayerController->SetShowMouseCursor(true); // Show cursor for interaction

                bIsInCookingMode = true;
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Cannot enter cooking mode: Table '%s' has no CookingCamera assigned."), *CurrentInteractableTable->GetName());
            }
        }
        else
        {
            // Optionally provide feedback if no table is in range
            UE_LOG(LogTemp, Log, TEXT("ToggleCookingMode pressed, but no interactable table in range."));
        }
    }
}
