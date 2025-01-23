#include "nlb/domain/NullObj.h"

NullObj NullObj::s_instance;

NullObj& NullObj::create() {
    return s_instance;
}

NullObj::NullObj() : AbstractNodeItem() {}

std::string NullObj::getText() {
    return nlb::Constants::EMPTY_STRING;
}

std::string NullObj::getActText() {
    return nlb::Constants::EMPTY_STRING;
}

std::string NullObj::getNouseText() {
    return nlb::Constants::EMPTY_STRING;
}

MultiLangString NullObj::getTexts() {
    return Obj::DEFAULT_TEXT;
}

MultiLangString NullObj::getActTexts() {
    return Obj::DEFAULT_ACT_TEXT;
}

MultiLangString NullObj::getNouseTexts() {
    return Obj::DEFAULT_NOUSE_TEXT;
}

Theme NullObj::getEffectiveTheme() {
    return Theme::DEFAULT;
}

std::string NullObj::getVarId() {
    return Obj::DEFAULT_VARID;
}

std::string NullObj::getConstrId() {
    return Obj::DEFAULT_CONSTRID;
}

std::string NullObj::getCommonToId() {
    return Obj::DEFAULT_COMMON_TO_ID;
}

Obj* NullObj::getCommonToObj(NonLinearBook* nonLinearBook) {
    return nullptr;
}

std::string NullObj::getName() {
    return Obj::DEFAULT_NAME;
}

std::string NullObj::getImageFileName() {
    return Obj::DEFAULT_IMAGE_FILE_NAME;
}

std::string NullObj::getSoundFileName() {
    return Obj::DEFAULT_SOUND_FILE_NAME;
}

bool NullObj::isSoundSFX() {
    return Obj::DEFAULT_SOUND_SFX;
}

bool NullObj::isAnimatedImage() {
    return Obj::DEFAULT_ANIMATED_IMAGE;
}

bool NullObj::isSuppressDsc() {
    return Obj::DEFAULT_SUPPRESS_DSC;
}

std::string NullObj::getDisp() {
    return nlb::Constants::EMPTY_STRING;
}

MultiLangString NullObj::getDisps() {
    return Obj::DEFAULT_DISP;
}

bool NullObj::isGraphical() {
    return Obj::DEFAULT_GRAPHICAL;
}

bool NullObj::isShowOnCursor() {
    return Obj::DEFAULT_SHOW_ON_CURSOR;
}

bool NullObj::isPreserved() {
    return Obj::DEFAULT_PRESERVED;
}

bool NullObj::isLoadOnce() {
    return Obj::DEFAULT_LOAD_ONCE;
}

bool NullObj::isCollapsable() {
    return Obj::DEFAULT_COLLAPSABLE;
}

Obj::MovementDirection NullObj::getMovementDirection() {
    return Obj::DEFAULT_MOVEMENT_DIRECTION;
}

Obj::Effect NullObj::getEffect() {
    return Obj::DEFAULT_EFFECT;
}

int NullObj::getStartFrame() {
    return Obj::DEFAULT_START_FRAME;
}

int NullObj::getMaxFrame() {
    return Obj::DEFAULT_MAX_FRAME;
}

int NullObj::getPreloadFrames() {
    return Obj::DEFAULT_PRELOAD_FRAMES;
}

int NullObj::getPauseFrames() {
    return Obj::DEFAULT_PAUSE_FRAMES;
}

Obj::CoordsOrigin NullObj::getCoordsOrigin() {
    return Obj::CoordsOrigin::LeftTop;
}

bool NullObj::isClearUnderTooltip() {
    return Obj::DEFAULT_CLEAR_UNDER_TOOLTIP;
}

bool NullObj::isActOnKey() {
    return Obj::DEFAULT_ACT_ON_KEY;
}

bool NullObj::isCacheText() {
    return Obj::DEFAULT_CACHE_TEXT;
}

bool NullObj::isLooped() {
    return Obj::DEFAULT_LOOPED;
}

bool NullObj::isNoRedrawOnAct() {
    return Obj::DEFAULT_NO_REDRAW_ON_ACT;
}

std::string NullObj::getMorphOverId() {
    return Obj::DEFAULT_MORPH_OVER_ID;
}

Obj* NullObj::getMorphOverObj() {
    return nullptr;
}

std::string NullObj::getMorphOutId() {
    return Obj::DEFAULT_MORPH_OUT_ID;
}

Obj* NullObj::getMorphOutObj() {
    return nullptr;
}

std::string NullObj::getOffset() {
    return Obj::DEFAULT_OFFSET;
}

Coords& NullObj::getRelativeCoords(bool lookInMorphs) {
    return CoordsLw::getZeroCoords();
}

bool NullObj::isTakable() {
    return Obj::DEFAULT_TAKABLE;
}

bool NullObj::isCallback() {
    return Obj::DEFAULT_CALLBACK;
}

bool NullObj::isImageInScene() {
    return Obj::DEFAULT_IMAGE_IN_SCENE;
}

bool NullObj::isImageInInventory() {
    return Obj::DEFAULT_IMAGE_IN_INVENTORY;
}

std::string NullObj::getContainerId() {
    return Obj::DEFAULT_CONTAINER_ID;
}

Obj::ContainerType NullObj::getContainerType() {
    return Obj::ContainerType::None;
}

std::string NullObj::getCumulativeText(
    const std::vector<std::string>& objIdsToBeExcluded, 
    std::map<std::string, std::string>& visitedVars
) {
    return nlb::Constants::EMPTY_STRING;
}

void NullObj::setId(const std::string& id) {
    // Do nothing
}

std::string NullObj::getId() const {
    return Variable::NA;
}