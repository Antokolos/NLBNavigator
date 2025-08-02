#pragma once

#include <string>
#include <vector>
#include <map>

#include "nlb/api/Theme.h"
#include "nlb/api/NodeItem.h"

class MultiLangString;
class NonLinearBook;

/*!
 * Interface representing an object in the system
 */
class Obj : public NodeItem {
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

    virtual std::string getText() const = 0;
    virtual std::string getActText() const = 0;
    virtual std::string getNouseText() const = 0;
    virtual MultiLangString getTexts() const = 0;
    virtual MultiLangString getActTexts() const = 0;
    virtual MultiLangString getNouseTexts() const = 0;
    virtual Theme getEffectiveTheme() const = 0;
    virtual std::string getVarId() const = 0;
    virtual std::string getConstrId() const = 0;
    virtual std::string getCommonToId() const = 0;
    virtual std::shared_ptr<Obj> getCommonToObj(std::shared_ptr<NonLinearBook> nonLinearBook) const = 0;
    virtual std::string getName() const = 0;
    virtual std::string getImageFileName() const = 0;
    virtual std::string getSoundFileName() const = 0;
    virtual bool isSoundSFX() const = 0;
    virtual bool isAnimatedImage() const = 0;
    virtual bool isSuppressDsc() const = 0;

    /*!
     * Returns display name of the object, which is used in the text, e.g. when inserted into the inventory
     * @return display name of the object
     */
    virtual std::string getDisp() const = 0;
    virtual MultiLangString getDisps() const = 0;
    virtual bool isGraphical() const = 0;
    virtual bool isShowOnCursor() const = 0;
    virtual bool isPreserved() const = 0;
    virtual bool isLoadOnce() const = 0;
    virtual bool isCollapsable() const = 0;
    virtual MovementDirection getMovementDirection() const = 0;
    virtual Effect getEffect() const = 0;
    virtual int getStartFrame() const = 0;
    virtual int getMaxFrame() const = 0;
    virtual int getPreloadFrames() const = 0;
    virtual int getPauseFrames() const = 0;
    virtual CoordsOrigin getCoordsOrigin() const = 0;
    virtual bool isClearUnderTooltip() const = 0;
    virtual bool isActOnKey() const = 0;
    virtual bool isCacheText() const = 0;
    virtual bool isLooped() const = 0;
    virtual bool isNoRedrawOnAct() const = 0;
    virtual std::string getMorphOverId() const = 0;
    virtual std::shared_ptr<Obj> getMorphOverObj() const = 0;
    virtual std::string getMorphOutId() const = 0;
    virtual std::shared_ptr<Obj> getMorphOutObj() const = 0;
    virtual std::string getOffset() const = 0;
    virtual Coords& getRelativeCoords(bool lookInMorphs) const = 0;
    virtual bool isTakable() const = 0;
    virtual bool isCallback() const = 0;
    virtual bool isImageInScene() const = 0;
    virtual bool isImageInInventory() const = 0;
    virtual std::string getContainerId() const = 0;
    virtual ContainerType getContainerType() const = 0;
    virtual std::string getCumulativeText(const std::vector<std::string>& objIdsToBeExcluded, std::map<std::string, std::string>& visitedVars) const = 0;
};