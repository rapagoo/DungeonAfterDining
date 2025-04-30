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
#include "Inventory/InventoryItemActor.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "Inventory/SlotStruct.h"
#include "Components/SceneComponent.h" // 추가

#include "WarriorDebugHelper.h"
#include "UI/Inventory/CookingWidget.h" // Include the cooking widget header
#include "Blueprint/UserWidget.h" // Needed for CreateWidget
#include "Particles/ParticleSystem.h" // Added for UParticleSystem
#include "Sound/SoundBase.h" // Added for USoundBase
#include "NiagaraFunctionLibrary.h" // Added for Niagara SpawnSystem
#include "NiagaraComponent.h" // Added for UNiagaraSystem type

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

// Getter function implementation for cooking mode status
bool AWarriorHeroCharacter::IsInCookingMode() const
{
	return bIsInCookingMode;
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

        // Bind Slicing Action (ensure SliceAction is set in Character Blueprint defaults)
        if (SliceAction)
        {
            EnhancedInputComponent->BindAction(SliceAction, ETriggerEvent::Started, this, &AWarriorHeroCharacter::Input_SliceStart); // Use Started for press
            EnhancedInputComponent->BindAction(SliceAction, ETriggerEvent::Completed, this, &AWarriorHeroCharacter::Input_SliceEnd); // Use Completed for release
            EnhancedInputComponent->BindAction(SliceAction, ETriggerEvent::Canceled, this, &AWarriorHeroCharacter::Input_SliceEnd); // Also handle cancellation
             UE_LOG(LogTemp, Log, TEXT("Bound SliceAction to Input_SliceStart/End"));
        }
        else
        {
             UE_LOG(LogTemp, Warning, TEXT("SliceAction is not set in Character Defaults. Slicing input not bound."));
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
        UE_LOG(LogTemp, Warning, TEXT("OnInteractionSphereEndOverlap: Exiting range of CurrentInteractableTable '%s'. Clearing pointer. IsInCookingMode? %s"), 
            *Table->GetName(),
            bIsInCookingMode ? TEXT("Yes") : TEXT("No"));
        CurrentInteractableTable = nullptr;
        // TODO: Hide UI Prompt
        // UE_LOG(LogTemp, Log, TEXT("Interactable table out of range: %s"), *Table->GetName()); // Original log
    }
}

void AWarriorHeroCharacter::Input_ToggleCookingModePressed()
{
    APlayerController* PlayerController = Cast<APlayerController>(GetController());
    if (!PlayerController) return;

    ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
    if (!LocalPlayer) return;

    UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);
    if (!Subsystem) return;

    if (bIsInCookingMode)
    {
        // Exit Cooking Mode
        UE_LOG(LogTemp, Log, TEXT("Exiting Cooking Mode..."));

        // Remove and clean up the cooking widget
        if (CurrentCookingWidget.IsValid())
        {
            CurrentCookingWidget->RemoveFromParent();
            if (CurrentInteractableTable) // Ensure table still exists
            {
                CurrentInteractableTable->SetActiveCookingWidget(nullptr);
            }
            CurrentCookingWidget = nullptr;
        }

        // Remove Cooking Input Mapping Context if it's valid
        if (CookingMappingContext)
        {
            Subsystem->RemoveMappingContext(CookingMappingContext);
        }
        PlayerController->SetViewTargetWithBlend(this, 0.5f); // Blend back to self

        // Restore game input mode
        FInputModeGameOnly InputModeData;
        PlayerController->SetInputMode(InputModeData);
        PlayerController->SetShowMouseCursor(false); // Hide cursor

        bIsInCookingMode = false;
        bIsDraggingSlice = false; // Reset dragging state
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
                // Add Cooking Input Mapping Context if it's valid
                if (CookingMappingContext)
                {
                     Subsystem->AddMappingContext(CookingMappingContext, 1); // Priority 1
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("CookingMappingContext is not set in Character Defaults. Cooking input might not work."));
                }

                PlayerController->SetViewTargetWithBlend(CookingCamera, 0.5f); // Blend to table camera

                // Set input mode for cooking (needs mouse interaction)
                FInputModeGameAndUI InputModeData;
                InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
                InputModeData.SetHideCursorDuringCapture(false); // Ensure cursor stays visible while dragging
                PlayerController->SetInputMode(InputModeData);
                PlayerController->SetShowMouseCursor(true); // Show cursor for interaction

                bIsInCookingMode = true;

                // Create and show the cooking widget
                if (CookingWidgetClass)
                {
                    UCookingWidget* NewWidget = CreateWidget<UCookingWidget>(PlayerController, CookingWidgetClass);
                    if (NewWidget)
                    {
                        NewWidget->AddToViewport();
                        CurrentInteractableTable->SetActiveCookingWidget(NewWidget); // Link widget to table
                        CurrentCookingWidget = NewWidget; // Store reference
                    }
                    else
                    {
                        UE_LOG(LogTemp, Error, TEXT("Failed to create Cooking Widget instance."));
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("CookingWidgetClass is not set in Character Defaults. Cannot create cooking UI."));
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Cannot enter cooking mode: Table '%s' has no CookingCamera assigned."), *CurrentInteractableTable->GetName());
            }
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("ToggleCookingMode pressed, but no interactable table in range."));
        }
    }
}

