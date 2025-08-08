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

ObjImpl::ObjImpl(Obj* source, NonLinearBook* currentNLB)
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

ObjImpl::ObjImpl(NonLinearBook* currentNLB) 
    : AbstractNodeItem(currentNLB),
      m_varId(DEFAULT_VARID),
      m_constrId(DEFAULT_CONSTRID),
      m_commonToId(DEFAULT_COMMON_TO_ID),
      m_name(DEFAULT_NAME),
      m_imageFileName(DEFAULT_IMAGE_FILE_NAME),
      m_soundFileName(DEFAULT_SOUND_FILE_NAME),
      m_soundSFX(DEFAULT_SOUND_SFX),
      m_animatedImage(DEFAULT_ANIMATED_IMAGE),
      m_suppressDsc(DEFAULT_SUPPRESS_DSC),
      m_text(DEFAULT_TEXT),
      m_actText(DEFAULT_ACT_TEXT),
      m_nouseText(DEFAULT_NOUSE_TEXT),
      m_disp(DEFAULT_DISP),
      m_graphical(DEFAULT_GRAPHICAL),
      m_showOnCursor(DEFAULT_SHOW_ON_CURSOR),
      m_preserved(DEFAULT_PRESERVED),
      m_loadOnce(DEFAULT_LOAD_ONCE),
      m_collapsable(DEFAULT_COLLAPSABLE),
      m_movementDirection(DEFAULT_MOVEMENT_DIRECTION),
      m_effect(DEFAULT_EFFECT),
      m_startFrame(DEFAULT_START_FRAME),
      m_maxFrame(DEFAULT_MAX_FRAME),
      m_preloadFrames(DEFAULT_PRELOAD_FRAMES),
      m_pauseFrames(DEFAULT_PAUSE_FRAMES),
      m_coordsOrigin(CoordsOrigin::LeftTop),
      m_clearUnderTooltip(DEFAULT_CLEAR_UNDER_TOOLTIP),
      m_actOnKey(DEFAULT_ACT_ON_KEY),
      m_cacheText(DEFAULT_CACHE_TEXT),
      m_looped(DEFAULT_LOOPED),
      m_noRedrawOnAct(DEFAULT_NO_REDRAW_ON_ACT),
      m_morphOverId(DEFAULT_MORPH_OVER_ID),
      m_morphOutId(DEFAULT_MORPH_OUT_ID),
      m_offset(DEFAULT_OFFSET),
      m_takable(DEFAULT_TAKABLE),
      m_callback(DEFAULT_CALLBACK),
      m_imageInScene(DEFAULT_IMAGE_IN_SCENE),
      m_imageInInventory(DEFAULT_IMAGE_IN_INVENTORY),
      m_containerId(DEFAULT_CONTAINER_ID) {
}

