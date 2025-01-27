#include "nlb/domain/NullObj.h"

NullObj NullObj::s_instance;

NullObj& NullObj::create() {
    return s_instance;
}

NullObj::NullObj() : AbstractNodeItem() {}

std::string NullObj::getText() const {
    return nlb::Constants::EMPTY_STRING;
}

std::string NullObj::getActText() const {
    return nlb::Constants::EMPTY_STRING;
}

std::string NullObj::getNouseText() const {
    return nlb::Constants::EMPTY_STRING;
}

MultiLangString NullObj::getTexts() const {
    return Obj::DEFAULT_TEXT;
}

MultiLangString NullObj::getActTexts() const {
    return Obj::DEFAULT_ACT_TEXT;
}

MultiLangString NullObj::getNouseTexts() const {
    return Obj::DEFAULT_NOUSE_TEXT;
}

Theme NullObj::getEffectiveTheme() const {
    return Theme::DEFAULT;
}

std::string NullObj::getVarId() const {
    return Obj::DEFAULT_VARID;
}

std::string NullObj::getConstrId() const {
    return Obj::DEFAULT_CONSTRID;
}

std::string NullObj::getCommonToId() const {
    return Obj::DEFAULT_COMMON_TO_ID;
}

std::shared_ptr<Obj> NullObj::getCommonToObj(std::shared_ptr<NonLinearBook> nonLinearBook) const {
    return nullptr;
}

std::string NullObj::getName() const {
    return Obj::DEFAULT_NAME;
}

std::string NullObj::getImageFileName() const {
    return Obj::DEFAULT_IMAGE_FILE_NAME;
}

std::string NullObj::getSoundFileName() const {
    return Obj::DEFAULT_SOUND_FILE_NAME;
}

bool NullObj::isSoundSFX() const {
    return Obj::DEFAULT_SOUND_SFX;
}

bool NullObj::isAnimatedImage() const {
    return Obj::DEFAULT_ANIMATED_IMAGE;
}

bool NullObj::isSuppressDsc() const {
    return Obj::DEFAULT_SUPPRESS_DSC;
}

std::string NullObj::getDisp() const {
    return nlb::Constants::EMPTY_STRING;
}

MultiLangString NullObj::getDisps() const {
    return Obj::DEFAULT_DISP;
}

bool NullObj::isGraphical() const {
    return Obj::DEFAULT_GRAPHICAL;
}

bool NullObj::isShowOnCursor() const {
    return Obj::DEFAULT_SHOW_ON_CURSOR;
}

bool NullObj::isPreserved() const {
    return Obj::DEFAULT_PRESERVED;
}

bool NullObj::isLoadOnce() const {
    return Obj::DEFAULT_LOAD_ONCE;
}

bool NullObj::isCollapsable() const {
    return Obj::DEFAULT_COLLAPSABLE;
}

Obj::MovementDirection NullObj::getMovementDirection() const {
    return Obj::DEFAULT_MOVEMENT_DIRECTION;
}

Obj::Effect NullObj::getEffect() const {
    return Obj::DEFAULT_EFFECT;
}

int NullObj::getStartFrame() const {
    return Obj::DEFAULT_START_FRAME;
}

int NullObj::getMaxFrame() const {
    return Obj::DEFAULT_MAX_FRAME;
}

int NullObj::getPreloadFrames() const {
    return Obj::DEFAULT_PRELOAD_FRAMES;
}

int NullObj::getPauseFrames() const {
    return Obj::DEFAULT_PAUSE_FRAMES;
}

Obj::CoordsOrigin NullObj::getCoordsOrigin() const {
    return Obj::CoordsOrigin::LeftTop;
}

bool NullObj::isClearUnderTooltip() const {
    return Obj::DEFAULT_CLEAR_UNDER_TOOLTIP;
}

bool NullObj::isActOnKey() const {
    return Obj::DEFAULT_ACT_ON_KEY;
}

bool NullObj::isCacheText() const {
    return Obj::DEFAULT_CACHE_TEXT;
}

bool NullObj::isLooped() const {
    return Obj::DEFAULT_LOOPED;
}

bool NullObj::isNoRedrawOnAct() const {
    return Obj::DEFAULT_NO_REDRAW_ON_ACT;
}

std::string NullObj::getMorphOverId() const {
    return Obj::DEFAULT_MORPH_OVER_ID;
}

std::shared_ptr<Obj> NullObj::getMorphOverObj() const {
    return nullptr;
}

std::string NullObj::getMorphOutId() const {
    return Obj::DEFAULT_MORPH_OUT_ID;
}

std::shared_ptr<Obj> NullObj::getMorphOutObj() const {
    return nullptr;
}

std::string NullObj::getOffset() const {
    return Obj::DEFAULT_OFFSET;
}

Coords& NullObj::getRelativeCoords(bool lookInMorphs) const {
    return CoordsLw::getZeroCoords();
}

bool NullObj::isTakable() const {
    return Obj::DEFAULT_TAKABLE;
}

bool NullObj::isCallback() const {
    return Obj::DEFAULT_CALLBACK;
}

bool NullObj::isImageInScene() const {
    return Obj::DEFAULT_IMAGE_IN_SCENE;
}

bool NullObj::isImageInInventory() const {
    return Obj::DEFAULT_IMAGE_IN_INVENTORY;
}

std::string NullObj::getContainerId() const {
    return Obj::DEFAULT_CONTAINER_ID;
}

Obj::ContainerType NullObj::getContainerType() const {
    return Obj::ContainerType::None;
}

std::string NullObj::getCumulativeText(
    const std::vector<std::string>& objIdsToBeExcluded, 
    std::map<std::string, std::string>& visitedVars
) const {
    return nlb::Constants::EMPTY_STRING;
}

void NullObj::setId(const std::string& id) {
    // Do nothing
}

std::string NullObj::getId() const {
    return Variable::NA;
}