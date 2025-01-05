#include "PLDInteractableActor.h"
#include "PLDInteractionWidget.h"
#include "NLBNavigatorCharacter.h"
#include "Engine/Engine.h"

APLDInteractableActor::APLDInteractableActor()
{
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

    // Set to world space to prevent affecting parent bounds
    InteractionWidget->SetWidgetSpace(EWidgetSpace::World);
    
    // Minimize collision impact
    InteractionWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    InteractionWidget->SetGenerateOverlapEvents(false);
}

void APLDInteractableActor::BeginPlay()
{
    Super::BeginPlay();
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