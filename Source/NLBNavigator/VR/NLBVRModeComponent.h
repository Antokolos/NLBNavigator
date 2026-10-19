// Режим VR для персонажа шутера: тот же персонаж ходит по той же карте, но смотрит через шлем
// и действует руками (контроллеры OpenXR, например Quest по Link). Компонент создаёт всё нужное
// для VR при включении режима и убирает при выключении — в обычном режиме персонаж прежний.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NLBVRModeComponent.generated.h"

class ACharacter;
class UCameraComponent;
class UEnhancedInputComponent;
class UInputAction;
class UInputMappingContext;
class UMotionControllerComponent;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UUserWidget;
class UWidgetComponent;
class UWidgetInteractionComponent;
class ANLBGrabbableActor;
struct FInputActionValue;

UCLASS(ClassGroup = (NLB), meta = (BlueprintSpawnableComponent))
class NLBNAVIGATOR_API UNLBVRModeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNLBVRModeComponent();

	/** Меш руки (по умолчанию — маленькая сфера движка; можно поставить руки из MannequinsXR) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR")
	TObjectPtr<UStaticMesh> HandMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR")
	FVector HandScale = FVector(0.08f);

	/** Дальность луча правой руки: взаимодействие с предметами и щелчки по панелям */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR")
	float PointerRange = 2000.0f;

	/** Радиус захвата предметов рукой (кнопка Grip) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR")
	float GrabRadius = 20.0f;

	/** Панель (книга, меню паузы): расстояние перед игроком и ширина, см */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR")
	float PanelDistance = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR")
	float PanelWidth = 160.0f;

	/** Поворот рывками, градусов */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR")
	float SnapTurnDegrees = 45.0f;

	/** Включить или выключить VR; false — не получилось (OutError: почему) */
	bool SetVRMode(bool bEnable, FString& OutError);
	bool IsVRActive() const { return bVRActive; }

	/** Привязка действий VR к вводу персонажа (вызывается из SetupPlayerInputComponent) */
	void SetupInput(UEnhancedInputComponent* Input);

	/** Показать виджет на панели перед игроком (в VR экран окна не виден) / убрать */
	void ShowPanel(UUserWidget* Widget);
	void HidePanel();
	bool IsPanelShown() const;

	/** Действие персонажа на предмете под лучом (APLDInteractableActor и др.) */
	DECLARE_DELEGATE_OneParam(FOnInteract, AActor*);
	FOnInteract OnInteract;
	/** Кнопка меню контроллера */
	DECLARE_DELEGATE(FOnPause);
	FOnPause OnPause;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void CreateRig();
	void DestroyRig();
	void CreateInputAssets();

	void OnMove(const FInputActionValue& Value);
	void OnTurn(const FInputActionValue& Value);
	void OnTriggerPressed();
	void OnTriggerReleased();
	void OnGrip(bool bLeft);
	void OnRelease(bool bLeft);
	void OnNext();
	void OnBack();
	void OnMenu();

	ACharacter* Character() const;

	bool bVRActive = false;
	bool bTurnLatched = false;  ///< рывок поворота — один на одно отклонение стика

	// Камера персонажа: в VR следует за шлемом от уровня пола
	UPROPERTY() TObjectPtr<UCameraComponent> Camera;
	UPROPERTY() TObjectPtr<USceneComponent> CameraParent;
	FTransform CameraRelative;
	FName CameraSocket;

	UPROPERTY() TObjectPtr<USceneComponent> VROrigin;
	UPROPERTY() TObjectPtr<UMotionControllerComponent> LeftController;
	UPROPERTY() TObjectPtr<UMotionControllerComponent> RightController;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> LeftHand;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> RightHand;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Laser;
	UPROPERTY() TObjectPtr<UWidgetInteractionComponent> Pointer;
	UPROPERTY() TObjectPtr<UWidgetComponent> Panel;

	UPROPERTY() TObjectPtr<AActor> PointedInteractable;
	UPROPERTY() TObjectPtr<ANLBGrabbableActor> GrabbedLeft;
	UPROPERTY() TObjectPtr<ANLBGrabbableActor> GrabbedRight;

	// Ввод VR создаётся в коде (ассеты не нужны): Oculus Touch / контроллеры OpenXR
	UPROPERTY() TObjectPtr<UInputMappingContext> VRContext;
	UPROPERTY() TObjectPtr<UInputAction> MoveAction;
	UPROPERTY() TObjectPtr<UInputAction> TurnAction;
	UPROPERTY() TObjectPtr<UInputAction> TriggerAction;
	UPROPERTY() TObjectPtr<UInputAction> GripLeftAction;
	UPROPERTY() TObjectPtr<UInputAction> GripRightAction;
	UPROPERTY() TObjectPtr<UInputAction> NextAction;
	UPROPERTY() TObjectPtr<UInputAction> BackAction;
	UPROPERTY() TObjectPtr<UInputAction> MenuAction;
};
