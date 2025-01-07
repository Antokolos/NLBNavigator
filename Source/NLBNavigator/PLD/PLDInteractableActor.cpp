#include "PLDInteractableActor.h"
#include "PLDInteractionWidget.h"
#include "NLBNavigatorCharacter.h"
#include "Engine/Engine.h"

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
        if (UPLDInteractionWidget* Widget = Cast<UPLDInteractionWidget>(InteractionWidget->GetUserWidgetObject()))
        {
            Widget->UpdateText(TEXT("E: Interact"));
        }
    }
}

void APLDInteractableActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
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
    if (InteractionWidget)
    {
        InteractionWidget->SetVisibility(bShow);
    }
}