void AWarriorHeroCharacter::Input_SliceStart()
{
    UE_LOG(LogTemp, Log, TEXT("Input_SliceStart triggered."));
    if (!bIsInCookingMode || !CurrentInteractableTable) 
    {
        UE_LOG(LogTemp, Warning, TEXT("Input_SliceStart aborted: Not in cooking mode (%d) or no table (%d)."), bIsInCookingMode, CurrentInteractableTable != nullptr);
        return;
    }

    APlayerController* PlayerController = Cast<APlayerController>(GetController());
    if (!PlayerController) 
    {
         UE_LOG(LogTemp, Warning, TEXT("Input_SliceStart aborted: No PlayerController."));
         return;
    }

    // Get current mouse position
    if (PlayerController->GetMousePosition(SliceStartScreenPosition.X, SliceStartScreenPosition.Y))
    {
        // UE_LOG(LogTemp, Log, TEXT("Slice Start - Screen Pos: %s"), *SliceStartScreenPosition.ToString()); // 제거

        // Use GetHitResultUnderCursor to find the object directly under the mouse
        FHitResult HitResult;
        bool bHit = PlayerController->GetHitResultUnderCursorByChannel(
            UEngineTypes::ConvertToTraceType(ECollisionChannel::ECC_GameTraceChannel1), // Use InteractionQuery Channel
            true, // bTraceComplex
            HitResult
        );

        // Draw debug point for where the cursor trace hit (or where it would have been)
        // We might need WorldOrigin/Direction if trace fails, but let's get it anyway for debug
        // FVector WorldOrigin, WorldDirection; // 제거
        // UGameplayStatics::DeprojectScreenToWorld(PlayerController, SliceStartScreenPosition, WorldOrigin, WorldDirection); // 제거
        // DrawDebugSphere(GetWorld(), WorldOrigin + WorldDirection * HitResult.Distance, 5.0f, 12, FColor::Yellow, false, 30.0f); // 제거

        // Reset candidate initially
        SlicedItemCandidate = nullptr;
        bIsDraggingSlice = false; 

        if (bHit && HitResult.GetActor())
        {   
            // Check if the hit actor is an inventory item
            AInventoryItemActor* HitItemActor = Cast<AInventoryItemActor>(HitResult.GetActor());
            if (HitItemActor)
            {
                SliceStartWorldLocation = HitResult.Location; // Use the actual hit location
                SlicedItemCandidate = HitItemActor; // Store the candidate item
                // UE_LOG(LogTemp, Log, TEXT("Slice Start - Cursor trace hit INVENTORY ITEM: %s at %s. Storing as candidate. Setting bIsDraggingSlice = true."), // 제거
                //     *SlicedItemCandidate->GetName(), // 제거
                //     *SliceStartWorldLocation.ToString() // 제거
                // ); // 제거
                 bIsDraggingSlice = true;
                 // DrawDebugSphere(GetWorld(), SliceStartWorldLocation, 5.0f, 12, FColor::Cyan, false, 30.0f); // 제거
            }
            else
            {
                // UE_LOG(LogTemp, Log, TEXT("Slice Start - Cursor trace hit actor '%s', but it's not an InventoryItemActor."), *HitResult.GetActor()->GetName()); // 제거
                SliceStartWorldLocation = FVector::ZeroVector; // Still reset location if not an item
            }
        }
        else
        {
             // UE_LOG(LogTemp, Warning, TEXT("Slice Start - Cursor trace did not hit anything.")); // 제거
             SliceStartWorldLocation = FVector::ZeroVector; // Ensure reset
        }
        
        // Ensure drag state is false if we didn't hit a valid item
        if (!SlicedItemCandidate)
        {
            bIsDraggingSlice = false;
        }
    }
    else
    {
        // UE_LOG(LogTemp, Warning, TEXT("Slice Start - Failed to get mouse position. bIsDraggingSlice remains false.")); // 제거
         bIsDraggingSlice = false; // Explicitly ensure it's false
    }
}

