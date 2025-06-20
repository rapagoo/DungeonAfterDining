// Copyright Epic Games, Inc. All Rights Reserved.

#include "ClimbingSystemCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/CustomMovementComponent.h"
#include "Inventory/InventoryComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "MotionWarpingComponent.h"
#include "MyGameInstance.h"
#include "Kismet/GameplayStatics.h"

#include "WarriorDebugHelper.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AClimbingSystemCharacter

AClimbingSystemCharacter::AClimbingSystemCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCustomMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	CustomMovementComponent = Cast<UCustomMovementComponent>(GetCharacterMovement());

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarpingComp"));

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));

}

//////////////////////////////////////////////////////////////////////////
// Input

void AClimbingSystemCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	AddInputMappingContext(DefaultMappingContext, 0);

	if (CustomMovementComponent)
	{
		CustomMovementComponent->OnEnterClimbStateDelegate.BindUObject(this, &ThisClass::OnPlayerEnterClimbState);
		CustomMovementComponent->OnExitClimbStateDelegate.BindUObject(this, &ThisClass::OnPlayerExitClimbState);
	}
}

void AClimbingSystemCharacter::AddInputMappingContext(UInputMappingContext* ContextToAdd, int32 InPriority)
{
	if (!ContextToAdd) return;

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(ContextToAdd, InPriority);
		}
	}
}

void AClimbingSystemCharacter::RemoveInputMappingContext(UInputMappingContext* ContextToAdd)
{
	if (!ContextToAdd) return;

	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->RemoveMappingContext(ContextToAdd);
		}
	}
}

void AClimbingSystemCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Bind Inventory actions first
		if (InventoryComponent)
		{
			InventoryComponent->SetupInputBinding(EnhancedInputComponent);
			UE_LOG(LogTemplateCharacter, Log, TEXT("InventoryComponent input binding setup called for %s"), *GetName());
		}
		else
		{
			UE_LOG(LogTemplateCharacter, Error, TEXT("InventoryComponent is null on %s during SetupPlayerInputComponent"), *GetName());
		}

		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AClimbingSystemCharacter::HandleGroundMovementInput);
		EnhancedInputComponent->BindAction(ClimbMoveAction, ETriggerEvent::Triggered, this, &AClimbingSystemCharacter::HandleClimbMovementInput);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AClimbingSystemCharacter::Look);

		EnhancedInputComponent->BindAction(ClimbAction, ETriggerEvent::Started, this, &AClimbingSystemCharacter::OnClimbActionStarted);

		EnhancedInputComponent->BindAction(ClimbHopAction, ETriggerEvent::Started, this, &AClimbingSystemCharacter::OnClimbHopActionStarted);
	}
}

void AClimbingSystemCharacter::HandleGroundMovementInput(const FInputActionValue& Value)
{
	// input is a Vector2D
	const FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AClimbingSystemCharacter::HandleClimbMovementInput(const FInputActionValue& Value)
{
	// input is a Vector2D
	const FVector2D MovementVector = Value.Get<FVector2D>();

	const FVector ForwardDirection = FVector::CrossProduct(
		-CustomMovementComponent->GetClimableSurfaceNormal(),
		GetActorRightVector()
	);

	const FVector RightDirection = FVector::CrossProduct(
		-CustomMovementComponent->GetClimableSurfaceNormal(),
		-GetActorUpVector()
	);


	// add movement 
	AddMovementInput(ForwardDirection, MovementVector.Y);
	AddMovementInput(RightDirection, MovementVector.X);
}

void AClimbingSystemCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AClimbingSystemCharacter::OnClimbActionStarted(const FInputActionValue& Value)
{
	if (!CustomMovementComponent) return;

	if (!CustomMovementComponent->IsClimbing())
	{
		CustomMovementComponent->ToggleClimbing(true);
	}
	else
	{
		CustomMovementComponent->ToggleClimbing(false);
	}
}

void AClimbingSystemCharacter::OnPlayerEnterClimbState()
{
	AddInputMappingContext(ClimbMappingContext, 1);
}

void AClimbingSystemCharacter::OnPlayerExitClimbState()
{
	RemoveInputMappingContext(ClimbMappingContext);
}

void AClimbingSystemCharacter::OnClimbHopActionStarted(const FInputActionValue& Value)
{
	if (CustomMovementComponent)
	{
		CustomMovementComponent->RequestHopping();
	}
}

void AClimbingSystemCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Attempt to load inventory from GameInstance
	UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
	if (UMyGameInstance* MyGI = Cast<UMyGameInstance>(GI))
	{
		if (InventoryComponent) // Ensure InventoryComponent is valid
		{
			if (MyGI->TempInventoryToTransfer.Num() > 0)
			{
				UE_LOG(LogTemplateCharacter, Log, TEXT("%s::BeginPlay: Loading %d items from GameInstance TempInventoryToTransfer."), *GetName(), MyGI->TempInventoryToTransfer.Num());
				InventoryComponent->InventorySlots = MyGI->TempInventoryToTransfer;
				// Optionally, update UI or other components that depend on inventory here
				MyGI->TempInventoryToTransfer.Empty();
				UE_LOG(LogTemplateCharacter, Log, TEXT("%s::BeginPlay: Cleared GameInstance TempInventoryToTransfer."), *GetName());
			}
			else
			{
				UE_LOG(LogTemplateCharacter, Log, TEXT("%s::BeginPlay: No inventory data found in GameInstance TempInventoryToTransfer. Using default/existing inventory."), *GetName());
			}
		}
		else
		{
			UE_LOG(LogTemplateCharacter, Warning, TEXT("%s::BeginPlay: InventoryComponent is null. Cannot load inventory."), *GetName());
		}
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("%s::BeginPlay: Could not cast GameInstance to UMyGameInstance. Inventory not loaded."), *GetName());
	}
}