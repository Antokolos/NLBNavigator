#include "NLBVRModeComponent.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/WidgetInteractionComponent.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "IXRTrackingSystem.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "MotionControllerComponent.h"
#include "UObject/ConstructorHelpers.h"

#include "InteractableInterface.h"
#include "NLBGrabbableActor.h"
#include "NLBScreenWidget.h"

UNLBVRModeComponent::UNLBVRModeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (Sphere.Succeeded())
	{
		HandMesh = Sphere.Object;
	}
}

ACharacter* UNLBVRModeComponent::Character() const
{
	return Cast<ACharacter>(GetOwner());
}

// ============================================================================ включение и выключение

bool UNLBVRModeComponent::SetVRMode(bool bEnable, FString& OutError)
{
	if (bEnable == bVRActive)
	{
		return true;
	}
	ACharacter* Owner = Character();
	if (!Owner)
	{
		return false;
	}
	if (bEnable)
	{
		if (!GEngine || !GEngine->XRSystem.IsValid() || !UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayConnected())
		{
			OutError = TEXT("Шлем не подключён: нужен Quest по Link или Air Link и включённый OpenXR.");
			return false;
		}
		if (!UHeadMountedDisplayFunctionLibrary::EnableHMD(true))
		{
			OutError = TEXT("Не удалось включить шлем.");
			return false;
		}
		UHeadMountedDisplayFunctionLibrary::SetTrackingOrigin(EHMDTrackingOrigin::LocalFloor);
		CreateRig();
		if (APlayerController* PC = Cast<APlayerController>(Owner->GetController()))
		{
			if (UEnhancedInputLocalPlayerSubsystem* Sub = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
			{
				if (VRContext) Sub->AddMappingContext(VRContext, 2);
			}
		}
		bVRActive = true;
	}
	else
	{
		HidePanel();
		if (GrabbedLeft) { GrabbedLeft->Release(); GrabbedLeft = nullptr; }
		if (GrabbedRight) { GrabbedRight->Release(); GrabbedRight = nullptr; }
		if (APlayerController* PC = Cast<APlayerController>(Owner->GetController()))
		{
			if (UEnhancedInputLocalPlayerSubsystem* Sub = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
			{
				if (VRContext) Sub->RemoveMappingContext(VRContext);
			}
		}
		DestroyRig();
		UHeadMountedDisplayFunctionLibrary::EnableHMD(false);
		bVRActive = false;
	}
	return true;
}

void UNLBVRModeComponent::CreateRig()
{
	ACharacter* Owner = Character();
	USceneComponent* Root = Owner->GetCapsuleComponent();
	// Начало координат VR — на уровне пола под капсулой: шлем даёт рост игрока сам (LocalFloor)
	VROrigin = NewObject<USceneComponent>(Owner, TEXT("NLB_VROrigin"));
	VROrigin->SetupAttachment(Root);
	VROrigin->SetRelativeLocation(FVector(0, 0, -Owner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
	VROrigin->RegisterComponent();

	// Камера персонажа переходит к началу VR и следует за шлемом; руки и оружие шутера прячем
	Camera = Owner->FindComponentByClass<UCameraComponent>();
	if (Camera)
	{
		CameraParent = Camera->GetAttachParent();
		CameraSocket = Camera->GetAttachSocketName();
		CameraRelative = Camera->GetRelativeTransform();
		Camera->AttachToComponent(VROrigin, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		Camera->SetRelativeTransform(FTransform::Identity);
		Camera->bUsePawnControlRotation = false;
		Camera->bLockToHmd = true;
		TArray<USceneComponent*> Children;
		Camera->GetChildrenComponents(true, Children);
		for (USceneComponent* C : Children)
		{
			if (Cast<UMeshComponent>(C)) C->SetVisibility(false);
		}
	}

	auto MakeController = [&](const TCHAR* Name, FName Source) {
		UMotionControllerComponent* MC = NewObject<UMotionControllerComponent>(Owner, Name);
		MC->SetupAttachment(VROrigin);
		MC->SetTrackingMotionSource(Source);
		MC->RegisterComponent();
		return MC;
	};
	LeftController = MakeController(TEXT("NLB_LeftController"), FName(TEXT("Left")));
	RightController = MakeController(TEXT("NLB_RightController"), FName(TEXT("Right")));

	auto MakeHand = [&](UMotionControllerComponent* Parent, const TCHAR* Name) {
		UStaticMeshComponent* Hand = NewObject<UStaticMeshComponent>(Owner, Name);
		Hand->SetupAttachment(Parent);
		Hand->SetStaticMesh(HandMesh);
		Hand->SetRelativeScale3D(HandScale);
		Hand->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Hand->RegisterComponent();
		return Hand;
	};
	LeftHand = MakeHand(LeftController, TEXT("NLB_LeftHand"));
	RightHand = MakeHand(RightController, TEXT("NLB_RightHand"));

	// Луч правой руки: тонкий цилиндр вперёд
	Laser = NewObject<UStaticMeshComponent>(Owner, TEXT("NLB_Laser"));
	Laser->SetupAttachment(RightController);
	if (UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")))
	{
		Laser->SetStaticMesh(Cylinder);
	}
	Laser->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Laser->SetCastShadow(false);
	Laser->RegisterComponent();

	// Щелчки по панелям (книга, меню паузы) — тем же лучом, как мышью
	Pointer = NewObject<UWidgetInteractionComponent>(Owner, TEXT("NLB_Pointer"));
	Pointer->SetupAttachment(RightController);
	Pointer->InteractionDistance = PointerRange;
	Pointer->InteractionSource = EWidgetInteractionSource::World;
	Pointer->bShowDebug = false;
	Pointer->RegisterComponent();
}

void UNLBVRModeComponent::DestroyRig()
{
	ACharacter* Owner = Character();
	if (Camera)
	{
		Camera->AttachToComponent(CameraParent ? CameraParent.Get() : Owner->GetCapsuleComponent(),
		                          FAttachmentTransformRules::KeepRelativeTransform, CameraSocket);
		Camera->SetRelativeTransform(CameraRelative);
		Camera->bUsePawnControlRotation = true;
		Camera->bLockToHmd = false;
		TArray<USceneComponent*> Children;
		Camera->GetChildrenComponents(true, Children);
		for (USceneComponent* C : Children)
		{
			if (Cast<UMeshComponent>(C)) C->SetVisibility(true);
		}
	}
	if (PointedInteractable)
	{
		if (IInteractableInterface* I = Cast<IInteractableInterface>(PointedInteractable)) I->ShowInteractionWidget(false);
		PointedInteractable = nullptr;
	}
	for (USceneComponent* C : {static_cast<USceneComponent*>(Pointer.Get()), static_cast<USceneComponent*>(Laser.Get()),
	                           static_cast<USceneComponent*>(LeftHand.Get()), static_cast<USceneComponent*>(RightHand.Get()),
	                           static_cast<USceneComponent*>(LeftController.Get()), static_cast<USceneComponent*>(RightController.Get()),
	                           static_cast<USceneComponent*>(VROrigin.Get())})
	{
		if (C) C->DestroyComponent();
	}
	Pointer = nullptr; Laser = nullptr; LeftHand = nullptr; RightHand = nullptr;
	LeftController = nullptr; RightController = nullptr; VROrigin = nullptr;
}

// ============================================================================ панель перед игроком

void UNLBVRModeComponent::ShowPanel(UUserWidget* Widget)
{
	ACharacter* Owner = Character();
	if (!Owner || !Widget)
	{
		return;
	}
	if (!Panel)
	{
		Panel = NewObject<UWidgetComponent>(Owner, TEXT("NLB_Panel"));
		Panel->SetUsingAbsoluteLocation(true);
		Panel->SetUsingAbsoluteRotation(true);
		Panel->SetUsingAbsoluteScale(true);
		Panel->SetWidgetSpace(EWidgetSpace::World);
		Panel->SetDrawSize(FVector2D(1920, 1080));
		Panel->SetPivot(FVector2D(0.5f, 0.5f));
		Panel->SetTwoSided(true);
		Panel->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Panel->SetCollisionResponseToAllChannels(ECR_Ignore);
		Panel->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		Panel->SetupAttachment(Owner->GetRootComponent());
		Panel->RegisterComponent();
	}
	Panel->SetWidget(Widget);
	// Перед игроком на уровне глаз, лицом к нему; ширина PanelWidth (экран 1920 px)
	const FVector Eye = Camera ? Camera->GetComponentLocation() : Owner->GetActorLocation();
	const FRotator Yaw(0, Camera ? Camera->GetComponentRotation().Yaw : Owner->GetActorRotation().Yaw, 0);
	Panel->SetWorldLocation(Eye + Yaw.Vector() * PanelDistance);
	Panel->SetWorldRotation(FRotator(0, Yaw.Yaw + 180.0f, 0));
	Panel->SetWorldScale3D(FVector(PanelWidth / 1920.0f));
	Panel->SetVisibility(true);
}

void UNLBVRModeComponent::HidePanel()
{
	if (Panel)
	{
		Panel->SetWidget(nullptr);
		Panel->DestroyComponent();
		Panel = nullptr;
	}
}

bool UNLBVRModeComponent::IsPanelShown() const
{
	return Panel != nullptr;
}

// ============================================================================ ввод

void UNLBVRModeComponent::CreateInputAssets()
{
	if (VRContext)
	{
		return;
	}
	auto MakeAction = [this](EInputActionValueType Type) {
		UInputAction* A = NewObject<UInputAction>(this);
		A->ValueType = Type;
		return A;
	};
	VRContext = NewObject<UInputMappingContext>(this);
	MoveAction = MakeAction(EInputActionValueType::Axis2D);
	TurnAction = MakeAction(EInputActionValueType::Axis1D);
	TriggerAction = MakeAction(EInputActionValueType::Boolean);
	GripLeftAction = MakeAction(EInputActionValueType::Boolean);
	GripRightAction = MakeAction(EInputActionValueType::Boolean);
	NextAction = MakeAction(EInputActionValueType::Boolean);
	BackAction = MakeAction(EInputActionValueType::Boolean);
	MenuAction = MakeAction(EInputActionValueType::Boolean);
	// Контроллеры Oculus Touch (Quest по Link): левый стик — ходьба, правый — поворот рывками,
	// правый курок — действие/щелчок, Grip — взять, A — «дальше» в книге, B — назад/меню книги,
	// кнопка меню левого контроллера — меню паузы
	VRContext->MapKey(MoveAction, EKeys::OculusTouch_Left_Thumbstick_2D);
	VRContext->MapKey(TurnAction, EKeys::OculusTouch_Right_Thumbstick_X);
	VRContext->MapKey(TriggerAction, EKeys::OculusTouch_Right_Trigger_Click);
	VRContext->MapKey(GripLeftAction, EKeys::OculusTouch_Left_Grip_Click);
	VRContext->MapKey(GripRightAction, EKeys::OculusTouch_Right_Grip_Click);
	VRContext->MapKey(NextAction, EKeys::OculusTouch_Right_A_Click);
	VRContext->MapKey(BackAction, EKeys::OculusTouch_Right_B_Click);
	VRContext->MapKey(MenuAction, EKeys::OculusTouch_Left_Menu_Click);
}

void UNLBVRModeComponent::SetupInput(UEnhancedInputComponent* Input)
{
	if (!Input)
	{
		return;
	}
	CreateInputAssets();
	Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &UNLBVRModeComponent::OnMove);
	Input->BindAction(TurnAction, ETriggerEvent::Triggered, this, &UNLBVRModeComponent::OnTurn);
	Input->BindAction(TurnAction, ETriggerEvent::Completed, this, &UNLBVRModeComponent::OnTurn);
	Input->BindAction(TriggerAction, ETriggerEvent::Started, this, &UNLBVRModeComponent::OnTriggerPressed);
	Input->BindAction(TriggerAction, ETriggerEvent::Completed, this, &UNLBVRModeComponent::OnTriggerReleased);
	Input->BindAction(GripLeftAction, ETriggerEvent::Started, this, &UNLBVRModeComponent::OnGrip, true);
	Input->BindAction(GripLeftAction, ETriggerEvent::Completed, this, &UNLBVRModeComponent::OnRelease, true);
	Input->BindAction(GripRightAction, ETriggerEvent::Started, this, &UNLBVRModeComponent::OnGrip, false);
	Input->BindAction(GripRightAction, ETriggerEvent::Completed, this, &UNLBVRModeComponent::OnRelease, false);
	Input->BindAction(NextAction, ETriggerEvent::Started, this, &UNLBVRModeComponent::OnNext);
	Input->BindAction(BackAction, ETriggerEvent::Started, this, &UNLBVRModeComponent::OnBack);
	Input->BindAction(MenuAction, ETriggerEvent::Started, this, &UNLBVRModeComponent::OnMenu);
}

void UNLBVRModeComponent::OnMove(const FInputActionValue& Value)
{
	ACharacter* Owner = Character();
	if (!bVRActive || !Owner || IsPanelShown())
	{
		return;  // пока открыта книга или меню — стоим
	}
	// вперёд — туда, куда смотрит шлем (по горизонтали)
	const FVector2D V = Value.Get<FVector2D>();
	const FRotator Yaw(0, Camera ? Camera->GetComponentRotation().Yaw : Owner->GetActorRotation().Yaw, 0);
	Owner->AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), V.Y);
	Owner->AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), V.X);
}

void UNLBVRModeComponent::OnTurn(const FInputActionValue& Value)
{
	ACharacter* Owner = Character();
	const float X = Value.Get<float>();
	if (FMath::Abs(X) < 0.3f)
	{
		bTurnLatched = false;
		return;
	}
	if (!bVRActive || !Owner || bTurnLatched || FMath::Abs(X) < 0.7f)
	{
		return;
	}
	bTurnLatched = true;
	if (AController* C = Owner->GetController())
	{
		FRotator R = C->GetControlRotation();
		R.Yaw += X > 0 ? SnapTurnDegrees : -SnapTurnDegrees;
		C->SetControlRotation(R);
	}
}

void UNLBVRModeComponent::OnTriggerPressed()
{
	if (!bVRActive)
	{
		return;
	}
	// по панели — щелчок как мышью; иначе — действие с предметом под лучом
	if (Pointer && Pointer->IsOverHitTestVisibleWidget())
	{
		Pointer->PressPointerKey(EKeys::LeftMouseButton);
	}
	else if (PointedInteractable && OnInteract.IsBound())
	{
		OnInteract.Execute(PointedInteractable);
	}
}

void UNLBVRModeComponent::OnTriggerReleased()
{
	if (bVRActive && Pointer)
	{
		Pointer->ReleasePointerKey(EKeys::LeftMouseButton);
	}
}

void UNLBVRModeComponent::OnGrip(bool bLeft)
{
	UStaticMeshComponent* Hand = bLeft ? LeftHand : RightHand;
	TObjectPtr<ANLBGrabbableActor>& Held = bLeft ? GrabbedLeft : GrabbedRight;
	if (!bVRActive || !Hand || Held)
	{
		return;
	}
	// ближайший предмет, до области захвата которого дотягивается рука
	TArray<FOverlapResult> Overlaps;
	const FVector At = Hand->GetComponentLocation();
	GetWorld()->OverlapMultiByObjectType(Overlaps, At, FQuat::Identity,
	                                     FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllObjects),
	                                     FCollisionShape::MakeSphere(GrabRadius));
	ANLBGrabbableActor* Best = nullptr;
	float BestDist = TNumericLimits<float>::Max();
	for (const FOverlapResult& O : Overlaps)
	{
		ANLBGrabbableActor* G = Cast<ANLBGrabbableActor>(O.GetActor());
		if (!G || G->bIsGrabbed) continue;
		const float D = FVector::Dist(At, G->GetActorLocation());
		if (D < BestDist) { BestDist = D; Best = G; }
	}
	if (Best)
	{
		Best->Grab(Hand);
		Held = Best;
		Hand->SetVisibility(false);
	}
}

