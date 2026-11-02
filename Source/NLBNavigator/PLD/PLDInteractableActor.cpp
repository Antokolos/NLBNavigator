#include "PLDInteractableActor.h"
#include "PLDInteractionWidget.h"
#include "NLBNavigatorCharacter.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"

APLDInteractableActor::APLDInteractableActor()
{
    PrimaryActorTick.bCanEverTick = true; // Enable ticking for this actor

    // Создаём и настраиваем статическую меш-компоненту
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    RootComponent = MeshComponent;

    // Задаём меш по умолчанию (опционально)
    // static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("/Game/PathToYourMesh.YourMesh"));
    // if (MeshAsset.Succeeded())
    // {
    //     MeshComponent->SetStaticMesh(MeshAsset.Object);
    // }
    InteractionWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("InteractionWidget"));
    InteractionWidget->SetupAttachment(RootComponent);

    static ConstructorHelpers::FClassFinder<UPLDInteractionWidget> WidgetClass(TEXT("/Game/PLD/UI/PLDInteractionWidget.PLDInteractionWidget_C"));
    if (WidgetClass.Succeeded())
    {
        InteractionWidget->SetWidgetClass(WidgetClass.Class);
    }

    InteractionWidget->SetVisibility(false);
    InteractionWidget->SetDrawAtDesiredSize(true);

    // Ignore all trace/raycast channels
    InteractionWidget->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
    InteractionWidget->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
    
    InteractionWidget->SetWidgetSpace(EWidgetSpace::Screen);

    // Minimize collision impact
    InteractionWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    InteractionWidget->SetGenerateOverlapEvents(false);
}

void APLDInteractableActor::BeginPlay()
{
    Super::BeginPlay();
    if (InteractionWidget)
    {
        // The half-size of the bounding box of the mesh, for example,
        // if BoxExtent is (50, 50, 50), the full bounding box size is (100, 100, 100).
        FVector MeshCenter = MeshComponent->Bounds.BoxExtent;
        InteractionWidget->SetRelativeLocation(MeshCenter);
    }
    SetActorTickEnabled(false);  // тик нужен, только пока подсказка видна в VR (лицом к игроку)
}

void APLDInteractableActor::UpdatePromptPlacement()
{
    // Поверх предмета, как экранная подсказка на ПК. Настоящего «слоя поверх сцены» у виджета в
    // мире нет (он проверяет глубину), поэтому подсказка — на линии от глаз к центру предмета,
    // близко к глазам (VRPromptDistance), и уменьшена в той же пропорции: в шлеме она там же и того
    // же размера, что и на предмете, но ближе всего остального — ни предмет, ни соседи её не закроют
    const FBoxSphereBounds B = MeshComponent->Bounds;
    // Глаза — камера персонажа: в VR она следует за шлемом каждый кадр. У менеджера камеры место
    // в VR бывает без смещения шлема — подсказка «у глаз» оказывалась вне поля зрения
    FVector Eye = B.Origin - FVector::ForwardVector * 200.0f;
    const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
    if (const UCameraComponent* Cam = Pawn ? Pawn->FindComponentByClass<UCameraComponent>() : nullptr)
    {
        Eye = Cam->GetComponentLocation();
    }
    else if (APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
    {
        Eye = Camera->GetCameraLocation();
    }
    FVector Dir = B.Origin - Eye;
    const float ToObject = Dir.Size();
    if (!Dir.Normalize()) Dir = FVector::ForwardVector;
    const float Near = FMath::Min(VRPromptDistance, ToObject * 0.8f);
    if (!VRPromptWidget)
    {
        return;
    }
    VRPromptWidget->SetWorldLocation(Eye + Dir * Near);
    VRPromptWidget->SetWorldRotation((-Dir).Rotation());  // виджет смотрит по +X — на игрока
    // размер в поле зрения — постоянный (как экранная подсказка): масштаб задан для VRPromptDistance
    VRPromptWidget->SetWorldScale3D(FVector(VRPromptScale * Near / FMath::Max(VRPromptDistance, 1.0f)));
}

void APLDInteractableActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (bPromptInWorld && VRPromptWidget && VRPromptWidget->IsVisible())
    {
        UpdatePromptPlacement();
    }
    // if (InteractionWidget)
    // {
    //     APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
    //     if (PlayerController)
    //     {
    //         FRotator NewRotation = PlayerController->PlayerCameraManager->GetCameraRotation();
    //         NewRotation.Yaw += 180.0f; // Add half-turn to Yaw
    //         NewRotation.Pitch = 0.0f;  // Set Pitch to zero
    //         NewRotation.Roll = 0.0f;   // Set Roll to zero
    //         InteractionWidget->SetRelativeRotation(NewRotation);
    //     }
    // }
}

