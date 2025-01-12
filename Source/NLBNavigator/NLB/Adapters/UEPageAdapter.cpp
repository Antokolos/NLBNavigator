#include "UEPageAdapter.h"
#include "UEMultiLangStringAdapter.h"

void UUEPageAdapter::SetCorePage(Page* InCorePage)
{
    CorePage = InCorePage;
}

Page* UUEPageAdapter::GetCorePage() const
{
    return CorePage;
}

UUEMultiLangStringAdapter* UUEPageAdapter::ConvertToAdapter(const MultiLangString& mls) const {
    UUEMultiLangStringAdapter* MultiLangStringAdapter = NewObject<UUEMultiLangStringAdapter>();
    if (MultiLangStringAdapter) {
        MultiLangStringAdapter->SetCoreMultiLangString(mls);
    }
    return MultiLangStringAdapter;
}

UUEThemeAdapter* UUEPageAdapter::ConvertToAdapter(const Theme& theme) const
{
    UUEThemeAdapter* ThemeAdapter = NewObject<UUEThemeAdapter>();
    if (ThemeAdapter) {
        ThemeAdapter->SetCoreTheme(theme);
    }
    return ThemeAdapter;
}

FString UUEPageAdapter::GetImageFileName() const
{
    return CorePage ? FString(CorePage->getImageFileName().c_str()) : FString();
}

bool UUEPageAdapter::IsImageBackground() const
{
    return CorePage ? CorePage->isImageBackground() : false;
}

bool UUEPageAdapter::IsImageAnimated() const
{
    return CorePage ? CorePage->isImageAnimated() : false;
}

FString UUEPageAdapter::GetSoundFileName() const
{
    return CorePage ? FString(CorePage->getSoundFileName().c_str()) : FString();
}

bool UUEPageAdapter::IsSoundSFX() const
{
    return CorePage ? CorePage->isSoundSFX() : false;
}

FString UUEPageAdapter::GetText() const
{
    return CorePage ? FString(CorePage->getText().c_str()) : FString();
}

UUEMultiLangStringAdapter* UUEPageAdapter::GetTexts() const
{
    return CorePage ? ConvertToAdapter(CorePage->getTexts()) : NewObject<UUEMultiLangStringAdapter>();
}

UUEThemeAdapter* UUEPageAdapter::GetTheme() const
{
    return CorePage ? ConvertToAdapter(CorePage->getTheme()) : NewObject<UUEThemeAdapter>();
}

UUEThemeAdapter* UUEPageAdapter::GetEffectiveTheme() const
{
    return CorePage ? ConvertToAdapter(CorePage->getEffectiveTheme()) : NewObject<UUEThemeAdapter>();
}

FString UUEPageAdapter::GetVarId() const
{
    return CorePage ? FString(CorePage->getVarId().c_str()) : FString();
}

FString UUEPageAdapter::GetTimerVarId() const
{
    return CorePage ? FString(CorePage->getTimerVarId().c_str()) : FString();
}

FString UUEPageAdapter::GetCaption() const
{
    return CorePage ? FString(CorePage->getCaption().c_str()) : FString();
}

FString UUEPageAdapter::GetNotes() const
{
    return CorePage ? FString(CorePage->getNotes().c_str()) : FString();
}

UUEMultiLangStringAdapter* UUEPageAdapter::GetCaptions() const
{
    return CorePage ? ConvertToAdapter(CorePage->getCaptions()) : NewObject<UUEMultiLangStringAdapter>();
}

bool UUEPageAdapter::IsUseCaption() const
{
    return CorePage ? CorePage->isUseCaption() : false;
}

bool UUEPageAdapter::IsUseMPL() const
{
    return CorePage ? CorePage->isUseMPL() : false;
}

bool UUEPageAdapter::IsLeaf() const
{
    return CorePage ? CorePage->isLeaf() : false;
}

bool UUEPageAdapter::IsFinish() const
{
    return CorePage ? CorePage->isFinish() : false;
}

