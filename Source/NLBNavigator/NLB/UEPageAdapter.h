#pragma once

#include "CoreMinimal.h"
#include "Page.h"
#include "UELinkAdapter.h"
#include "UEMultiLangStringAdapter.h"
#include "UEPageAdapter.generated.h"

UCLASS(BlueprintType)
class NLBNAVIGATOR_API UUEPageAdapter : public UObject {
    GENERATED_BODY()

private:
    Page CorePage; // Чистый C++ класс

public:
    // Геттеры
    UFUNCTION(BlueprintCallable, Category = "Page")
    FString GetId() const;

    UFUNCTION(BlueprintCallable, Category = "Page")
    UUEMultiLangStringAdapter* GetContent() const;

    UFUNCTION(BlueprintCallable, Category = "Page")
    TArray<UUELinkAdapter*> GetLinks() const;

    // Сеттеры
    UFUNCTION(BlueprintCallable, Category = "Page")
    void SetId(const FString& NewId);

    UFUNCTION(BlueprintCallable, Category = "Page")
    void SetContent(const UUEMultiLangStringAdapter* NewContent);

    UFUNCTION(BlueprintCallable, Category = "Page")
    void SetLinks(const TArray<UUELinkAdapter*>& NewLinks);
};
