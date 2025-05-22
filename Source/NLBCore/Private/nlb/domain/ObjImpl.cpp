#include "nlb/domain/ObjImpl.h"
#include "nlb/domain/NonLinearBookImpl.h"
#include "nlb/api/Variable.h"
#include "nlb/api/Obj.h"
#include "nlb/domain/SearchResult.h"
#include "nlb/util/StringHelper.h"
#include "nlb/api/Constants.h"

// File name constants
const std::string ObjImpl::TEXT_SUBDIR_NAME = "text";
const std::string ObjImpl::ACT_TEXT_SUBDIR_NAME = "acttext";
const std::string ObjImpl::NOUSE_TEXT_SUBDIR_NAME = "nousetxt";
const std::string ObjImpl::VARID_FILE_NAME = "varid";
const std::string ObjImpl::CONSTRID_FILE_NAME = "constrid";
const std::string ObjImpl::COMMONTOID_FILE_NAME = "commonto";
const std::string ObjImpl::NAME_FILE_NAME = "name";
const std::string ObjImpl::IMAGE_FILE_NAME = "image";
const std::string ObjImpl::SOUND_FILE_NAME = "sound";
const std::string ObjImpl::SOUND_SFX_FILE_NAME = "soundsfx";
const std::string ObjImpl::SUPPRESS_DSC_FILE_NAME = "suppdsc";
const std::string ObjImpl::ANIMATED_FILE_NAME = "animated";
const std::string ObjImpl::DISP_SUBDIR_NAME = "disp";
const std::string ObjImpl::GRAPHICAL_FILE_NAME = "graphical";
const std::string ObjImpl::SHOW_ON_CURSOR_FILE_NAME = "showoncur";
const std::string ObjImpl::PRESERVED_FILE_NAME = "preserved";
const std::string ObjImpl::LOADONCE_FILE_NAME = "loadonce";
const std::string ObjImpl::COLLAPSABLE_FILE_NAME = "collapsable";
const std::string ObjImpl::MVDIRECTION_FILE_NAME = "mvdirection";
const std::string ObjImpl::EFFECT_FILE_NAME = "effect";
const std::string ObjImpl::START_FRAME_FILE_NAME = "startframe";
const std::string ObjImpl::MAX_FRAME_FILE_NAME = "maxframe";
const std::string ObjImpl::PRELOAD_FRAMES_FILE_NAME = "preloadf";
const std::string ObjImpl::PAUSE_FRAMES_FILE_NAME = "pausef";
const std::string ObjImpl::COORDSOR_FILE_NAME = "coordsor";
const std::string ObjImpl::CLEARUTT_FILE_NAME = "clearutt";
const std::string ObjImpl::ACTONKEY_FILE_NAME = "actonkey";
const std::string ObjImpl::CACHETEXT_FILE_NAME = "cachetxt";
const std::string ObjImpl::NO_REDRAW_ON_ACT_FILE_NAME = "noactredraw";
const std::string ObjImpl::LOOPED_FILE_NAME = "looped";
const std::string ObjImpl::MORPH_OVER_FILE_NAME = "morphover";
const std::string ObjImpl::MORPH_OUT_FILE_NAME = "morphout";
const std::string ObjImpl::OFFSET_FILE_NAME = "offset";
const std::string ObjImpl::TAKABLE_FILE_NAME = "takable";
const std::string ObjImpl::CALLBACK_FILE_NAME = "callback";
const std::string ObjImpl::IMAGE_IN_SCENE_FILE_NAME = "imgscene";
const std::string ObjImpl::IMAGE_IN_INVENTORY_FILE_NAME = "imginv";
const std::string ObjImpl::CONTAINERID_FILE_NAME = "containerid";

