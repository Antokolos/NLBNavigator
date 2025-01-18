#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "nlb/util/UEMultiLangStringAdapter.h"
#include "nlb/api/UEThemeAdapter.h"
#include "UEPageAdapter.generated.h"

class Page;

/**
 * @class UUEPageAdapter
 * @brief Unreal Engine adapter for the Page interface
 */
UCLASS(Blueprintable)
class NLBNAVIGATOR_API UUEPageAdapter : public UObject
{
    GENERATED_BODY()

private:
    Page* CorePage;

public:
    /** Sets the CorePage instance */
    void SetCorePage(Page* InCorePage);

    /** Gets the CorePage instance */
    Page* GetCorePage() const;

    // Image-related methods
    UFUNCTION(BlueprintCallable, Category = "Page|Image")
    FString GetImageFileName() const;

    UFUNCTION(BlueprintCallable, Category = "Page|Image")
    bool IsImageBackground() const;

    UFUNCTION(BlueprintCallable, Category = "Page|Image")
    bool IsImageAnimated() const;

    // Sound-related methods
    UFUNCTION(BlueprintCallable, Category = "Page|Sound")
    FString GetSoundFileName() const;

    UFUNCTION(BlueprintCallable, Category = "Page|Sound")
    bool IsSoundSFX() const;

    // Text and content methods
    UFUNCTION(BlueprintCallable, Category = "Page|Content")
    FString GetText() const;

    UFUNCTION(BlueprintCallable, Category = "Page|Content")
    UUEMultiLangStringAdapter* GetTexts() const;

    UFUNCTION(BlueprintCallable, Category = "Page|Content")
    UUEThemeAdapter* GetTheme() const;

    UFUNCTION(BlueprintCallable, Category = "Page|Content")
    UUEThemeAdapter* GetEffectiveTheme() const;

    UFUNCTION(BlueprintCallable, Category = "Page|Content")
    FString GetVarId() const;

    UFUNCTION(BlueprintCallable, Category = "Page|Content")
    FString GetTimerVarId() const;

    UFUNCTION(BlueprintCallable, Category = "Page|Content")
    FString GetCaption() const;

    UFUNCTION(BlueprintCallable, Category = "Page|Content")
    FString GetNotes() const;

    UFUNCTION(BlueprintCallable, Category = "Page|Content")
    UUEMultiLangStringAdapter* GetCaptions() const;

    UFUNCTION(BlueprintCallable, Category = "Page|Content")
    bool IsUseCaption() const;

    UFUNCTION(BlueprintCallable, Category = "Page|Content")
    bool IsUseMPL() const;

    UFUNCTION(BlueprintCallable, Category = "Page|Content")
    bool IsLeaf() const;

    UFUNCTION(BlueprintCallable, Category = "Page|Content")
    bool IsFinish() const;

    // Traverse-related methods
    UFUNCTION(BlueprintCallable, Category = "Page|Traverse")
    FString GetTraverseText() const;

    UFUNCTION(BlueprintCallable, Category = "Page|Traverse")
    UUEMultiLangStringAdapter* GetTraverseTexts() const;

    UFUNCTION(BlueprintCallable, Category = "Page|Traverse")
    bool IsAutoTraverse() const;

    UFUNCTION(BlueprintCallable, Category = "Page|Traverse")
    bool IsAutoReturn() const;

    // Return-related methods
    UFUNCTION(BlueprintCallable, Category = "Page|Return")
    FString GetReturnText() const;

    UFUNCTION(BlueprintCallable, Category = "Page|Return")
    UUEMultiLangStringAdapter* GetReturnTexts() const;

    UFUNCTION(BlueprintCallable, Category = "Page|Return")
    FString GetReturnPageId() const;

    UFUNCTION(BlueprintCallable, Category = "Page|Return")
    bool ShouldReturn() const;

    // Module-related methods
    UFUNCTION(BlueprintCallable, Category = "Page|Module")
    FString GetModuleConstrId() const;

    UFUNCTION(BlueprintCallable, Category = "Page|Module")
    FString GetModuleName() const;

    UFUNCTION(BlueprintCallable, Category = "Page|Module")
    bool IsModuleExternal() const;

    // UFUNCTION(BlueprintCallable, Category = "Page|Module")
    // UENonlinearBookAdapter* GetModule() const;

    // Autowire-related methods
    UFUNCTION(BlueprintCallable, Category = "Page|Autowire")
    bool IsAutowire() const;

    UFUNCTION(BlueprintCallable, Category = "Page|Autowire")
    bool IsGlobalAutowire() const;

    UFUNCTION(BlueprintCallable, Category = "Page|Autowire")
    bool IsNoSave() const;

    UFUNCTION(BlueprintCallable, Category = "Page|Autowire")
    bool IsAutosFirst() const;

    UFUNCTION(BlueprintCallable, Category = "Page|Autowire")
    FString GetAutowireInText() const;

    UFUNCTION(BlueprintCallable, Category = "Page|Autowire")
    UUEMultiLangStringAdapter* GetAutowireInTexts() const;

    UFUNCTION(BlueprintCallable, Category = "Page|Autowire")
    FString GetAutowireOutText() const;

    UFUNCTION(BlueprintCallable, Category = "Page|Autowire")
    UUEMultiLangStringAdapter* GetAutowireOutTexts() const;

    UFUNCTION(BlueprintCallable, Category = "Page|Autowire")
    bool IsAutoIn() const;

    UFUNCTION(BlueprintCallable, Category = "Page|Autowire")
    bool IsNeedsAction() const;

    UFUNCTION(BlueprintCallable, Category = "Page|Autowire")
    bool IsAutoOut() const;

    UFUNCTION(BlueprintCallable, Category = "Page|Autowire")
    FString GetAutowireInConstrId() const;

    UFUNCTION(BlueprintCallable, Category = "Page|Autowire")
    FString GetAutowireOutConstrId() const;

private:    
    UUEMultiLangStringAdapter* ConvertToAdapter(const MultiLangString& mls) const;
    UUEThemeAdapter* ConvertToAdapter(const Theme& theme) const;
};
