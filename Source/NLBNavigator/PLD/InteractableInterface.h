#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractableInterface.generated.h"

UINTERFACE(MinimalAPI)
class UInteractableInterface : public UInterface
{
    GENERATED_BODY()
};

class NLBNAVIGATOR_API IInteractableInterface
{
    GENERATED_BODY()

public:
    // Метод для взаимодействия
    virtual void Interact(AActor* Interactor) = 0;

    // Метод для отображения подсказки
    virtual void ShowInteractionWidget(bool bShow) = 0;
};