ObjImpl::ObjImpl(NonLinearBook* currentNLB, float left, float top)
    : AbstractNodeItem(currentNLB, left, top),
      m_varId(DEFAULT_VARID),
      m_constrId(DEFAULT_CONSTRID),
      m_commonToId(DEFAULT_COMMON_TO_ID),
      m_name(DEFAULT_NAME),
      m_imageFileName(DEFAULT_IMAGE_FILE_NAME),
      m_soundFileName(DEFAULT_SOUND_FILE_NAME),
      m_soundSFX(DEFAULT_SOUND_SFX),
      m_animatedImage(DEFAULT_ANIMATED_IMAGE),
      m_suppressDsc(DEFAULT_SUPPRESS_DSC),
      m_text(DEFAULT_TEXT),
      m_actText(DEFAULT_ACT_TEXT),
      m_nouseText(DEFAULT_NOUSE_TEXT),
      m_disp(DEFAULT_DISP),
      m_graphical(DEFAULT_GRAPHICAL),
      m_showOnCursor(DEFAULT_SHOW_ON_CURSOR),
      m_preserved(DEFAULT_PRESERVED),
      m_loadOnce(DEFAULT_LOAD_ONCE),
      m_collapsable(DEFAULT_COLLAPSABLE),
      m_movementDirection(DEFAULT_MOVEMENT_DIRECTION),
      m_effect(DEFAULT_EFFECT),
      m_startFrame(DEFAULT_START_FRAME),
      m_maxFrame(DEFAULT_MAX_FRAME),
      m_preloadFrames(DEFAULT_PRELOAD_FRAMES),
      m_pauseFrames(DEFAULT_PAUSE_FRAMES),
      m_coordsOrigin(CoordsOrigin::LeftTop),
      m_clearUnderTooltip(DEFAULT_CLEAR_UNDER_TOOLTIP),
      m_actOnKey(DEFAULT_ACT_ON_KEY),
      m_cacheText(DEFAULT_CACHE_TEXT),
      m_looped(DEFAULT_LOOPED),
      m_noRedrawOnAct(DEFAULT_NO_REDRAW_ON_ACT),
      m_morphOverId(DEFAULT_MORPH_OVER_ID),
      m_morphOutId(DEFAULT_MORPH_OUT_ID),
      m_offset(DEFAULT_OFFSET),
      m_takable(DEFAULT_TAKABLE),
      m_callback(DEFAULT_CALLBACK),
      m_imageInScene(DEFAULT_IMAGE_IN_SCENE),
      m_imageInInventory(DEFAULT_IMAGE_IN_INVENTORY),
      m_containerId(DEFAULT_CONTAINER_ID) {
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
    NonLinearBook* currentNLB = getCurrentNLB();
    std::string containerId = getContainerId();
    if (containerId.empty()) {
        return Theme::DEFAULT;
    }
    Page* containerPage = currentNLB->getPageById(containerId);
    if (!containerPage) {
        Obj* containerObj = currentNLB->getObjById(containerId);
        if (containerObj) {
            return containerObj->getEffectiveTheme();
        }
    } else {
        return containerPage->getEffectiveTheme();
    }
    return Theme::DEFAULT;
}

SearchResult* ObjImpl::searchText(const SearchContract& contract) const {
    SearchResult* result = AbstractNodeItem::searchText(contract);
    if (result) {
        return result;
    } else if (
        textMatches(m_text, contract) ||
        textMatches(m_actText, contract) ||
        textMatches(m_nouseText, contract) ||
        textMatches(m_name, contract) ||
        textMatches(m_disp, contract) ||
        textMatches(m_imageFileName, contract) ||
        textMatches(m_soundFileName, contract) ||
        textMatches(m_varId, contract) ||
        textMatches(m_constrId, contract) ||
        textMatches(m_commonToId, contract)
    ) {
        result = new SearchResult();
        result->setId(AbstractNodeItem::getId());
        result->setInformation(getName());
        return result;
    }
    return nullptr;
}

Obj* ObjImpl::getCommonToObj(NonLinearBook* nonLinearBook) const {
    if (!StringHelper::isEmpty(getCommonToId())) {
        Variable* commonTo = nonLinearBook->getVariableById(getCommonToId());
        if (!commonTo->isDeleted()) {
            return nonLinearBook->getObjById(commonTo->getValue());
        }
    }
    return nullptr;
}

Coords& ObjImpl::getRelativeCoords(bool lookInMorphs) const {
    NonLinearBook* nlb = getCurrentNLB();
    NodeItem* node = nlb->getPageById(m_containerId);
    if (!node) {
        node = getCurrentNLB()->getObjById(m_containerId);
    }
    if (!node) {
        return getRelativeCoordsByMorph(lookInMorphs);
    }
    
    Coords* coordsParent = node->getCoords();
    Coords* coordsThis = AbstractNodeItem::getCoords();
    CoordsLw* result = new CoordsLw();
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
    Obj* morphOut = getMorphOutObj();
    if (morphOut) {
        return morphOut->getRelativeCoords(false);
    }
    Obj* morphOver = getMorphOverObj();
    if (morphOver) {
        return morphOver->getRelativeCoords(false);
    }
    return CoordsLw::getZeroCoords();
}

