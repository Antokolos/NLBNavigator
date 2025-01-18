#pragma once

#include "CoreMinimal.h"
#include "UELinkAdapter.generated.h"

class Link;

UCLASS(BlueprintType)
class NLBNAVIGATOR_API UUELinkAdapter : public UObject {
    GENERATED_BODY()

private:
    Link* CoreLink; // Чистый C++ класс Link

public:
    // Геттеры
    UFUNCTION(BlueprintCallable, Category = "Link")
    FString GetId() const;

    // Сеттеры
    UFUNCTION(BlueprintCallable, Category = "Link")
    void SetId(const FString& NewId);

    void SetCoreLink(Link* link);
    const Link* GetCoreLink() const;
};
