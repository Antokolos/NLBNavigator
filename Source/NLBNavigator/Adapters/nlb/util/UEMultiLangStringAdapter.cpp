#include "UEMultiLangStringAdapter.h"

bool UUEMultiLangStringAdapter::IsEmpty() const {
    return CoreMultiLangString.isEmpty();
}

FString UUEMultiLangStringAdapter::Get(const FString& LangKey) const {
    std::string Value = CoreMultiLangString.get(TCHAR_TO_UTF8(*LangKey));
    return FString(Value.c_str());
}

void UUEMultiLangStringAdapter::Put(const FString& LangKey, const FString& Value) {
    CoreMultiLangString.put(TCHAR_TO_UTF8(*LangKey), TCHAR_TO_UTF8(*Value));
}

TArray<FString> UUEMultiLangStringAdapter::KeySet() const {
    TArray<FString> Result;

    std::set<std::string> Keys = CoreMultiLangString.keySet();
    for (const auto& Key : Keys) {
        Result.Add(FString(Key.c_str()));
    }
    return Result;
}

TArray<FString> UUEMultiLangStringAdapter::Values() const {
    TArray<FString> Result;

    std::vector<std::string> Values = CoreMultiLangString.values();
    for (const auto& Value : Values) {
        Result.Add(FString(Value.c_str()));
    }
    return Result;
}

bool UUEMultiLangStringAdapter::IsSubsetOf(const UUEMultiLangStringAdapter* Other) const {
    if (!Other) return false;
    return CoreMultiLangString.isSubsetOf(Other->CoreMultiLangString);
}

void UUEMultiLangStringAdapter::SetCoreMultiLangString(const MultiLangString& multiLangString)
{
    CoreMultiLangString = multiLangString;
}

const MultiLangString& UUEMultiLangStringAdapter::GetCoreMultiLangString() const
{
    return CoreMultiLangString;
}