#include "UEThemeAdapter.h"

EThemeUE UUEThemeAdapter::GetTheme() const {
    switch (CoreTheme) {
        case Theme::STANDARD:
            return EThemeUE::STANDARD;
        case Theme::VN:
            return EThemeUE::VN;
        default:
            return EThemeUE::DEFAULT;
    }
}

void UUEThemeAdapter::SetTheme(EThemeUE NewTheme) {
    switch (NewTheme) {
        case EThemeUE::STANDARD:
            CoreTheme = Theme::STANDARD;
            break;
        case EThemeUE::VN:
            CoreTheme = Theme::VN;
            break;
        default:
            CoreTheme = Theme::DEFAULT;
            break;
    }
}

void UUEThemeAdapter::SetThemeFromString(const FString& ThemeName) {
    CoreTheme = ThemeUtils::fromString(TCHAR_TO_UTF8(*ThemeName));
}

FString UUEThemeAdapter::GetThemeAsString() const {
    return FString(ThemeUtils::toString(CoreTheme).c_str());
}