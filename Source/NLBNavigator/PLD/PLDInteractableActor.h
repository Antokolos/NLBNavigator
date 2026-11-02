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
    virtual void Tick(float DeltaTime) override;

public:
    virtual void Interact(AActor* Interactor) override;
    virtual void ShowInteractionWidget(bool bShow) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* MeshComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UWidgetComponent* InteractionWidget;

    /// VR: масштаб подсказки в мире (сантиметров на точку виджета)
    UPROPERTY(EditAnywhere, Category = "Interaction")
    float VRPromptScale = 0.12f;
    /// VR: подсказка рисуется близко к глазам, на линии взгляда на предмет (не дальше стольких
    /// см), и уменьшена в той же пропорции — видна поверх предмета, как экранная на ПК, и её не
    /// закрывает ни сам предмет, ни соседние; подбирать по предмету ничего не нужно
    UPROPERTY(EditAnywhere, Category = "Interaction")
    float VRPromptDistance = 60.0f;

private:
    /// Подсказка в мире (VR): экранная подсказка проецируется по одному «глазу» и в шлеме видна
    /// в стороне от предмета
    bool bPromptInWorld = false;
    void UpdatePromptPlacement();
};
