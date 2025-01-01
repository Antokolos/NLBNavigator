// Copyright Epic Games, Inc. All Rights Reserved.

#include "NLBNavigatorCharacter.h"
#include "NLBNavigatorProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"

#include "NLBController.h"
#include "Kismet/GameplayStatics.h"

#include "InputMappingContext.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraActor.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// ANLBNavigatorCharacter

ANLBNavigatorCharacter::ANLBNavigatorCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
		
	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));

	VisualNovelController = nullptr;
	bIsInVisualNovelMode = false;

	// Load InputAction -- better do it from character BP
	/*static ConstructorHelpers::FObjectFinder<UInputAction> InputActionFinder(TEXT("/Game/NLB/Input/Actions/IA_NextPage"));
	if (InputActionFinder.Succeeded())
	{
		NextPageAction = InputActionFinder.Object;
	}*/
}

//////////////////////////////////////////////////////////////////////////// Input

void ANLBNavigatorCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void ANLBNavigatorCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ANLBNavigatorCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ANLBNavigatorCharacter::Look);

		// Bind the action to the SwitchToVisualNovel function
		EnhancedInputComponent->BindAction(SwitchToVisualNovelAction, ETriggerEvent::Triggered, this, &ANLBNavigatorCharacter::SwitchToVisualNovel);

		EnhancedInputComponent->BindAction(NextPageAction, ETriggerEvent::Triggered, this, &ANLBNavigatorCharacter::NextPage);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}

	// Attach the Input Mapping Context
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			// Load the Input Mapping Context from the asset created in the editor
			// Ensure the path is correct and matches your asset's path in the editor
			FString AssetPath = TEXT("/Game/NLB/Input/IMC_Default.IMC_Default");

			// Load the UInputMappingContext object at runtime
			NLBMappingContext = Cast<UInputMappingContext>(StaticLoadObject(UInputMappingContext::StaticClass(), nullptr, *AssetPath));
			// if (NLBMappingContext)
			// {
			// 	Subsystem->AddMappingContext(NLBMappingContext, 1);
			// 	UE_LOG(LogTemp, Log, TEXT("UInputMappingContext loaded successfully."));
			// }
			// else
			// {
			// 	UE_LOG(LogTemp, Error, TEXT("Failed to load UInputMappingContext."));
			// }
		}
	}
}


void ANLBNavigatorCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add movement 
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}
}

void ANLBNavigatorCharacter::Look(const FInputActionValue& Value)
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

void ANLBNavigatorCharacter::SwitchToVisualNovel()
{
	if (bIsInVisualNovelMode)
	{
		// Return to shooter mode if already in visual novel mode
		ReturnToShooter();
		return;
	}

	if (!VisualNovelController)
	{
		// Spawn the NLBController actor if it does not exist
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;

		VisualNovelController = GetWorld()->SpawnActor<ANLBController>(ANLBController::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

		if (!VisualNovelController)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to spawn NLBController."));
			return;
		}
	}

	// Switch the camera view to the NLBController
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (PlayerController && VisualNovelController)
	{
		PlayerController->SetViewTargetWithBlend(VisualNovelController, 1.0f);
		VisualNovelController->AddToViewport();
		VisualNovelController->LoadCurrentSet();
		DisableCharacterInput();
		ActivateVisualNovelInput();
		bIsInVisualNovelMode = true;
	}
}

void ANLBNavigatorCharacter::ReturnToShooter()
{
	// Switch the camera view back to the first-person character
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (PlayerController)
	{
		PlayerController->SetViewTargetWithBlend(this, 0.0f);
		bIsInVisualNovelMode = false;
	}
	if (VisualNovelController)
	{
		VisualNovelController->RemoveFromViewport();
	}
	EnableCharacterInput();
	DeactivateVisualNovelInput();
}

void ANLBNavigatorCharacter::NextPage()
{
	if (VisualNovelController && NextPageAction)
	{
		VisualNovelController->LoadNextSet();
	}
}

//void ANLBNavigatorCharacter::LoadSwitchToVisualNovelAction()
//{
//	// Ensure the path is correct and matches your asset's path in the editor
//	FString AssetPath = TEXT("/Game/NLB/Input/Actions/IA_SwitchToVN.IA_SwitchToVN");
//
//	// Load the UInputAction object at runtime
//	UInputAction* LoadedAction = Cast<UInputAction>(StaticLoadObject(UInputAction::StaticClass(), nullptr, *AssetPath));
//	if (LoadedAction)
//	{
//		SwitchToVisualNovelAction = LoadedAction;
//		UE_LOG(LogTemp, Log, TEXT("SwitchToVisualNovelAction loaded successfully."));
//	}
//	else
//	{
//		UE_LOG(LogTemp, Error, TEXT("Failed to load SwitchToVisualNovelAction."));
//	}
//}

void ANLBNavigatorCharacter::ActivateVisualNovelInput()
{
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = GetEnhancedInputSubsystem())
	{
		if (NLBMappingContext)
		{
			Subsystem->AddMappingContext(NLBMappingContext, 1); // Priority = 1
		}
	}
}

void ANLBNavigatorCharacter::DeactivateVisualNovelInput()
{
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = GetEnhancedInputSubsystem())
	{
		if (NLBMappingContext)
		{
			Subsystem->RemoveMappingContext(NLBMappingContext);
		}
	}
}

void ANLBNavigatorCharacter::DisableCharacterInput()
{
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = GetEnhancedInputSubsystem())
	{
		if (DefaultMappingContext)
		{
			Subsystem->RemoveMappingContext(DefaultMappingContext);
		}
	}
}

void ANLBNavigatorCharacter::EnableCharacterInput()
{
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = GetEnhancedInputSubsystem())
	{
		if (DefaultMappingContext)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0); // Priority = 0
		}
	}
}

UEnhancedInputLocalPlayerSubsystem* ANLBNavigatorCharacter::GetEnhancedInputSubsystem() const
{
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
		{
			return LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
		}
	}
	return nullptr;
}
