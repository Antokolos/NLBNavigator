#pragma once

#include "InteractableInterface.h"
#include "GameFramework/Actor.h"
#include "Components/WidgetComponent.h"
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
    virtual void ShowInteractionWidget(bool bShow) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* MeshComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UWidgetComponent* InteractionWidget;
};
