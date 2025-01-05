#pragma once

#include "InteractableInterface.h"
#include "GameFramework/Actor.h"
#include "PLDInteractableActor.generated.h"

UCLASS()
class NLBNAVIGATOR_API APLDInteractableActor : public AActor, public IInteractableInterface
{
    GENERATED_BODY()

public:
    APLDInteractableActor();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Interact(AActor* Interactor) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* MeshComponent;
};
