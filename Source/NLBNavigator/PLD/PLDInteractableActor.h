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

    /// VR: масштаб подсказки (сантиметров на точку виджета) там, где она висит — в VRPromptDistance
    /// от глаз. Размер в поле зрения постоянный, как у экранной подсказки на ПК, при любом
    /// расстоянии до предмета
    UPROPERTY(EditAnywhere, Category = "Interaction")
    float VRPromptScale = 0.04f;
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

    /// Подсказка для VR — отдельный виджет в мире, созданный сразу в World space (как панель книги,
    /// которая в шлеме видна). Переключение экранного виджета в World на ходу в шлеме не рисовалось
    UPROPERTY(Transient)
    UWidgetComponent* VRPromptWidget = nullptr;
    UWidgetComponent* EnsureVRPrompt();
};
