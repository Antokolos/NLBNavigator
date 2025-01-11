#pragma once

#include "CoreMinimal.h"
#include "NLBCore/Public/Constants.h"
#include "UEConstantsAdapter.generated.h"

/**
 * UE adapter for NLB Constants
 */
UCLASS(BlueprintType)
class NLBNAVIGATOR_API UUEConstantsAdapter : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "NLB|Constants")
    static float GetZeroTolerance() { return nlb::Constants::FL_ZERO_TOLERANCE; }

    UFUNCTION(BlueprintPure, Category = "NLB|Constants")
    static FString GetUnicodeEncoding() { return UTF8_TO_TCHAR(nlb::Constants::UNICODE_ENCODING); }

    UFUNCTION(BlueprintPure, Category = "NLB|Constants")
    static FString GetMediaFileNameSeparator() { return UTF8_TO_TCHAR(nlb::Constants::MEDIA_FILE_NAME_SEP); }

    UFUNCTION(BlueprintPure, Category = "NLB|Constants")
    static FString GetRussianLanguageCode() { return UTF8_TO_TCHAR(nlb::Constants::RU); }

    UFUNCTION(BlueprintPure, Category = "NLB|Constants")
    static FString GetEnglishLanguageCode() { return UTF8_TO_TCHAR(nlb::Constants::EN); }

    UFUNCTION(BlueprintPure, Category = "NLB|Constants")
    static FString GetMainModuleName() { return UTF8_TO_TCHAR(nlb::Constants::MAIN_MODULE_NAME); }

    UFUNCTION(BlueprintPure, Category = "NLB|Constants")
    static FString GetEmptyString() { return UTF8_TO_TCHAR(nlb::Constants::EMPTY_STRING); }

    UFUNCTION(BlueprintPure, Category = "NLB|Constants")
    static FString GetVoid() { return UTF8_TO_TCHAR(nlb::Constants::VOID); }

    UFUNCTION(BlueprintPure, Category = "NLB|Constants")
    static FString GetEOLString() { return UTF8_TO_TCHAR(nlb::Constants::EOL_STRING); }

    UFUNCTION(BlueprintPure, Category = "NLB|Constants")
    static FString GetModificationsUndoIdPostfix() { return UTF8_TO_TCHAR(nlb::Constants::MODIFICATIONS_UNDO_ID_POSTFIX); }

    UFUNCTION(BlueprintPure, Category = "NLB|Constants")
    static FString GetNLBModuleAttribute() { return UTF8_TO_TCHAR(nlb::Constants::NLB_MODULE_ATTR); }

    UFUNCTION(BlueprintPure, Category = "NLB|Constants")
    static FString GetNLBPageAttribute() { return UTF8_TO_TCHAR(nlb::Constants::NLB_PAGE_ATTR); }

    UFUNCTION(BlueprintPure, Category = "NLB|Constants")
    static FString GetNLBObjectAttribute() { return UTF8_TO_TCHAR(nlb::Constants::NLB_OBJ_ATTR); }

    UFUNCTION(BlueprintPure, Category = "NLB|Constants")
    static FString GetNLBLinkAttribute() { return UTF8_TO_TCHAR(nlb::Constants::NLB_LINK_ATTR); }

    UFUNCTION(BlueprintPure, Category = "NLB|Constants")
    static FString GetYes() { return UTF8_TO_TCHAR(nlb::Constants::YES); }

    UFUNCTION(BlueprintPure, Category = "NLB|Constants")
    static FString GetNo() { return UTF8_TO_TCHAR(nlb::Constants::NO); }
};