void AWarriorHeroCharacter::Input_SliceEnd()
{
    // UE_LOG(LogTemp, Log, TEXT("Input_SliceEnd triggered. Current bIsDraggingSlice = %d"), bIsDraggingSlice); // 제거
    if (!bIsInCookingMode || !bIsDraggingSlice || !CurrentInteractableTable) 
    {
         // UE_LOG(LogTemp, Warning, TEXT("Input_SliceEnd aborted early: CookingMode=%d, IsDragging=%d, Table=%d"), bIsInCookingMode, bIsDraggingSlice, CurrentInteractableTable != nullptr); // 제거
         // Reset slice state even if aborted early
         bIsDraggingSlice = false;
         SliceStartWorldLocation = FVector::ZeroVector;
         SliceEndWorldLocation = FVector::ZeroVector;
         return; 
    }

    APlayerController* PlayerController = Cast<APlayerController>(GetController());
    if (!PlayerController) 
    {
        // UE_LOG(LogTemp, Warning, TEXT("Input_SliceEnd aborted: No PlayerController.")); // 제거
        bIsDraggingSlice = false; // Ensure reset
        SliceStartWorldLocation = FVector::ZeroVector; 
        return;
    }

    FVector2D SliceEndScreenPosition;
    if (PlayerController->GetMousePosition(SliceEndScreenPosition.X, SliceEndScreenPosition.Y))
    {
        // UE_LOG(LogTemp, Log, TEXT("Slice End - Screen Pos: %s"), *SliceEndScreenPosition.ToString()); // 제거

        // --- Check Screen Distance First ---
        const float ScreenDistance = FVector2D::Distance(SliceStartScreenPosition, SliceEndScreenPosition);
        const float ScreenDistanceThreshold = 10.0f; // Minimum pixel distance for a slice
        // UE_LOG(LogTemp, Log, TEXT("Calculated Screen Distance: %.2f (Threshold: %.1f)"), ScreenDistance, ScreenDistanceThreshold); // 제거

        if (ScreenDistance < ScreenDistanceThreshold)
        {
            // UE_LOG(LogTemp, Log, TEXT("Slice canceled - screen drag distance too short.")); // 제거
        }
        else
        {   
            // Screen distance is sufficient, get world end location via GetHitResultUnderCursor
            FHitResult EndHitResult;
            bool bEndHit = PlayerController->GetHitResultUnderCursorByChannel(
                UEngineTypes::ConvertToTraceType(ECollisionChannel::ECC_GameTraceChannel1),
                true, // bTraceComplex
                EndHitResult
            );
            
            // Draw debug point for where the end cursor trace hit
            // FVector EndWorldOrigin, EndWorldDirection; // Need temp vars for debug draw // 제거
            // UGameplayStatics::DeprojectScreenToWorld(PlayerController, SliceEndScreenPosition, EndWorldOrigin, EndWorldDirection); // 제거
            // DrawDebugSphere(GetWorld(), EndWorldOrigin + EndWorldDirection * EndHitResult.Distance, 5.0f, 12, FColor::Orange, false, 30.0f); // 제거

            AInventoryItemActor* EndHitItemActor = nullptr;
            if (bEndHit && EndHitResult.GetActor())
            {
                SliceEndWorldLocation = EndHitResult.Location;
                EndHitItemActor = Cast<AInventoryItemActor>(EndHitResult.GetActor()); // Try casting end hit actor
                // ... Log End Cursor trace hit ...
                // ... Draw Debug Sphere (Magenta) ...
            }
            else
            {
                 // UE_LOG(LogTemp, Warning, TEXT("Slice End - Cursor trace for end point did not hit anything or invalid actor.")); // 제거
                 SliceEndWorldLocation = FVector::ZeroVector;
            }

            // --- Perform Slice only if Start and End hit the SAME Inventory Item ---
            if (SlicedItemCandidate != nullptr && // Was a valid item hit at the start?
                EndHitItemActor == SlicedItemCandidate && // Did the end hit the SAME item?
                !SliceStartWorldLocation.IsNearlyZero() && 
                !SliceEndWorldLocation.IsNearlyZero())
            {
                // All conditions met, proceed with slice
                if (SlicedItemCandidate)
                {
                    // UE_LOG(LogTemp, Log, TEXT("Start/End points valid and hit the SAME InventoryItem (%s). Performing Slice."), *SlicedItemCandidate->GetName()); // 제거
                     // Draw the debug line representing the slice path
                    // DrawDebugLine(GetWorld(), SliceStartWorldLocation, SliceEndWorldLocation, FColor::Yellow, false, 30.0f, 0, 1.0f); // 제거

                    // Calculate cutting plane (Moved calculation here)
                    FVector SliceDirection = (SliceEndWorldLocation - SliceStartWorldLocation).GetSafeNormal();
                    FVector PlanePosition = (SliceStartWorldLocation + SliceEndWorldLocation) / 2.0f;
                    FVector PlaneNormal = (SliceDirection ^ FVector::UpVector).GetSafeNormal(); 
                    if (PlaneNormal.IsNearlyZero()) { PlaneNormal = FVector::ForwardVector; }

                    PerformSlice(SlicedItemCandidate, PlanePosition, PlaneNormal); // Use the stored candidate
                }
            }
            else
            { 
                // Log the reason for aborting
                // FString AbortReason = TEXT("Unknown"); // 제거
                // if (!SlicedItemCandidate) AbortReason = TEXT("No valid item hit at start"); // 제거
                // else if (EndHitItemActor != SlicedItemCandidate) AbortReason = FString::Printf(TEXT("End hit different actor (%s) or not an item"), EndHitItemActor ? *EndHitItemActor->GetName() : TEXT("None")); // 제거
                // else if (SliceStartWorldLocation.IsNearlyZero() || SliceEndWorldLocation.IsNearlyZero()) AbortReason = TEXT("Start or End WorldLocation zero"); // 제거
                // UE_LOG(LogTemp, Warning, TEXT("Slice aborted - Reason: %s"), *AbortReason); // 제거
            }
        }
    }
    // else // 제거 (else 블록이 비어있게 되므로 제거)
    // { UE_LOG(LogTemp, Warning, TEXT("Slice End - Failed to get mouse position.")); } // 제거

    // Reset locations and dragging state AFTER the operation attempts
    // UE_LOG(LogTemp, Log, TEXT("Input_SliceEnd finished. Resetting bIsDraggingSlice = false.")); // 제거
    bIsDraggingSlice = false; 
    SliceStartWorldLocation = FVector::ZeroVector; 
    SliceEndWorldLocation = FVector::ZeroVector;

    // Reset candidate at the very end
    SlicedItemCandidate = nullptr;
}