std::string ObjImpl::getObjIdByMorphId(const std::string& morphId) const {
    if (!StringHelper::isEmpty(morphId)) {
        Variable* morphVar = getCurrentNLB()->getVariableById(morphId);
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
            Obj* obj = getCurrentNLB()->getObjById(objId);
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

Obj* ObjImpl::getMorphOverObj() const {
    return getCurrentNLB()->getObjById(getObjIdByMorphId(m_morphOverId));
}

std::string ObjImpl::getMorphOutId() const {
    return m_morphOutId;
}

Obj* ObjImpl::getMorphOutObj() const {
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
        NonLinearBook* nlb = getCurrentNLB();
        if (nlb->getPageById(m_containerId)) {
            return Obj::ContainerType::Page;
        } else if (nlb->getObjById(m_containerId)) {
            return Obj::ContainerType::Obj;
        }
    }
    return Obj::ContainerType::None;
}

// Setter methods implementations
void ObjImpl::setCommonToId(const std::string& commonToId) {
    m_commonToId = commonToId;
}

void ObjImpl::setVarId(const std::string& varId) {
    m_varId = varId;
}

void ObjImpl::setConstrId(const std::string& constrId) {
    m_constrId = constrId;
}

void ObjImpl::setImageFileName(const std::string& imageFileName) {
    m_imageFileName = imageFileName;
}

void ObjImpl::setName(const std::string& name) {
    m_name = name;
}

void ObjImpl::setSoundFileName(const std::string& soundFileName) {
    m_soundFileName = soundFileName;
}

void ObjImpl::setSoundSFX(bool soundSFX) {
    m_soundSFX = soundSFX;
}

void ObjImpl::setAnimatedImage(bool animatedImage) {
    m_animatedImage = animatedImage;
}

void ObjImpl::setSuppressDsc(bool suppressDsc) {
    m_suppressDsc = suppressDsc;
}

void ObjImpl::setDisp(const std::string& disp) {
    m_disp.put(getCurrentNLB()->getLanguage(), disp);
}

void ObjImpl::setGraphical(bool graphical) {
    m_graphical = graphical;
}

void ObjImpl::setShowOnCursor(bool showOnCursor) {
    m_showOnCursor = showOnCursor;
}

void ObjImpl::setPreserved(bool preserved) {
    m_preserved = preserved;
}

void ObjImpl::setLoadOnce(bool loadOnce) {
    m_loadOnce = loadOnce;
}

void ObjImpl::setCollapsable(bool collapsable) {
    m_collapsable = collapsable;
}

void ObjImpl::setMovementDirection(MovementDirection movementDirection) {
    m_movementDirection = movementDirection;
}

void ObjImpl::setEffect(Effect effect) {
    m_effect = effect;
}

void ObjImpl::setStartFrame(int startFrame) {
    m_startFrame = startFrame;
}

void ObjImpl::setMaxFrame(int maxFrame) {
    m_maxFrame = maxFrame;
}

void ObjImpl::setPreloadFrames(int preloadFrames) {
    m_preloadFrames = preloadFrames;
}

void ObjImpl::setPauseFrames(int pauseFrames) {
    m_pauseFrames = pauseFrames;
}

void ObjImpl::setCoordsOrigin(CoordsOrigin coordsOrigin) {
    m_coordsOrigin = coordsOrigin;
}

void ObjImpl::setClearUnderTooltip(bool clearUnderTooltip) {
    m_clearUnderTooltip = clearUnderTooltip;
}

void ObjImpl::setActOnKey(bool actOnKey) {
    m_actOnKey = actOnKey;
}

void ObjImpl::setCacheText(bool cacheText) {
    m_cacheText = cacheText;
}

void ObjImpl::setLooped(bool looped) {
    m_looped = looped;
}

void ObjImpl::setNoRedrawOnAct(bool noRedrawOnAct) {
    m_noRedrawOnAct = noRedrawOnAct;
}

void ObjImpl::setMorphOverId(const std::string& morphOverId) {
    m_morphOverId = morphOverId;
}

void ObjImpl::setMorphOutId(const std::string& morphOutId) {
    m_morphOutId = morphOutId;
}

void ObjImpl::setOffset(const std::string& offset) {
    m_offset = offset;
}

void ObjImpl::setTakable(bool takable) {
    m_takable = takable;
}

void ObjImpl::setCallback(bool callback) {
    m_callback = callback;
}

void ObjImpl::setImageInScene(bool imageInScene) {
    m_imageInScene = imageInScene;
}

void ObjImpl::setImageInInventory(bool imageInInventory) {
    m_imageInInventory = imageInInventory;
}

void ObjImpl::setContainerId(const std::string& containerId) {
    m_containerId = containerId;
}

void ObjImpl::writeObj(
    const FileManipulator* fileManipulator,
    const std::string& objsDir,
    NonLinearBookImpl* nonLinearBook) {
    
    writeToFile(fileManipulator, objsDir, getId(), nonLinearBook);
    
    const std::string objDir = objsDir + "/" + getId();
    
    if (!isDeleted()) {
        // Write basic object properties
        fileManipulator->writeOptionalString(objDir, VARID_FILE_NAME, m_varId, DEFAULT_VARID);
        fileManipulator->writeOptionalString(objDir, CONSTRID_FILE_NAME, m_constrId, DEFAULT_CONSTRID);
        fileManipulator->writeOptionalString(objDir, COMMONTOID_FILE_NAME, m_commonToId, DEFAULT_COMMON_TO_ID);
        fileManipulator->writeOptionalString(objDir, NAME_FILE_NAME, m_name, DEFAULT_NAME);
        fileManipulator->writeOptionalString(objDir, IMAGE_FILE_NAME, m_imageFileName, DEFAULT_IMAGE_FILE_NAME);
        fileManipulator->writeOptionalString(objDir, SOUND_FILE_NAME, m_soundFileName, DEFAULT_SOUND_FILE_NAME);
        
        // Write boolean flags
        fileManipulator->writeOptionalString(objDir, SOUND_SFX_FILE_NAME,
            m_soundSFX ? "true" : "false", DEFAULT_SOUND_SFX ? "true" : "false");
        fileManipulator->writeOptionalString(objDir, ANIMATED_FILE_NAME,
            m_animatedImage ? "true" : "false", DEFAULT_ANIMATED_IMAGE ? "true" : "false");
        fileManipulator->writeOptionalString(objDir, SUPPRESS_DSC_FILE_NAME,
            m_suppressDsc ? "true" : "false", DEFAULT_SUPPRESS_DSC ? "true" : "false");
        fileManipulator->writeOptionalString(objDir, GRAPHICAL_FILE_NAME,
            m_graphical ? "true" : "false", DEFAULT_GRAPHICAL ? "true" : "false");
        fileManipulator->writeOptionalString(objDir, SHOW_ON_CURSOR_FILE_NAME,
            m_showOnCursor ? "true" : "false", DEFAULT_SHOW_ON_CURSOR ? "true" : "false");
        fileManipulator->writeOptionalString(objDir, PRESERVED_FILE_NAME,
            m_preserved ? "true" : "false", DEFAULT_PRESERVED ? "true" : "false");
        fileManipulator->writeOptionalString(objDir, LOADONCE_FILE_NAME,
            m_loadOnce ? "true" : "false", DEFAULT_LOAD_ONCE ? "true" : "false");
        fileManipulator->writeOptionalString(objDir, COLLAPSABLE_FILE_NAME,
            m_collapsable ? "true" : "false", DEFAULT_COLLAPSABLE ? "true" : "false");
        fileManipulator->writeOptionalString(objDir, CLEARUTT_FILE_NAME,
            m_clearUnderTooltip ? "true" : "false", DEFAULT_CLEAR_UNDER_TOOLTIP ? "true" : "false");
        fileManipulator->writeOptionalString(objDir, ACTONKEY_FILE_NAME,
            m_actOnKey ? "true" : "false", DEFAULT_ACT_ON_KEY ? "true" : "false");
        fileManipulator->writeOptionalString(objDir, CACHETEXT_FILE_NAME,
            m_cacheText ? "true" : "false", DEFAULT_CACHE_TEXT ? "true" : "false");
        fileManipulator->writeOptionalString(objDir, LOOPED_FILE_NAME,
            m_looped ? "true" : "false", DEFAULT_LOOPED ? "true" : "false");
        fileManipulator->writeOptionalString(objDir, NO_REDRAW_ON_ACT_FILE_NAME,
            m_noRedrawOnAct ? "true" : "false", DEFAULT_NO_REDRAW_ON_ACT ? "true" : "false");
        fileManipulator->writeOptionalString(objDir, TAKABLE_FILE_NAME,
            m_takable ? "true" : "false", DEFAULT_TAKABLE ? "true" : "false");
        fileManipulator->writeOptionalString(objDir, CALLBACK_FILE_NAME,
            m_callback ? "true" : "false", DEFAULT_CALLBACK ? "true" : "false");
        fileManipulator->writeOptionalString(objDir, IMAGE_IN_SCENE_FILE_NAME,
            m_imageInScene ? "true" : "false", DEFAULT_IMAGE_IN_SCENE ? "true" : "false");
        fileManipulator->writeOptionalString(objDir, IMAGE_IN_INVENTORY_FILE_NAME,
            m_imageInInventory ? "true" : "false", DEFAULT_IMAGE_IN_INVENTORY ? "true" : "false");
        
        // Write enum values
        fileManipulator->writeOptionalString(objDir, MVDIRECTION_FILE_NAME,
            movementDirectionToString(m_movementDirection),
            movementDirectionToString(DEFAULT_MOVEMENT_DIRECTION));
        fileManipulator->writeOptionalString(objDir, EFFECT_FILE_NAME,
            effectToString(m_effect),
            effectToString(DEFAULT_EFFECT));
        fileManipulator->writeOptionalString(objDir, COORDSOR_FILE_NAME,
            coordsOriginToString(m_coordsOrigin),
            coordsOriginToString(CoordsOrigin::LeftTop));
        
        // Write integer values
        fileManipulator->writeOptionalString(objDir, START_FRAME_FILE_NAME,
            std::to_string(m_startFrame), std::to_string(DEFAULT_START_FRAME));
        fileManipulator->writeOptionalString(objDir, MAX_FRAME_FILE_NAME,
            std::to_string(m_maxFrame), std::to_string(DEFAULT_MAX_FRAME));
        fileManipulator->writeOptionalString(objDir, PRELOAD_FRAMES_FILE_NAME,
            std::to_string(m_preloadFrames), std::to_string(DEFAULT_PRELOAD_FRAMES));
        fileManipulator->writeOptionalString(objDir, PAUSE_FRAMES_FILE_NAME,
            std::to_string(m_pauseFrames), std::to_string(DEFAULT_PAUSE_FRAMES));
        
        // Write additional properties
        fileManipulator->writeOptionalString(objDir, MORPH_OVER_FILE_NAME, m_morphOverId, DEFAULT_MORPH_OVER_ID);
        fileManipulator->writeOptionalString(objDir, MORPH_OUT_FILE_NAME, m_morphOutId, DEFAULT_MORPH_OUT_ID);
        fileManipulator->writeOptionalString(objDir, OFFSET_FILE_NAME, m_offset, DEFAULT_OFFSET);
        fileManipulator->writeOptionalString(objDir, CONTAINERID_FILE_NAME, m_containerId, DEFAULT_CONTAINER_ID);
        
        // Write multilanguage strings
        fileManipulator->writeOptionalMultiLangString(objDir + "/" + TEXT_SUBDIR_NAME, m_text, DEFAULT_TEXT);
        fileManipulator->writeOptionalMultiLangString(objDir + "/" + ACT_TEXT_SUBDIR_NAME, m_actText, DEFAULT_ACT_TEXT);
        fileManipulator->writeOptionalMultiLangString(objDir + "/" + NOUSE_TEXT_SUBDIR_NAME, m_nouseText, DEFAULT_NOUSE_TEXT);
        fileManipulator->writeOptionalMultiLangString(objDir + "/" + DISP_SUBDIR_NAME, m_disp, DEFAULT_DISP);
    }
}

void ObjImpl::readObj(const std::string& objDir) {
    setId(FileUtils::getFileName(objDir));
    readNodeItemProperties(objDir);
    
    // Read basic object properties
    m_varId = FileManipulator::getOptionalFileAsString(objDir, VARID_FILE_NAME, DEFAULT_VARID);
    m_constrId = FileManipulator::getOptionalFileAsString(objDir, CONSTRID_FILE_NAME, DEFAULT_CONSTRID);
    m_commonToId = FileManipulator::getOptionalFileAsString(objDir, COMMONTOID_FILE_NAME, DEFAULT_COMMON_TO_ID);
    m_name = FileManipulator::getOptionalFileAsString(objDir, NAME_FILE_NAME, DEFAULT_NAME);
    m_imageFileName = FileManipulator::getOptionalFileAsString(objDir, IMAGE_FILE_NAME, DEFAULT_IMAGE_FILE_NAME);
    m_soundFileName = FileManipulator::getOptionalFileAsString(objDir, SOUND_FILE_NAME, DEFAULT_SOUND_FILE_NAME);
    
    // Read boolean flags
    m_soundSFX = FileManipulator::getOptionalFileAsString(objDir, SOUND_SFX_FILE_NAME,
        DEFAULT_SOUND_SFX ? "true" : "false") == "true";
    m_animatedImage = FileManipulator::getOptionalFileAsString(objDir, ANIMATED_FILE_NAME,
        DEFAULT_ANIMATED_IMAGE ? "true" : "false") == "true";
    m_suppressDsc = FileManipulator::getOptionalFileAsString(objDir, SUPPRESS_DSC_FILE_NAME,
        DEFAULT_SUPPRESS_DSC ? "true" : "false") == "true";
    m_graphical = FileManipulator::getOptionalFileAsString(objDir, GRAPHICAL_FILE_NAME,
        DEFAULT_GRAPHICAL ? "true" : "false") == "true";
    m_showOnCursor = FileManipulator::getOptionalFileAsString(objDir, SHOW_ON_CURSOR_FILE_NAME,
        DEFAULT_SHOW_ON_CURSOR ? "true" : "false") == "true";
    m_preserved = FileManipulator::getOptionalFileAsString(objDir, PRESERVED_FILE_NAME,
        DEFAULT_PRESERVED ? "true" : "false") == "true";
    m_loadOnce = FileManipulator::getOptionalFileAsString(objDir, LOADONCE_FILE_NAME,
        DEFAULT_LOAD_ONCE ? "true" : "false") == "true";
    m_collapsable = FileManipulator::getOptionalFileAsString(objDir, COLLAPSABLE_FILE_NAME,
        DEFAULT_COLLAPSABLE ? "true" : "false") == "true";
    m_clearUnderTooltip = FileManipulator::getOptionalFileAsString(objDir, CLEARUTT_FILE_NAME,
        DEFAULT_CLEAR_UNDER_TOOLTIP ? "true" : "false") == "true";
    m_actOnKey = FileManipulator::getOptionalFileAsString(objDir, ACTONKEY_FILE_NAME,
        DEFAULT_ACT_ON_KEY ? "true" : "false") == "true";
    m_cacheText = FileManipulator::getOptionalFileAsString(objDir, CACHETEXT_FILE_NAME,
        DEFAULT_CACHE_TEXT ? "true" : "false") == "true";
    m_looped = FileManipulator::getOptionalFileAsString(objDir, LOOPED_FILE_NAME,
        DEFAULT_LOOPED ? "true" : "false") == "true";
    m_noRedrawOnAct = FileManipulator::getOptionalFileAsString(objDir, NO_REDRAW_ON_ACT_FILE_NAME,
        DEFAULT_NO_REDRAW_ON_ACT ? "true" : "false") == "true";
    m_takable = FileManipulator::getOptionalFileAsString(objDir, TAKABLE_FILE_NAME,
        DEFAULT_TAKABLE ? "true" : "false") == "true";
    m_callback = FileManipulator::getOptionalFileAsString(objDir, CALLBACK_FILE_NAME,
        DEFAULT_CALLBACK ? "true" : "false") == "true";
    m_imageInScene = FileManipulator::getOptionalFileAsString(objDir, IMAGE_IN_SCENE_FILE_NAME,
        DEFAULT_IMAGE_IN_SCENE ? "true" : "false") == "true";
    m_imageInInventory = FileManipulator::getOptionalFileAsString(objDir, IMAGE_IN_INVENTORY_FILE_NAME,
        DEFAULT_IMAGE_IN_INVENTORY ? "true" : "false") == "true";
    
    // Read enum values
    m_movementDirection = stringToMovementDirection(
        FileManipulator::getOptionalFileAsString(objDir, MVDIRECTION_FILE_NAME,
            movementDirectionToString(DEFAULT_MOVEMENT_DIRECTION)));
    m_effect = stringToEffect(
        FileManipulator::getOptionalFileAsString(objDir, EFFECT_FILE_NAME,
            effectToString(DEFAULT_EFFECT)));
    m_coordsOrigin = stringToCoordsOrigin(
        FileManipulator::getOptionalFileAsString(objDir, COORDSOR_FILE_NAME,
            coordsOriginToString(CoordsOrigin::LeftTop)));
    
    // Read integer values
    m_startFrame = std::stoi(FileManipulator::getOptionalFileAsString(objDir, START_FRAME_FILE_NAME,
        std::to_string(DEFAULT_START_FRAME)));
    m_maxFrame = std::stoi(FileManipulator::getOptionalFileAsString(objDir, MAX_FRAME_FILE_NAME,
        std::to_string(DEFAULT_MAX_FRAME)));
    m_preloadFrames = std::stoi(FileManipulator::getOptionalFileAsString(objDir, PRELOAD_FRAMES_FILE_NAME,
        std::to_string(DEFAULT_PRELOAD_FRAMES)));
    m_pauseFrames = std::stoi(FileManipulator::getOptionalFileAsString(objDir, PAUSE_FRAMES_FILE_NAME,
        std::to_string(DEFAULT_PAUSE_FRAMES)));
    
    // Read additional properties
    m_morphOverId = FileManipulator::getOptionalFileAsString(objDir, MORPH_OVER_FILE_NAME, DEFAULT_MORPH_OVER_ID);
    m_morphOutId = FileManipulator::getOptionalFileAsString(objDir, MORPH_OUT_FILE_NAME, DEFAULT_MORPH_OUT_ID);
    m_offset = FileManipulator::getOptionalFileAsString(objDir, OFFSET_FILE_NAME, DEFAULT_OFFSET);
    m_containerId = FileManipulator::getOptionalFileAsString(objDir, CONTAINERID_FILE_NAME, DEFAULT_CONTAINER_ID);
    
    // Read multilanguage strings
    m_text = FileManipulator::readOptionalMultiLangString(objDir + "/" + TEXT_SUBDIR_NAME, DEFAULT_TEXT);
    m_actText = FileManipulator::readOptionalMultiLangString(objDir + "/" + ACT_TEXT_SUBDIR_NAME, DEFAULT_ACT_TEXT);
    m_nouseText = FileManipulator::readOptionalMultiLangString(objDir + "/" + NOUSE_TEXT_SUBDIR_NAME, DEFAULT_NOUSE_TEXT);
    m_disp = FileManipulator::readOptionalMultiLangString(objDir + "/" + DISP_SUBDIR_NAME, DEFAULT_DISP);
}

std::string ObjImpl::movementDirectionToString(MovementDirection direction) const {
    switch (direction) {
        case MovementDirection::Top: return "Top";
        case MovementDirection::Left: return "Left";
        case MovementDirection::Right: return "Right";
        case MovementDirection::Bottom: return "Bottom";
        case MovementDirection::None:
        default: return "None";
    }
}

ObjImpl::MovementDirection ObjImpl::stringToMovementDirection(const std::string& str) const {
    if (str == "Top") return MovementDirection::Top;
    if (str == "Left") return MovementDirection::Left;
    if (str == "Right") return MovementDirection::Right;
    if (str == "Bottom") return MovementDirection::Bottom;
    return MovementDirection::None;
}

std::string ObjImpl::effectToString(Effect effect) const {
    switch (effect) {
        case Effect::MoveIn: return "MoveIn";
        case Effect::MoveOut: return "MoveOut";
        case Effect::FadeIn: return "FadeIn";
        case Effect::FadeOut: return "FadeOut";
        case Effect::ZoomIn: return "ZoomIn";
        case Effect::ZoomOut: return "ZoomOut";
        case Effect::Overlap: return "Overlap";
        case Effect::None:
        default: return "None";
    }
}

ObjImpl::Effect ObjImpl::stringToEffect(const std::string& str) const {
    if (str == "MoveIn") return Effect::MoveIn;
    if (str == "MoveOut") return Effect::MoveOut;
    if (str == "FadeIn") return Effect::FadeIn;
    if (str == "FadeOut") return Effect::FadeOut;
    if (str == "ZoomIn") return Effect::ZoomIn;
    if (str == "ZoomOut") return Effect::ZoomOut;
    if (str == "Overlap") return Effect::Overlap;
    return Effect::None;
}

std::string ObjImpl::coordsOriginToString(CoordsOrigin origin) const {
    switch (origin) {
        case CoordsOrigin::LeftTop: return "LeftTop";
        case CoordsOrigin::MiddleTop: return "MiddleTop";
        case CoordsOrigin::RightTop: return "RightTop";
        case CoordsOrigin::LeftMiddle: return "LeftMiddle";
        case CoordsOrigin::MiddleMiddle: return "MiddleMiddle";
        case CoordsOrigin::RightMiddle: return "RightMiddle";
        case CoordsOrigin::LeftBottom: return "LeftBottom";
        case CoordsOrigin::MiddleBottom: return "MiddleBottom";
        case CoordsOrigin::RightBottom: return "RightBottom";
        default: return "LeftTop";
    }
}

ObjImpl::CoordsOrigin ObjImpl::stringToCoordsOrigin(const std::string& str) const {
    if (str == "LeftTop") return ObjImpl::CoordsOrigin::LeftTop;
    if (str == "MiddleTop") return ObjImpl::CoordsOrigin::MiddleTop;
    if (str == "RightTop") return ObjImpl::CoordsOrigin::RightTop;
    if (str == "LeftMiddle") return ObjImpl::CoordsOrigin::LeftMiddle;
    if (str == "MiddleMiddle") return ObjImpl::CoordsOrigin::MiddleMiddle;
    if (str == "RightMiddle") return ObjImpl::CoordsOrigin::RightMiddle;
    if (str == "LeftBottom") return ObjImpl::CoordsOrigin::LeftBottom;
    if (str == "MiddleBottom") return ObjImpl::CoordsOrigin::MiddleBottom;
    if (str == "RightBottom") return ObjImpl::CoordsOrigin::RightBottom;
    return ObjImpl::CoordsOrigin::LeftTop;
}
