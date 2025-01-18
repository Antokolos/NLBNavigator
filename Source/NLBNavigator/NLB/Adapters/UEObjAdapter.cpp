#include "UEObjAdapter.h"
#include "NLBCore/Public/nlb/api/Obj.h"

void UUEObjAdapter::SetCoreObj(Obj* InCoreObj)
{
    CoreObj = InCoreObj;
}

Obj* UUEObjAdapter::GetCoreObj() const
{
    return CoreObj;
}

UUEMultiLangStringAdapter* UUEObjAdapter::ConvertToAdapter(const MultiLangString& mls) const {
    UUEMultiLangStringAdapter* MultiLangStringAdapter = NewObject<UUEMultiLangStringAdapter>();
    if (MultiLangStringAdapter) {
        MultiLangStringAdapter->SetCoreMultiLangString(mls);
    }
    return MultiLangStringAdapter;
}

UUEThemeAdapter* UUEObjAdapter::ConvertToAdapter(const Theme& theme) const
{
    UUEThemeAdapter* ThemeAdapter = NewObject<UUEThemeAdapter>();
    if (ThemeAdapter) {
        ThemeAdapter->SetCoreTheme(theme);
    }
    return ThemeAdapter;
}

// Text-related methods implementation
FString UUEObjAdapter::GetText() const
{
    return CoreObj ? FString(CoreObj->getText().c_str()) : FString();
}

UUEMultiLangStringAdapter* UUEObjAdapter::GetTexts() const
{
    return CoreObj ? ConvertToAdapter(CoreObj->getTexts()) : NewObject<UUEMultiLangStringAdapter>();
}

FString UUEObjAdapter::GetActText() const
{
    return CoreObj ? FString(CoreObj->getActText().c_str()) : FString();
}

FString UUEObjAdapter::GetNouseText() const
{
    return CoreObj ? FString(CoreObj->getNouseText().c_str()) : FString();
}

UUEMultiLangStringAdapter* UUEObjAdapter::GetActTexts() const
{
    return CoreObj ? ConvertToAdapter(CoreObj->getActTexts()) : NewObject<UUEMultiLangStringAdapter>();
}

UUEMultiLangStringAdapter* UUEObjAdapter::GetNouseTexts() const
{
    return CoreObj ? ConvertToAdapter(CoreObj->getNouseTexts()) : NewObject<UUEMultiLangStringAdapter>();
}

// Theme and display methods implementation
UUEThemeAdapter* UUEObjAdapter::GetEffectiveTheme() const
{
    return CoreObj ? ConvertToAdapter(CoreObj->getEffectiveTheme()) : NewObject<UUEThemeAdapter>();
}

FString UUEObjAdapter::GetDisp() const
{
    return CoreObj ? FString(CoreObj->getDisp().c_str()) : FString();
}

UUEMultiLangStringAdapter* UUEObjAdapter::GetDisps() const
{
    return CoreObj ? ConvertToAdapter(CoreObj->getDisps()) : NewObject<UUEMultiLangStringAdapter>();
}

// General attributes implementation
FString UUEObjAdapter::GetVarId() const
{
    return CoreObj ? FString(CoreObj->getVarId().c_str()) : FString();
}

FString UUEObjAdapter::GetConstrId() const
{
    return CoreObj ? FString(CoreObj->getConstrId().c_str()) : FString();
}

FString UUEObjAdapter::GetCommonToId() const
{
    return CoreObj ? FString(CoreObj->getCommonToId().c_str()) : FString();
}

FString UUEObjAdapter::GetName() const
{
    return CoreObj ? FString(CoreObj->getName().c_str()) : FString();
}

FString UUEObjAdapter::GetImageFileName() const
{
    return CoreObj ? FString(CoreObj->getImageFileName().c_str()) : FString();
}

FString UUEObjAdapter::GetSoundFileName() const
{
    return CoreObj ? FString(CoreObj->getSoundFileName().c_str()) : FString();
}

bool UUEObjAdapter::IsSoundSFX() const
{
    return CoreObj ? CoreObj->isSoundSFX() : false;
}

bool UUEObjAdapter::IsAnimatedImage() const
{
    return CoreObj ? CoreObj->isAnimatedImage() : false;
}

// Flags and behavior implementation
bool UUEObjAdapter::IsGraphical() const
{
    return CoreObj ? CoreObj->isGraphical() : false;
}

bool UUEObjAdapter::IsShowOnCursor() const
{
    return CoreObj ? CoreObj->isShowOnCursor() : false;
}

bool UUEObjAdapter::IsPreserved() const
{
    return CoreObj ? CoreObj->isPreserved() : false;
}

bool UUEObjAdapter::IsLoadOnce() const
{
    return CoreObj ? CoreObj->isLoadOnce() : false;
}

bool UUEObjAdapter::IsCollapsable() const
{
    return CoreObj ? CoreObj->isCollapsable() : false;
}