// Placeholder for the actual slicing logic
void AWarriorHeroCharacter::PerformSlice(AInventoryItemActor* ItemToSlice, const FVector& PlanePosition, const FVector& PlaneNormal)
{
    if (!ItemToSlice)
    {
        return;
    }

    // Call the item's slicing function
    ItemToSlice->SliceItem(PlanePosition, PlaneNormal);

    // Check if the item was successfully marked as sliced after the attempt
    if (ItemToSlice->IsSliced())
    {
        // Play slicing visual effect (Niagara) if assigned
        if (SliceNiagaraEffect)
        {
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), SliceNiagaraEffect, PlanePosition, PlaneNormal.Rotation());
        }
        // Optional: Log if the deprecated particle system is still assigned
        // else if (SliceParticleEffect_DEPRECATED)
        // {
        //     UE_LOG(LogTemp, Warning, TEXT("Slice effect: Using deprecated Cascade particle system. Please assign a Niagara system to SliceNiagaraEffect."));
        //     UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SliceParticleEffect_DEPRECATED, PlanePosition, PlaneNormal.Rotation());
        // }

        // Play slicing sound effect if assigned
        if (SliceSound)
        {
            UGameplayStatics::PlaySoundAtLocation(GetWorld(), SliceSound, PlanePosition);
        }

        UE_LOG(LogTemp, Log, TEXT("Slice successful for %s. Played effects (Niagara: %s, Sound: %s)."), 
            *ItemToSlice->GetName(), 
            SliceNiagaraEffect ? TEXT("Yes") : TEXT("No"), 
            SliceSound ? TEXT("Yes") : TEXT("No"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Slice attempted for %s, but IsSliced() is still false."), *ItemToSlice->GetName());
    }
}