void UNLBVRModeComponent::OnRelease(bool bLeft)
{
	UStaticMeshComponent* Hand = bLeft ? LeftHand : RightHand;
	TObjectPtr<ANLBGrabbableActor>& Held = bLeft ? GrabbedLeft : GrabbedRight;
	if (Held)
	{
		Held->Release();
		Held = nullptr;
	}
	if (Hand) Hand->SetVisibility(true);
}

void UNLBVRModeComponent::OnNext()
{
	if (!bVRActive || !Panel) return;
	if (UNLBScreenWidget* Book = Cast<UNLBScreenWidget>(Panel->GetWidget())) Book->NextPage();
}

void UNLBVRModeComponent::OnBack()
{
	if (!bVRActive || !Panel) return;
	if (UNLBScreenWidget* Book = Cast<UNLBScreenWidget>(Panel->GetWidget())) Book->Back();
	else OnMenu();  // другая панель (меню паузы) — закрыть
}

void UNLBVRModeComponent::OnMenu()
{
	if (bVRActive && OnPause.IsBound()) OnPause.Execute();
}

// ============================================================================ луч правой руки

void UNLBVRModeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bVRActive || !RightController)
	{
		return;
	}
	const FVector Start = RightController->GetComponentLocation();
	const FVector Dir = RightController->GetForwardVector();
	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetOwner());
	float Length = PointerRange;
	AActor* Found = nullptr;
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, Start + Dir * PointerRange, ECC_Visibility, Params))
	{
		Length = Hit.Distance;
		AActor* A = Hit.GetActor();
		if (A && A->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
		{
			Found = A;
		}
	}
	// панель книги или меню — тоже цель луча
	if (Pointer && Pointer->IsOverHitTestVisibleWidget())
	{
		Length = FMath::Min(Length, Pointer->GetLastHitResult().Distance);
		Found = nullptr;
	}
	// подсказка «E: Interact» — у предмета под лучом
	if (Found != PointedInteractable)
	{
		if (IInteractableInterface* Old = Cast<IInteractableInterface>(PointedInteractable)) Old->ShowInteractionWidget(false);
		if (IInteractableInterface* New = Cast<IInteractableInterface>(Found)) New->ShowInteractionWidget(true);
		PointedInteractable = Found;
	}
	// цилиндр движка — 100 см по оси Z: поворачиваем вдоль луча и растягиваем на его длину
	if (Laser)
	{
		Laser->SetRelativeRotation(FRotator(-90, 0, 0));
		Laser->SetRelativeLocation(FVector(Length * 0.5f, 0, 0));
		Laser->SetRelativeScale3D(FVector(0.005f, 0.005f, Length / 100.0f));
	}
}