bool UUEObjAdapter::IsClearUnderTooltip() const
{
    return CoreObj ? CoreObj->isClearUnderTooltip() : false;
}

bool UUEObjAdapter::IsActOnKey() const
{
    return CoreObj ? CoreObj->isActOnKey() : false;
}

bool UUEObjAdapter::IsTakable() const
{
    return CoreObj ? CoreObj->isTakable() : false;
}

bool UUEObjAdapter::IsCallback() const
{
    return CoreObj ? CoreObj->isCallback() : false;
}

bool UUEObjAdapter::IsImageInScene() const
{
    return CoreObj ? CoreObj->isImageInScene() : false;
}

bool UUEObjAdapter::IsImageInInventory() const
{
    return CoreObj ? CoreObj->isImageInInventory() : false;
}

int UUEObjAdapter::GetStartFrame() const
{
    return CoreObj ? CoreObj->getStartFrame() : 0;
}

int UUEObjAdapter::GetMaxFrame() const
{
    return CoreObj ? CoreObj->getMaxFrame() : 0;
}

int UUEObjAdapter::GetPreloadFrames() const
{
    return CoreObj ? CoreObj->getPreloadFrames() : 0;
}

int UUEObjAdapter::GetPauseFrames() const
{
    return CoreObj ? CoreObj->getPauseFrames() : 0;
}

// Containers implementation
FString UUEObjAdapter::GetContainerId() const
{
    return CoreObj ? FString(CoreObj->getContainerId().c_str()) : FString();
}

FString UUEObjAdapter::GetCumulativeText(const TArray<FString>& ObjIdsToBeExcluded, TMap<FString, FString>& VisitedVars) const
{
    // TODO: stub
    return FString();
}

// Movement and effects implementation
EMovementDirection UUEObjAdapter::GetMovementDirection() const
{
    if (!CoreObj)
    {
        return EMovementDirection::None;
    }

    switch (CoreObj->getMovementDirection())
    {
        case Obj::MovementDirection::Top:    return EMovementDirection::Top;
        case Obj::MovementDirection::Left:   return EMovementDirection::Left;
        case Obj::MovementDirection::Right:  return EMovementDirection::Right;
        case Obj::MovementDirection::Bottom: return EMovementDirection::Bottom;
        case Obj::MovementDirection::None:
        default:                             return EMovementDirection::None;
    }
}

EEffect UUEObjAdapter::GetEffect() const
{
    if (!CoreObj)
    {
        return EEffect::None;
    }

    switch (CoreObj->getEffect())
    {
        case Obj::Effect::MoveIn:   return EEffect::MoveIn;
        case Obj::Effect::MoveOut:  return EEffect::MoveOut;
        case Obj::Effect::FadeIn:   return EEffect::FadeIn;
        case Obj::Effect::FadeOut:  return EEffect::FadeOut;
        case Obj::Effect::ZoomIn:   return EEffect::ZoomIn;
        case Obj::Effect::ZoomOut:  return EEffect::ZoomOut;
        case Obj::Effect::Overlap:  return EEffect::Overlap;
        case Obj::Effect::None:
        default:                    return EEffect::None;
    }
}

ECoordsOrigin UUEObjAdapter::GetCoordsOrigin() const
{
    if (!CoreObj)
    {
        return ECoordsOrigin::LeftTop;
    }

    switch (CoreObj->getCoordsOrigin())
    {
        case Obj::CoordsOrigin::LeftTop:       return ECoordsOrigin::LeftTop;
        case Obj::CoordsOrigin::MiddleTop:     return ECoordsOrigin::MiddleTop;
        case Obj::CoordsOrigin::RightTop:      return ECoordsOrigin::RightTop;
        case Obj::CoordsOrigin::LeftMiddle:    return ECoordsOrigin::LeftMiddle;
        case Obj::CoordsOrigin::MiddleMiddle:  return ECoordsOrigin::MiddleMiddle;
        case Obj::CoordsOrigin::RightMiddle:   return ECoordsOrigin::RightMiddle;
        case Obj::CoordsOrigin::LeftBottom:    return ECoordsOrigin::LeftBottom;
        case Obj::CoordsOrigin::MiddleBottom:  return ECoordsOrigin::MiddleBottom;
        case Obj::CoordsOrigin::RightBottom:   return ECoordsOrigin::RightBottom;
        default:                               return ECoordsOrigin::LeftTop;
    }
}

EContainerType UUEObjAdapter::GetContainerType() const
{
    if (!CoreObj)
    {
        return EContainerType::None;
    }

    switch (CoreObj->getContainerType())
    {
        case Obj::ContainerType::Page: return EContainerType::Page;
        case Obj::ContainerType::Obj:  return EContainerType::Obj;
        case Obj::ContainerType::None:
        default:                       return EContainerType::None;
    }
}