ObjImpl::ObjImpl(std::shared_ptr<Obj> source, std::shared_ptr<NonLinearBook> currentNLB)
    : AbstractNodeItem(source, currentNLB) {
    m_varId = source->getVarId();
    m_constrId = source->getConstrId();
    m_commonToId = source->getCommonToId();
    m_name = source->getName();
    m_imageFileName = source->getImageFileName();
    m_soundFileName = source->getSoundFileName();
    m_soundSFX = source->isSoundSFX();
    m_animatedImage = source->isAnimatedImage();
    setDisps(source->getDisps());
    setTexts(source->getTexts());
    setActTexts(source->getActTexts());
    setNouseTexts(source->getNouseTexts());
    m_graphical = source->isGraphical();
    m_showOnCursor = source->isShowOnCursor();
    m_preserved = source->isPreserved();
    m_loadOnce = source->isLoadOnce();
    m_collapsable = source->isCollapsable();
    m_offset = source->getOffset();
    m_movementDirection = source->getMovementDirection();
    m_effect = source->getEffect();
    m_startFrame = source->getStartFrame();
    m_maxFrame = source->getMaxFrame();
    m_preloadFrames = source->getPreloadFrames();
    m_pauseFrames = source->getPauseFrames();
    m_coordsOrigin = source->getCoordsOrigin();
    m_clearUnderTooltip = source->isClearUnderTooltip();
    m_actOnKey = source->isActOnKey();
    m_cacheText = source->isCacheText();
    m_looped = source->isLooped();
    m_noRedrawOnAct = source->isNoRedrawOnAct();
    m_morphOverId = source->getMorphOverId();
    m_morphOutId = source->getMorphOutId();
    m_takable = source->isTakable();
    m_callback = source->isCallback();
    m_suppressDsc = source->isSuppressDsc();
    m_imageInScene = source->isImageInScene();
    m_imageInInventory = source->isImageInInventory();
    m_containerId = source->getContainerId();
}

ObjImpl::ObjImpl(std::shared_ptr<NonLinearBook> currentNLB) 
    : AbstractNodeItem(currentNLB) {
}

ObjImpl::ObjImpl(std::shared_ptr<NonLinearBook> currentNLB, float left, float top)
    : AbstractNodeItem(currentNLB, left, top) {
}

void ObjImpl::setText(const std::string& text) {
    m_text.put(getCurrentNLB()->getLanguage(), text);
}

std::string ObjImpl::getText() const {
    return m_text.get(getCurrentNLB()->getLanguage());
}

void ObjImpl::setActText(const std::string& actText) {
    m_actText.put(getCurrentNLB()->getLanguage(), actText);
}

std::string ObjImpl::getActText() const {
    return m_actText.get(getCurrentNLB()->getLanguage());
}

void ObjImpl::setNouseText(const std::string& nouseText) {
    m_nouseText.put(getCurrentNLB()->getLanguage(), nouseText);
}

std::string ObjImpl::getNouseText() const {
    return m_nouseText.get(getCurrentNLB()->getLanguage());
}

MultiLangString ObjImpl::getTexts() const {
    return MultiLangString::createCopy(m_text);
}

void ObjImpl::setTexts(const MultiLangString& text) {
    m_text = text;
}

MultiLangString ObjImpl::getActTexts() const {
    return MultiLangString::createCopy(m_actText);
}

void ObjImpl::setActTexts(const MultiLangString& actText) {
    m_actText = actText;
}

MultiLangString ObjImpl::getNouseTexts() const {
    return MultiLangString::createCopy(m_nouseText);
}

void ObjImpl::setNouseTexts(const MultiLangString& nouseText) {
    m_nouseText = nouseText;
}

Theme ObjImpl::getEffectiveTheme() const {
    std::shared_ptr<NonLinearBook> currentNLB = getCurrentNLB();
    std::string containerId = getContainerId();
    if (containerId.empty()) {
        return Theme::DEFAULT;
    }
    std::shared_ptr<Page> containerPage = currentNLB->getPageById(containerId);
    if (!containerPage) {
        std::shared_ptr<Obj> containerObj = currentNLB->getObjById(containerId);
        if (containerObj) {
            return containerObj->getEffectiveTheme();
        }
    } else {
        return containerPage->getEffectiveTheme();
    }
    return Theme::DEFAULT;
}