FString UUEPageAdapter::GetTraverseText() const
{
    return CorePage ? FString(CorePage->getTraverseText().c_str()) : FString();
}

UUEMultiLangStringAdapter* UUEPageAdapter::GetTraverseTexts() const
{
    return CorePage ? ConvertToAdapter(CorePage->getTraverseTexts()) : NewObject<UUEMultiLangStringAdapter>();
}

bool UUEPageAdapter::IsAutoTraverse() const
{
    return CorePage ? CorePage->isAutoTraverse() : false;
}

bool UUEPageAdapter::IsAutoReturn() const
{
    return CorePage ? CorePage->isAutoReturn() : false;
}

FString UUEPageAdapter::GetReturnText() const
{
    return CorePage ? FString(CorePage->getReturnText().c_str()) : FString();
}

UUEMultiLangStringAdapter* UUEPageAdapter::GetReturnTexts() const
{
    return CorePage ? ConvertToAdapter(CorePage->getReturnTexts()) : NewObject<UUEMultiLangStringAdapter>();
}

FString UUEPageAdapter::GetReturnPageId() const
{
    return CorePage ? FString(CorePage->getReturnPageId().c_str()) : FString();
}

bool UUEPageAdapter::ShouldReturn() const
{
    return CorePage ? CorePage->shouldReturn() : false;
}

FString UUEPageAdapter::GetModuleConstrId() const
{
    return CorePage ? FString(CorePage->getModuleConstrId().c_str()) : FString();
}

FString UUEPageAdapter::GetModuleName() const
{
    return CorePage ? FString(CorePage->getModuleName().c_str()) : FString();
}

bool UUEPageAdapter::IsModuleExternal() const
{
    return CorePage ? CorePage->isModuleExternal() : false;
}

// UENonLinearBookAdapter* UUEPageAdapter::GetModule() const
// {
//     return CorePage ? CorePage->getModule() : nullptr;
// }

bool UUEPageAdapter::IsAutowire() const
{
    return CorePage ? CorePage->isAutowire() : false;
}

bool UUEPageAdapter::IsGlobalAutowire() const
{
    return CorePage ? CorePage->isGlobalAutowire() : false;
}

bool UUEPageAdapter::IsNoSave() const
{
    return CorePage ? CorePage->isNoSave() : false;
}

bool UUEPageAdapter::IsAutosFirst() const
{
    return CorePage ? CorePage->isAutosFirst() : false;
}

FString UUEPageAdapter::GetAutowireInText() const
{
    return CorePage ? FString(CorePage->getAutowireInText().c_str()) : FString();
}

UUEMultiLangStringAdapter* UUEPageAdapter::GetAutowireInTexts() const
{
    return CorePage ? ConvertToAdapter(CorePage->getAutowireInTexts()) : NewObject<UUEMultiLangStringAdapter>();
}

FString UUEPageAdapter::GetAutowireOutText() const
{
    return CorePage ? FString(CorePage->getAutowireOutText().c_str()) : FString();
}

UUEMultiLangStringAdapter* UUEPageAdapter::GetAutowireOutTexts() const
{
    return CorePage ? ConvertToAdapter(CorePage->getAutowireOutTexts()) : NewObject<UUEMultiLangStringAdapter>();
}

bool UUEPageAdapter::IsAutoIn() const
{
    return CorePage ? CorePage->isAutoIn() : false;
}

bool UUEPageAdapter::IsNeedsAction() const
{
    return CorePage ? CorePage->isNeedsAction() : false;
}

bool UUEPageAdapter::IsAutoOut() const
{
    return CorePage ? CorePage->isAutoOut() : false;
}

FString UUEPageAdapter::GetAutowireInConstrId() const
{
    return CorePage ? FString(CorePage->getAutowireInConstrId().c_str()) : FString();
}

FString UUEPageAdapter::GetAutowireOutConstrId() const
{
    return CorePage ? FString(CorePage->getAutowireOutConstrId().c_str()) : FString();
}
