#pragma once

#include "CoreMinimal.h"
#include "NLBCore/Public/Link.h"
#include "UELinkAdapter.generated.h"

UCLASS(BlueprintType)
class NLBNAVIGATOR_API UUELinkAdapter : public UObject {
    GENERATED_BODY()

private:
    Link CoreLink; // Чистый C++ класс Link

public:
    // Геттеры
    UFUNCTION(BlueprintCallable, Category = "Link")
    FString GetId() const;

    UFUNCTION(BlueprintCallable, Category = "Link")
    FString GetDescription() const;

    // Сеттеры
    UFUNCTION(BlueprintCallable, Category = "Link")
    void SetId(const FString& NewId);

    UFUNCTION(BlueprintCallable, Category = "Link")
    void SetDescription(const FString& NewDescription);
    
    UFUNCTION(BlueprintCallable, Category = "Link")
    FString GetTargetPageId() const;

    void SetCoreLink(const Link& link);
    const Link& GetCoreLink() const;
};