std::shared_ptr<SearchResult> ObjImpl::searchText(const SearchContract& contract) const {
    std::shared_ptr<SearchResult> result = AbstractNodeItem::searchText(contract);
    if (result) {
        return result;
    } else if (
        textMatches(m_text, contract) ||
        textMatches(m_actText, contract) ||
        textMatches(m_nouseText, contract) ||
        textMatches(m_name, contract) ||
        textMatches(m_disp, contract) ||
        textMatches(m_imageFileName, contract) ||
        textMatches(m_soundFileName, contract)
    ) {
        result = std::make_shared<SearchResult>();
        result->setId(AbstractNodeItem::getId());
        result->setInformation(getName());
        return result;
    }
    return nullptr;
}

std::shared_ptr<Obj> ObjImpl::getCommonToObj(std::shared_ptr<NonLinearBook> nonLinearBook) const {
    if (!StringHelper::isEmpty(getCommonToId())) {
        std::shared_ptr<Variable> commonTo = nonLinearBook->getVariableById(getCommonToId());
        if (!commonTo->isDeleted()) {
            return nonLinearBook->getObjById(commonTo->getValue());
        }
    }
    return nullptr;
}

Coords& ObjImpl::getRelativeCoords(bool lookInMorphs) const {
    std::shared_ptr<NonLinearBook> nlb = getCurrentNLB();
    std::shared_ptr<NodeItem> node = nlb->getPageById(m_containerId);
    if (!node) {
        node = getCurrentNLB()->getObjById(m_containerId);
    }
    if (!node) {
        return getRelativeCoordsByMorph(lookInMorphs);
    }
    
    std::shared_ptr<Coords> coordsParent = node->getCoords();
    std::shared_ptr<Coords> coordsThis = AbstractNodeItem::getCoords();
    std::shared_ptr<CoordsLw> result = std::make_shared<CoordsLw>();
    result->setLeft(coordsThis->getLeft() - coordsParent->getLeft());
    result->setTop(coordsThis->getTop() - coordsParent->getTop());
    result->setWidth(coordsParent->getWidth());
    result->setHeight(coordsParent->getHeight());
    return *result;
}

Coords& ObjImpl::getRelativeCoordsByMorph(bool lookInMorphs) const {
    if (!lookInMorphs) {
        return CoordsLw::getZeroCoords();
    }
    std::shared_ptr<Obj> morphOut = getMorphOutObj();
    if (morphOut) {
        return morphOut->getRelativeCoords(false);
    }
    std::shared_ptr<Obj> morphOver = getMorphOverObj();
    if (morphOver) {
        return morphOver->getRelativeCoords(false);
    }
    return CoordsLw::getZeroCoords();
}

std::string ObjImpl::getObjIdByMorphId(const std::string& morphId) const {
    if (!StringHelper::isEmpty(morphId)) {
        std::shared_ptr<Variable> morphVar = getCurrentNLB()->getVariableById(morphId);
        if (!morphVar->isDeleted()) {
            return morphVar->getValue();
        }
    }
    return nlb::Constants::EMPTY_STRING;
}

std::string ObjImpl::getCumulativeText(
    const std::vector<std::string>& objIdsToBeExcluded,
    std::map<std::string, std::string>& visitedVars) const {
    std::string result;
    if (std::find(objIdsToBeExcluded.begin(), objIdsToBeExcluded.end(), AbstractNodeItem::getId()) == objIdsToBeExcluded.end()) {
        ///result += StringHelper::replaceVariables(getText(), visitedVars);
        std::vector<std::string> containedObjIds = getContainedObjIds();
        for (const std::string& objId : containedObjIds) {
            std::shared_ptr<Obj> obj = getCurrentNLB()->getObjById(objId);
            if (obj) {
                result += obj->getCumulativeText(objIdsToBeExcluded, visitedVars);
            }
        }
    }
    return result;
}

std::string ObjImpl::getCommonToId() const
{
    return m_commonToId;
}

std::string ObjImpl::getVarId() const {
    return m_varId;
}

std::string ObjImpl::getConstrId() const {
    return m_constrId;
}

