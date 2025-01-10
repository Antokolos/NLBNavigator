#include "UEMultiLangStringAdapter.h"

FString UUEMultiLangStringAdapter::GetTranslation(const FString& Language) const {
    return FString(CoreMultiLangString.getTranslation(TCHAR_TO_UTF8(*Language)).c_str());
}

void UUEMultiLangStringAdapter::SetTranslation(const FString& Language, const FString& Text) {
    CoreMultiLangString.setTranslation(TCHAR_TO_UTF8(*Language), TCHAR_TO_UTF8(*Text));
}

bool UUEMultiLangStringAdapter::HasTranslation(const FString& Language) const {
    return CoreMultiLangString.hasTranslation(TCHAR_TO_UTF8(*Language));
}

void UUEMultiLangStringAdapter::RemoveTranslation(const FString& Language) {
    CoreMultiLangString.removeTranslation(TCHAR_TO_UTF8(*Language));
}

void UUEMultiLangStringAdapter::Clear() {
    CoreMultiLangString.clear();
}