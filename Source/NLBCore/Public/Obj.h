#pragma once

#include <string>
#include <vector>
#include <map>

#include "Theme.h"
#include "NodeItem.h"
#include "MultiLangString.h"

class NonLinearBook;

/*!
 * Interface representing an object in the system
 */
class NLBCORE_API Obj : public NodeItem {
public:
    enum class ContainerType { Page, Obj, None };
    enum class MovementDirection { Top, Left, None, Right, Bottom };
    enum class Effect { MoveIn, MoveOut, FadeIn, FadeOut, ZoomIn, ZoomOut, Overlap, None };
    enum class CoordsOrigin { 
        LeftTop, MiddleTop, RightTop, 
        LeftMiddle, MiddleMiddle, RightMiddle, 
        LeftBottom, MiddleBottom, RightBottom 
    };

    // Constants
    static const std::string DEFAULT_IMAGE_FILE_NAME;
    static const bool DEFAULT_ANIMATED_IMAGE = false;
    static const std::string DEFAULT_SOUND_FILE_NAME;
    static const bool DEFAULT_SOUND_SFX = true;
    static const bool DEFAULT_SUPPRESS_DSC = false;
    static const MultiLangString DEFAULT_TEXT;
    static const MultiLangString DEFAULT_ACT_TEXT;
    static const MultiLangString DEFAULT_NOUSE_TEXT;
    static const std::string DEFAULT_VARID;
    static const std::string DEFAULT_CONSTRID;
    static const std::string DEFAULT_COMMON_TO_ID;
    static const std::string DEFAULT_NAME;
    static const MultiLangString DEFAULT_DISP;
    static const bool DEFAULT_GRAPHICAL = false;
    static const bool DEFAULT_SHOW_ON_CURSOR = false;
    static const bool DEFAULT_PRESERVED = false;
    static const bool DEFAULT_LOAD_ONCE = false;
    static const bool DEFAULT_COLLAPSABLE = false;
    static const bool DEFAULT_CLEAR_UNDER_TOOLTIP = false;
    static const bool DEFAULT_ACT_ON_KEY = false;
    static const bool DEFAULT_CACHE_TEXT = false;
    static const bool DEFAULT_LOOPED = false;
    static const bool DEFAULT_NO_REDRAW_ON_ACT = false;
    static const std::string DEFAULT_MORPH_OVER_ID;
    static const std::string DEFAULT_MORPH_OUT_ID;
    static const std::string DEFAULT_OFFSET;
    static const bool DEFAULT_TAKABLE = false;
    static const bool DEFAULT_CALLBACK = false;
    static const bool DEFAULT_IMAGE_IN_SCENE = true;
    static const bool DEFAULT_IMAGE_IN_INVENTORY = true;
    static const std::string DEFAULT_CONTAINER_ID;
    static const MovementDirection DEFAULT_MOVEMENT_DIRECTION = MovementDirection::None;
    static const Effect DEFAULT_EFFECT = Effect::None;
    static const int DEFAULT_START_FRAME = 0;
    static const int DEFAULT_MAX_FRAME = 8;
    static const int DEFAULT_PRELOAD_FRAMES = 0;
    static const int DEFAULT_PAUSE_FRAMES = 0;

    virtual ~Obj() = default;

    virtual std::string getText() = 0;
    virtual std::string getActText() = 0;
    virtual std::string getNouseText() = 0;
    virtual MultiLangString getTexts() = 0;
    virtual MultiLangString getActTexts() = 0;
    virtual MultiLangString getNouseTexts() = 0;
    virtual Theme getEffectiveTheme() = 0;
    virtual std::string getVarId() = 0;
    virtual std::string getConstrId() = 0;
    virtual std::string getCommonToId() = 0;
    virtual Obj* getCommonToObj(NonLinearBook* nonLinearBook) = 0;
    virtual std::string getName() = 0;
    virtual std::string getImageFileName() = 0;
    virtual std::string getSoundFileName() = 0;
    virtual bool isSoundSFX() = 0;
    virtual bool isAnimatedImage() = 0;
    virtual bool isSuppressDsc() = 0;

    /*!
     * Returns display name of the object, which is used in the text, e.g. when inserted into the inventory
     * @return display name of the object
     */
    virtual std::string getDisp() = 0;
    virtual MultiLangString getDisps() = 0;
    virtual bool isGraphical() = 0;
    virtual bool isShowOnCursor() = 0;
    virtual bool isPreserved() = 0;
    virtual bool isLoadOnce() = 0;
    virtual bool isCollapsable() = 0;
    virtual MovementDirection getMovementDirection() = 0;
    virtual Effect getEffect() = 0;
    virtual int getStartFrame() = 0;
    virtual int getMaxFrame() = 0;
    virtual int getPreloadFrames() = 0;
    virtual int getPauseFrames() = 0;
    virtual CoordsOrigin getCoordsOrigin() = 0;
    virtual bool isClearUnderTooltip() = 0;
    virtual bool isActOnKey() = 0;
    virtual bool isCacheText() = 0;
    virtual bool isLooped() = 0;
    virtual bool isNoRedrawOnAct() = 0;
    virtual std::string getMorphOverId() = 0;
    virtual Obj* getMorphOverObj() = 0;
    virtual std::string getMorphOutId() = 0;
    virtual Obj* getMorphOutObj() = 0;
    virtual std::string getOffset() = 0;
    virtual Coords getRelativeCoords(bool lookInMorphs) = 0;
    virtual bool isTakable() = 0;
    virtual bool isCallback() = 0;
    virtual bool isImageInScene() = 0;
    virtual bool isImageInInventory() = 0;
    virtual std::string getContainerId() = 0;
    virtual ContainerType getContainerType() = 0;
    virtual std::string getCumulativeText(const std::vector<std::string>& objIdsToBeExcluded, std::map<std::string, std::string>& visitedVars) = 0;
};