void APLDInteractableActor::Interact(AActor* Interactor)
{
    UE_LOG(LogTemp, Warning, TEXT("Interacted with %s"), *GetName());
    if (ANLBNavigatorCharacter* Character = Cast<ANLBNavigatorCharacter>(Interactor))
    {
        Character->SwitchToVisualNovel();
    }
}

UWidgetComponent* APLDInteractableActor::EnsureVRPrompt()
{
    if (VRPromptWidget || !InteractionWidget)
    {
        return VRPromptWidget;
    }
    VRPromptWidget = NewObject<UWidgetComponent>(this, TEXT("VRPromptWidget"));
    VRPromptWidget->SetUsingAbsoluteLocation(true);
    VRPromptWidget->SetUsingAbsoluteRotation(true);
    VRPromptWidget->SetUsingAbsoluteScale(true);
    VRPromptWidget->SetWidgetSpace(EWidgetSpace::World);  // до регистрации — как панель книги
    VRPromptWidget->SetWidgetClass(InteractionWidget->GetWidgetClass());
    VRPromptWidget->SetDrawAtDesiredSize(true);
    VRPromptWidget->SetPivot(FVector2D(0.5f, 0.5f));
    VRPromptWidget->SetTwoSided(true);
    VRPromptWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    VRPromptWidget->SetGenerateOverlapEvents(false);
    VRPromptWidget->SetCastShadow(false);
    VRPromptWidget->SetupAttachment(RootComponent);
    VRPromptWidget->RegisterComponent();
    VRPromptWidget->InitWidget();
    VRPromptWidget->SetVisibility(false);
    return VRPromptWidget;
}

void APLDInteractableActor::ShowInteractionWidget(bool bShow)
{
    if (!InteractionWidget)
    {
        return;
    }
    // подсказка и место — по текущему устройству (VR включается и выключается во время игры)
    const ANLBNavigatorCharacter* Character = Cast<ANLBNavigatorCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
    const bool bVR = Character && Character->IsVRMode();
    bPromptInWorld = bVR;
    UWidgetComponent* Shown = bVR ? EnsureVRPrompt() : InteractionWidget;
    if (bShow && Shown)
    {
        if (UPLDInteractionWidget* Widget = Cast<UPLDInteractionWidget>(Shown->GetUserWidgetObject()))
        {
            Widget->UpdateText(Character ? Character->GetInteractPrompt() : FString(TEXT("E: Interact")));
        }
        if (bVR)
        {
            UpdatePromptPlacement();
            // для разбора по журналу (adb logcat -s UE): где подсказка, её масштаб и есть ли виджет
            UE_LOG(LogTemp, Log, TEXT("NLB: подсказка VR у %s, масштаб %.3f, виджет %s"),
                   *Shown->GetComponentLocation().ToString(), Shown->GetComponentScale().X,
                   Shown->GetUserWidgetObject() ? TEXT("есть") : TEXT("НЕТ"));
        }
    }
    // видна только подсказка текущего устройства
    InteractionWidget->SetVisibility(bShow && !bVR);
    if (VRPromptWidget)
    {
        VRPromptWidget->SetVisibility(bShow && bVR);
    }
    SetActorTickEnabled(bShow && bVR);
}