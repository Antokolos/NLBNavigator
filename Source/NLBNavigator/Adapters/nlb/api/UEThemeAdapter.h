#pragma once

#include "CoreMinimal.h"
#include "NLBCore/Public/nlb/api/Theme.h"
#include "UEThemeAdapter.generated.h"

UENUM(BlueprintType)
enum class EThemeUE : uint8
{
    DEFAULT     UMETA(DisplayName = "Default"),
    STANDARD    UMETA(DisplayName = "Standard"),
    VN         UMETA(DisplayName = "VN")
};

UCLASS(BlueprintType)
class NLBNAVIGATOR_API UUEThemeAdapter : public UObject
{
    GENERATED_BODY()

private:
    Theme CoreTheme = Theme::DEFAULT;

public:
    // Get current theme
    UFUNCTION(BlueprintCallable, Category = "Theme")
    EThemeUE GetTheme() const;

    // Set theme
    UFUNCTION(BlueprintCallable, Category = "Theme")
    void SetTheme(EThemeUE NewTheme);

    // Set theme from string
    UFUNCTION(BlueprintCallable, Category = "Theme")
    void SetThemeFromString(const FString& ThemeName);

    // Get string representation of theme
    UFUNCTION(BlueprintCallable, Category = "Theme")
    FString GetThemeAsString() const;

    void SetCoreTheme(const Theme& theme);
    const Theme& GetCoreTheme() const;
};