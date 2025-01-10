#pragma once

#include "CoreMinimal.h"
#include "MultiLangString.h"
#include "UEMultiLangStringAdapter.generated.h"

UCLASS(BlueprintType)
class NLBNAVIGATOR_API UUEMultiLangStringAdapter : public UObject
{
    GENERATED_BODY()

private:
    MultiLangString CoreMultiLangString;

public:
    // Get translation for specified language
    UFUNCTION(BlueprintCallable, Category = "MultiLangString")
    FString GetTranslation(const FString& Language) const;

    // Set translation for specified language
    UFUNCTION(BlueprintCallable, Category = "MultiLangString")
    void SetTranslation(const FString& Language, const FString& Text);

    // Check if translation exists
    UFUNCTION(BlueprintCallable, Category = "MultiLangString")
    bool HasTranslation(const FString& Language) const;

    // Remove translation for specified language
    UFUNCTION(BlueprintCallable, Category = "MultiLangString")
    void RemoveTranslation(const FString& Language);

    // Clear all translations
    UFUNCTION(BlueprintCallable, Category = "MultiLangString")
    void Clear();

    void SetCoreMultiLangString(const MultiLangString& multiLangString);
    MultiLangString GetCoreMultiLangString() const;
};