// Implementation for placing an item from the inventory onto the interactable table
void AWarriorHeroCharacter::PlaceItemOnTable(int32 SlotIndex)
{
	// UE_LOG(LogTemp, Log, TEXT("[PlaceItemOnTable] Function called for SlotIndex %d."), SlotIndex); // 제거

	// 1. Validate necessary components and state
	if (!InventoryComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("[PlaceItemOnTable] InventoryComponent is null."));
		return;
	}
	if (!CurrentInteractableTable)
	{
		// UE_LOG(LogTemp, Error, TEXT("[PlaceItemOnTable] CurrentInteractableTable is null. Cannot place item.")); // 에러 로그는 유지하거나 필요에 따라 제거
		return;
	}
	if (!DefaultItemActorClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[PlaceItemOnTable] DefaultItemActorClass is not set in Character Defaults. Cannot spawn item actor."));
		return;
	}
    // Check if SlotIndex is valid directly against the component's array
	if (!InventoryComponent->InventorySlots.IsValidIndex(SlotIndex))
	{
		UE_LOG(LogTemp, Error, TEXT("[PlaceItemOnTable] Invalid SlotIndex: %d"), SlotIndex);
		return;
	}

	// 2. Get the item data from the slot (make a copy)
	FSlotStruct ItemDataToPlace = InventoryComponent->InventorySlots[SlotIndex];

	// 3. Check if the slot is actually occupied
	if (ItemDataToPlace.ItemID.RowName.IsNone() || ItemDataToPlace.Quantity <= 0)
	{
		// UE_LOG(LogTemp, Warning, TEXT("[PlaceItemOnTable] Slot %d is empty. Cannot place."), SlotIndex); // 경고 로그는 유지하거나 필요에 따라 제거
		return;
	}

	// 4. Attempt to remove ONE item from the inventory slot
	if (InventoryComponent->RemoveItemFromSlot(SlotIndex, 1))
	{
		// UE_LOG(LogTemp, Log, TEXT("[PlaceItemOnTable] Successfully removed 1 item from slot %d."), SlotIndex); // 제거

		// 5. Determine spawn location and rotation on the table
		FVector SpawnLocation = CurrentInteractableTable->GetActorLocation(); // Default location
		FRotator SpawnRotation = CurrentInteractableTable->GetActorRotation();

		// Try to find the 'ItemSpawnPoint' component by name
		USceneComponent* SpawnPointComponent = Cast<USceneComponent>(CurrentInteractableTable->GetDefaultSubobjectByName(FName("ItemSpawnPoint"))); // Use GetDefaultSubobjectByName
		if (SpawnPointComponent)
		{
			SpawnLocation = SpawnPointComponent->GetComponentLocation();
			SpawnRotation = SpawnPointComponent->GetComponentRotation();
			// UE_LOG(LogTemp, Log, TEXT("[PlaceItemOnTable] Found ItemSpawnPoint component. Using its location: %s"), *SpawnLocation.ToString()); // 제거
		}
		else
		{
			// Fallback if the component wasn't found (e.g., BP not set up correctly)
			UE_LOG(LogTemp, Warning, TEXT("[PlaceItemOnTable] Could not find 'ItemSpawnPoint' component on table '%s'. Using table actor location + offset as fallback."), *CurrentInteractableTable->GetName());
            SpawnLocation = CurrentInteractableTable->GetActorLocation() + FVector(0, 0, 10.0f); // Reduced fallback offset
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = GetInstigator();
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		// 6. Spawn the item actor at the spawn point location
		AInventoryItemActor* PlacedActor = GetWorld()->SpawnActor<AInventoryItemActor>(DefaultItemActorClass, SpawnLocation, SpawnRotation, SpawnParams);

		if (PlacedActor)
		{
			// UE_LOG(LogTemp, Log, TEXT("[PlaceItemOnTable] Successfully spawned actor %s."), *PlacedActor->GetName()); // 제거

			// 7. Set up the spawned actor
			FSlotStruct SpawnedItemData = ItemDataToPlace; // Use the copied data
			SpawnedItemData.Quantity = 1; // We are placing only one item
			PlacedActor->SetItemData(SpawnedItemData);
			// PlacedActor->RequestEnablePhysics(); // 필요에 따라 물리 활성화 여부 결정
		}
		else
		{
			// UE_LOG(LogTemp, Error, TEXT("[PlaceItemOnTable] Failed to spawn Item Actor of class %s."), *DefaultItemActorClass->GetName()); // 에러 로그는 유지하거나 필요에 따라 제거
		}
	}
	else
	{
		// UE_LOG(LogTemp, Warning, TEXT("[PlaceItemOnTable] Failed to remove item from slot %d. Aborting placement."), SlotIndex); // 경고 로그는 유지하거나 필요에 따라 제거
	}
}

#pragma region Components Getters
// ... existing code ...

// Called every frame
void AWarriorHeroCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Add any per-frame logic here if needed
}