std::string ObjImpl::getName() const {
    return m_name;
}

std::string ObjImpl::getImageFileName() const {
    return m_imageFileName;
}

std::string ObjImpl::getSoundFileName() const {
    return m_soundFileName;
}

bool ObjImpl::isSoundSFX() const {
    return m_soundSFX;
}

bool ObjImpl::isAnimatedImage() const {
    return m_animatedImage;
}

bool ObjImpl::isSuppressDsc() const {
    return m_suppressDsc;
}

std::string ObjImpl::getDisp() const {
    return m_disp.get(getCurrentNLB()->getLanguage());
}

MultiLangString ObjImpl::getDisps() const {
    return MultiLangString::createCopy(m_disp);
}

void ObjImpl::setDisps(const MultiLangString& disp) {
    m_disp = disp;
}

bool ObjImpl::isGraphical() const {
    return m_graphical;
}

bool ObjImpl::isShowOnCursor() const {
    return m_showOnCursor;
}

bool ObjImpl::isPreserved() const {
    return m_preserved;
}

bool ObjImpl::isLoadOnce() const {
    return m_loadOnce;
}

bool ObjImpl::isCollapsable() const {
    return m_collapsable;
}

std::string ObjImpl::getOffset() const {
    return m_offset;
}

Obj::MovementDirection ObjImpl::getMovementDirection() const {
    return m_movementDirection;
}

Obj::Effect ObjImpl::getEffect() const {
    return m_effect;
}

int ObjImpl::getStartFrame() const {
    return m_startFrame;
}

int ObjImpl::getMaxFrame() const {
    return m_maxFrame;
}

int ObjImpl::getPreloadFrames() const {
    return m_preloadFrames;
}

int ObjImpl::getPauseFrames() const {
    return m_pauseFrames;
}

Obj::CoordsOrigin ObjImpl::getCoordsOrigin() const {
    return m_coordsOrigin;
}

bool ObjImpl::isClearUnderTooltip() const {
    return m_clearUnderTooltip;
}

bool ObjImpl::isActOnKey() const {
    return m_actOnKey;
}

bool ObjImpl::isCacheText() const {
    return m_cacheText;
}

bool ObjImpl::isLooped() const {
    return m_looped;
}

bool ObjImpl::isNoRedrawOnAct() const {
    return m_noRedrawOnAct;
}

std::string ObjImpl::getMorphOverId() const {
    return m_morphOverId;
}

std::shared_ptr<Obj> ObjImpl::getMorphOverObj() const {
    return getCurrentNLB()->getObjById(getObjIdByMorphId(m_morphOverId));
}

std::string ObjImpl::getMorphOutId() const {
    return m_morphOutId;
}

std::shared_ptr<Obj> ObjImpl::getMorphOutObj() const {
    return getCurrentNLB()->getObjById(getObjIdByMorphId(m_morphOutId));
}

bool ObjImpl::isTakable() const {
    return m_takable;
}

bool ObjImpl::isCallback() const {
    return m_callback;
}

bool ObjImpl::isImageInScene() const {
    return m_imageInScene;
}

bool ObjImpl::isImageInInventory() const {
    return m_imageInInventory;
}

std::string ObjImpl::getContainerId() const {
    return m_containerId;
}

Obj::ContainerType ObjImpl::getContainerType() const {
    // If this object has a container ID, determine its type
    if (!m_containerId.empty()) {
        std::shared_ptr<NonLinearBook> nlb = getCurrentNLB();
        if (nlb->getPageById(m_containerId)) {
            return Obj::ContainerType::Page;
        } else if (nlb->getObjById(m_containerId)) {
            return Obj::ContainerType::Obj;
        }
    }
    return Obj::ContainerType::None;
}

void ObjImpl::writeObj(const std::shared_ptr<FileManipulator>& fileManipulator, const std::string& objsDir,
    std::shared_ptr<NonLinearBookImpl> nonLinearBook)
{
    // stub
}

void ObjImpl::readObj(const std::string& objDir)
{
    // stub
}
