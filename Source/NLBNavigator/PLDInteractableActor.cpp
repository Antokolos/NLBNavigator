#include "PLDInteractableActor.h"
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
}

void APLDInteractableActor::BeginPlay()
{
    Super::BeginPlay();
}

void APLDInteractableActor::Interact(AActor* Interactor)
{
    // Реализуйте ваше действие
    UE_LOG(LogTemp, Warning, TEXT("Interacted with %s"), *GetName());
    if (ANLBNavigatorCharacter* Character = Cast<ANLBNavigatorCharacter>(Interactor))
    {
        Character->SwitchToVisualNovel();
    }
}
