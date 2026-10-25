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

#include "InteractableInterface.h"
#include "PLDInteractableActor.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "IXRTrackingSystem.h"
#include "DrawDebugHelpers.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "TimerManager.h"
#include "NLBVRModeComponent.h"
#include "NLBPauseMenuWidget.h"
#include "NLBTouchControlsComponent.h"
#include "NLBNavigatorWeaponComponent.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// ANLBNavigatorCharacter

ANLBNavigatorCharacter::ANLBNavigatorCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
		
	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	VRMode = CreateDefaultSubobject<UNLBVRModeComponent>(TEXT("VRMode"));
	TouchControls = CreateDefaultSubobject<UNLBTouchControlsComponent>(TEXT("TouchControls"));
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

void ANLBNavigatorCharacter::BeginPlay()
{
	Super::BeginPlay();
	VRMode->OnInteract.BindUObject(this, &ANLBNavigatorCharacter::OnVRInteract);
	VRMode->OnPause.BindUObject(this, &ANLBNavigatorCharacter::TogglePauseMenu);
	// VR — по ключу -vr или по сохранённому выбору; шлем к первому кадру может быть не готов,
	// поэтому включаем на следующем такте
	bool bWantVR = FParse::Param(FCommandLine::Get(), TEXT("vr"));
#if PLATFORM_ANDROID
	// Android: сборка для шлема (Quest, с OpenXR) — только VR; сборка для телефона (без OpenXR,
	// tools/ue/package_android.ps1 -Target phone) — обычный режим с экранным управлением
	bWantVR = GEngine && GEngine->XRSystem.IsValid();
#endif
	if (!bWantVR && GConfig)
	{
		GConfig->GetBool(TEXT("NLBNavigator"), TEXT("VRMode"), bWantVR, GGameUserSettingsIni);
	}
	if (bWantVR)
	{
		GetWorldTimerManager().SetTimerForNextTick([this]() {
			FString Error;
			if (!VRMode->SetVRMode(true, Error))
			{
				UE_LOG(LogTemplateCharacter, Warning, TEXT("VR: %s"), *Error);
			}
			UpdateTouchControls();
		});
	}

	// Экранное управление (телефон): кнопки вызывают то же, что клавиши и геймпад
	TouchControls->OnMenu = [this]() { TogglePauseMenu(); };
	TouchControls->OnInteract = [this]() { Interact(); };
	TouchControls->OnJumpStart = [this]() { Jump(); };
	TouchControls->OnJumpStop = [this]() { StopJumping(); };
	// оружие шаблона — компонент подобранного актора, прикреплённого к персонажу
	auto FindWeapon = [this]() -> UNLBNavigatorWeaponComponent* {
		TArray<AActor*> Attached;
		GetAttachedActors(Attached);
		for (AActor* A : Attached)
		{
			if (UNLBNavigatorWeaponComponent* W = A ? A->FindComponentByClass<UNLBNavigatorWeaponComponent>() : nullptr) return W;
		}
		return FindComponentByClass<UNLBNavigatorWeaponComponent>();
	};
	TouchControls->CanFire = [FindWeapon]() { return FindWeapon() != nullptr; };
	TouchControls->OnFire = [FindWeapon]() { if (UNLBNavigatorWeaponComponent* W = FindWeapon()) W->Fire(); };
	TouchControls->OnLeaveBook = [this]() { if (bIsInVisualNovelMode) ReturnToShooter(); };
	GetWorldTimerManager().SetTimerForNextTick([this]() { UpdateTouchControls(); });
}

void ANLBNavigatorCharacter::UpdateTouchControls()
{
	if (!TouchControls)
	{
		return;
	}
	using EMode = UNLBTouchControlsComponent::EMode;
	if (IsVRMode() || PauseMenu)
	{
		TouchControls->SetMode(EMode::Hidden);  // в VR — контроллеры; меню паузы — касаниями
	}
	else
	{
		TouchControls->SetMode(bIsInVisualNovelMode ? EMode::Book : EMode::Shooter);
	}
}

bool ANLBNavigatorCharacter::IsVRMode() const
{
	return VRMode && VRMode->IsVRActive();
}

bool ANLBNavigatorCharacter::SetVRMode(bool bEnable, FString& OutError)
{
	if (bIsInVisualNovelMode)
	{
		ReturnToShooter();  // книга откроется заново уже в новом режиме
	}
	if (!VRMode->SetVRMode(bEnable, OutError))
	{
		return false;
	}
	UpdateTouchControls();
	if (GConfig)
	{
		GConfig->SetBool(TEXT("NLBNavigator"), TEXT("VRMode"), bEnable, GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}
	return true;
}

void ANLBNavigatorCharacter::OnVRInteract(AActor* Target)
{
	if (bIsInVisualNovelMode)
	{
		return;
	}
	if (IInteractableInterface* I = Cast<IInteractableInterface>(Target))
	{
		I->Interact(this);
	}
}

void ANLBNavigatorCharacter::TogglePauseMenu()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || bIsInVisualNovelMode)
	{
		return;  // в книге Esc — её собственное меню
	}
	if (PauseMenu)
	{
		// закрыть
		if (VRMode->IsVRActive()) VRMode->HidePanel();
		PauseMenu->RemoveFromParent();
		PauseMenu = nullptr;
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
		EnableCharacterInput();
		return;
	}
	PauseMenu = CreateWidget<UNLBPauseMenuWidget>(PC, UNLBPauseMenuWidget::StaticClass());
	if (!PauseMenu)
	{
		return;
	}
	PauseMenu->Owner = this;
	PauseMenu->Refresh();
	DisableCharacterInput();
	if (VRMode->IsVRActive())
	{
		VRMode->ShowPanel(PauseMenu);  // в шлеме экран окна не виден — меню на панели перед игроком
	}
	else
	{
		PauseMenu->AddToViewport(100);
		PC->SetShowMouseCursor(true);
		FInputModeUIOnly Mode;
		Mode.SetWidgetToFocus(PauseMenu->TakeWidget());
		PC->SetInputMode(Mode);
	}
}

void ANLBNavigatorCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
			if (PauseContext)
			{
				Subsystem->AddMappingContext(PauseContext, 3);
			}
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
		
		EnhancedInputComponent->BindAction(ToggleViewAction, ETriggerEvent::Triggered, this, &ANLBNavigatorCharacter::ToggleView);

		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Triggered, this, &ANLBNavigatorCharacter::Interact);

		// Режим VR: свои действия контроллеров (контекст ввода включается вместе с режимом)
		VRMode->SetupInput(EnhancedInputComponent);

		// Меню паузы: Esc или P (в редакторе Esc останавливает игру — там удобнее P)
		if (!PauseAction)
		{
			PauseAction = NewObject<UInputAction>(this);
			PauseAction->ValueType = EInputActionValueType::Boolean;
			PauseContext = NewObject<UInputMappingContext>(this);
			PauseContext->MapKey(PauseAction, EKeys::Escape);
			PauseContext->MapKey(PauseAction, EKeys::P);
		}
		EnhancedInputComponent->BindAction(PauseAction, ETriggerEvent::Started, this, &ANLBNavigatorCharacter::TogglePauseMenu);
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
		// Книга и режим — из настроек персонажа (детали BP_FirstPersonCharacter, категория NLB):
		// контроллер создаётся из C++-класса, своего Blueprint у него нет
		VisualNovelController->bVisualNovel = bBookVisualNovel;
		if (!BookPath.IsEmpty())
		{
			VisualNovelController->BookPath = BookPath;
		}
	}

	// Switch the camera view to the NLBController (в VR — нет: смотрим через шлем, книга на панели)
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (PlayerController && VisualNovelController)
	{
		if (!IsVRMode())
		{
			PlayerController->SetViewTargetWithBlend(VisualNovelController, 0.0f);
		}
		VisualNovelController->AddToViewport();
		VisualNovelController->LoadCurrentSet();
		DisableCharacterInput();
		ActivateVisualNovelInput();
		bIsInVisualNovelMode = true;
		UpdateTouchControls();  // экранное управление — кнопка «В игру»
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
		VisualNovelController->RemoveFromParent();
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

void ANLBNavigatorCharacter::ToggleView()
{
	if (VisualNovelController && ToggleViewAction)
	{
		VisualNovelController->ToggleView();
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
	UpdateTouchControls();  // книга или меню паузы — джойстики прочь
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
	UpdateTouchControls();
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

void ANLBNavigatorCharacter::Interact()
{
    if (CurrentInteractable)
    {
        Cast<APLDInteractableActor>(CurrentInteractable)->Interact(this);
    }
}

void ANLBNavigatorCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    // В VR предметы выбирает луч правой руки (UNLBVRModeComponent), а не взгляд камеры
    if (IsVRMode())
    {
        if (CurrentInteractable)
        {
            Cast<APLDInteractableActor>(CurrentInteractable)->ShowInteractionWidget(false);
            CurrentInteractable = nullptr;
        }
        return;
    }

    // Line Trace logic to find PLDInteractableActor
    FVector Start = FirstPersonCameraComponent->GetComponentLocation();
    FVector End = Start + (FirstPersonCameraComponent->GetForwardVector() * InteractionRange);

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        AActor* HitActor = Hit.GetActor();
        if (HitActor && HitActor->IsA<APLDInteractableActor>())
        {
            if (HitActor != CurrentInteractable)
            {
                if (CurrentInteractable)
                {
                    Cast<APLDInteractableActor>(CurrentInteractable)->ShowInteractionWidget(false);
                }
                CurrentInteractable = HitActor;
                Cast<APLDInteractableActor>(HitActor)->ShowInteractionWidget(true);
				
    			//DrawDebugLine(GetWorld(), Start, End, FColor::Green, false, 1.0f, 0, 1.0f);
            }
			else
			{
				//DrawDebugLine(GetWorld(), Start, End, FColor::Yellow, false, 1.0f, 0, 1.0f);
			}
			return;
        }
		else
		{
			//DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 1.0f, 0, 1.0f);
		}
    }

    if (CurrentInteractable)
    {
		//DrawDebugLine(GetWorld(), Start, End, FColor::Green, false, 1.0f, 0, 1.0f);
        Cast<APLDInteractableActor>(CurrentInteractable)->ShowInteractionWidget(false);
        CurrentInteractable = nullptr;
	}
}
