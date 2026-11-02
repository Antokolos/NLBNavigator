#include "PLDInteractableActor.h"
#include "PLDInteractionWidget.h"
#include "NLBNavigatorCharacter.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"

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
    // подсказка в мире поверх предмета, как на экране: на луче от центра предмета к глазам, чуть
    // перед его поверхностью (чтобы предмет её не закрывал), лицом к игроку
    APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0);
    const FBoxSphereBounds B = MeshComponent->Bounds;
    FVector Dir = Camera ? (Camera->GetCameraLocation() - B.Origin) : FVector::ForwardVector;
    if (!Dir.Normalize()) Dir = FVector::ForwardVector;
    // от центра до грани габарита по этому лучу
    float ToSurface = B.SphereRadius;
    for (int32 Axis = 0; Axis < 3; ++Axis)
    {
        if (FMath::Abs(Dir[Axis]) > KINDA_SMALL_NUMBER)
        {
            ToSurface = FMath::Min(ToSurface, B.BoxExtent[Axis] / FMath::Abs(Dir[Axis]));
        }
    }
    InteractionWidget->SetWorldLocation(B.Origin + Dir * (ToSurface + VRPromptOffset));
    InteractionWidget->SetWorldRotation(Dir.Rotation());  // виджет смотрит по +X — на игрока
}

void APLDInteractableActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (bPromptInWorld && InteractionWidget && InteractionWidget->IsVisible())
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

void APLDInteractableActor::ShowInteractionWidget(bool bShow)
{
    if (!InteractionWidget)
    {
        return;
    }
    if (bShow)
    {
        // подсказка и место — по текущему устройству (VR включается и выключается во время игры)
        const ANLBNavigatorCharacter* Character = Cast<ANLBNavigatorCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
        const bool bVR = Character && Character->IsVRMode();
        if (UPLDInteractionWidget* Widget = Cast<UPLDInteractionWidget>(InteractionWidget->GetUserWidgetObject()))
        {
            Widget->UpdateText(Character ? Character->GetInteractPrompt() : FString(TEXT("E: Interact")));
        }
        if (bVR != bPromptInWorld)
        {
            bPromptInWorld = bVR;
            InteractionWidget->SetWidgetSpace(bVR ? EWidgetSpace::World : EWidgetSpace::Screen);
            InteractionWidget->SetTwoSided(bVR);
            InteractionWidget->SetPivot(FVector2D(0.5f, 0.5f));
            if (bVR)
            {
                InteractionWidget->SetWorldScale3D(FVector(VRPromptScale));
            }
            else
            {
                // экран: как прежде — в углу габарита предмета, в масштабе экрана
                InteractionWidget->SetRelativeScale3D(FVector(1));
                InteractionWidget->SetRelativeRotation(FRotator::ZeroRotator);
                InteractionWidget->SetRelativeLocation(MeshComponent->Bounds.BoxExtent);
            }
        }
        if (bVR)
        {
            UpdatePromptPlacement();
        }
    }
    InteractionWidget->SetVisibility(bShow);
    SetActorTickEnabled(bShow && bPromptInWorld);
}