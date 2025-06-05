#pragma once

#include <string>
#include <memory>
#include <vector>
#include <map>
#include "nlb/domain/AbstractNodeItem.h"
#include "nlb/api/Obj.h"
#include "nlb/util/MultiLangString.h"
#include "nlb/util/FileManipulator.h"
#include "nlb/api/CoordsLw.h"

class NonLinearBookImpl;
class Variable;

/*!
 * Implementation of Obj interface representing an object in the system
 */
class ObjImpl : public AbstractNodeItem, public Obj {
public:
    // File names constants
    static const std::string TEXT_SUBDIR_NAME;
    static const std::string ACT_TEXT_SUBDIR_NAME;
    static const std::string NOUSE_TEXT_SUBDIR_NAME;
    static const std::string VARID_FILE_NAME;
    static const std::string CONSTRID_FILE_NAME;
    static const std::string COMMONTOID_FILE_NAME;
    static const std::string NAME_FILE_NAME;
    static const std::string IMAGE_FILE_NAME;
    static const std::string SOUND_FILE_NAME;
    static const std::string SOUND_SFX_FILE_NAME;
    static const std::string SUPPRESS_DSC_FILE_NAME;
    static const std::string ANIMATED_FILE_NAME;
    static const std::string DISP_SUBDIR_NAME;
    static const std::string GRAPHICAL_FILE_NAME;
    static const std::string SHOW_ON_CURSOR_FILE_NAME;
    static const std::string PRESERVED_FILE_NAME;
    static const std::string LOADONCE_FILE_NAME;
    static const std::string COLLAPSABLE_FILE_NAME;
    static const std::string MVDIRECTION_FILE_NAME;
    static const std::string EFFECT_FILE_NAME;
    static const std::string START_FRAME_FILE_NAME;
    static const std::string MAX_FRAME_FILE_NAME;
    static const std::string PRELOAD_FRAMES_FILE_NAME;
    static const std::string PAUSE_FRAMES_FILE_NAME;
    static const std::string COORDSOR_FILE_NAME;
    static const std::string CLEARUTT_FILE_NAME;
    static const std::string ACTONKEY_FILE_NAME;
    static const std::string CACHETEXT_FILE_NAME;
    static const std::string NO_REDRAW_ON_ACT_FILE_NAME;
    static const std::string LOOPED_FILE_NAME;
    static const std::string MORPH_OVER_FILE_NAME;
    static const std::string MORPH_OUT_FILE_NAME;
    static const std::string OFFSET_FILE_NAME;
    static const std::string TAKABLE_FILE_NAME;
    static const std::string CALLBACK_FILE_NAME;
    static const std::string IMAGE_IN_SCENE_FILE_NAME;
    static const std::string IMAGE_IN_INVENTORY_FILE_NAME;
    static const std::string CONTAINERID_FILE_NAME;

    // Constructors
    ObjImpl(std::shared_ptr<Obj> source, std::shared_ptr<NonLinearBook> currentNLB);
    explicit ObjImpl(std::shared_ptr<NonLinearBook> currentNLB);
    ObjImpl(std::shared_ptr<NonLinearBook> currentNLB, float left, float top);

    // Text handling methods
    void setText(const std::string& text);
    std::string getText() const override;
    void setActText(const std::string& actText);
    std::string getActText() const override;
    void setNouseText(const std::string& nouseText);
    std::string getNouseText() const override;
    MultiLangString getTexts() const override;
    void setTexts(const MultiLangString& text);
    MultiLangString getActTexts() const;
    void setActTexts(const MultiLangString& actText);
    MultiLangString getNouseTexts() const override;
    void setNouseTexts(const MultiLangString& nouseText);

    // Theme handling
    Theme getEffectiveTheme() const override;

    // Variable and constraint getters/setters
    std::string getVarId() const override;
    std::string getConstrId() const override;
    std::string getCommonToId() const override;
    std::shared_ptr<Obj> getCommonToObj(std::shared_ptr<NonLinearBook> nonLinearBook) const override;
    void setCommonToId(const std::string& commonToId);
    void setVarId(const std::string& varId);
    void setConstrId(const std::string& constrId);

    // Name and file properties
    std::string getName() const override;
    std::string getImageFileName() const override;
    std::string getSoundFileName() const override;
    void setSoundFileName(const std::string& soundFileName);
    void setImageFileName(const std::string& imageFileName);
    bool isSoundSFX() const override;
    void setSoundSFX(bool soundSFX);
    void setAnimatedImage(bool animatedImage);
    bool isAnimatedImage() const override;
    void setSuppressDsc(bool suppressDsc);
    bool isSuppressDsc() const override;
    void setName(const std::string& name);

    // Display properties
    std::string getDisp() const override;
    MultiLangString getDisps() const override;
    bool isGraphical() const override;
    void setGraphical(bool graphical);
    bool isShowOnCursor() const override;
    void setShowOnCursor(bool showOnCursor);
    bool isPreserved() const override;
    void setPreserved(bool preserved);
    bool isLoadOnce() const override;
    void setLoadOnce(bool loadOnce);
    bool isCollapsable() const override;
    void setCollapsable(bool collapsable);

    // Position and movement
    std::string getOffset() const override;
    void setOffset(const std::string& offset);
    MovementDirection getMovementDirection() const override;
    void setMovementDirection(MovementDirection movementDirection);

    // Effect properties
    Effect getEffect() const override;
    void setEffect(Effect effect);
    int getStartFrame() const override;
    void setStartFrame(int startFrame);
    int getMaxFrame() const override;
    void setMaxFrame(int maxFrame);
    int getPreloadFrames() const override;
    void setPreloadFrames(int preloadFrames);
    int getPauseFrames() const override;
    void setPauseFrames(int pauseFrames);

    // Coordinate handling
    CoordsOrigin getCoordsOrigin() const override;
    void setCoordsOrigin(CoordsOrigin coordsOrigin);
    bool isClearUnderTooltip() const override;
    void setClearUnderTooltip(bool clearUnderTooltip);

    // Interaction properties
    bool isActOnKey() const override;
    void setActOnKey(bool actOnKey);
    bool isCacheText() const override;
    void setCacheText(bool cacheText);
    bool isLooped() const override;
    void setLooped(bool looped);
    bool isNoRedrawOnAct() const override;
    void setNoRedrawOnAct(bool noRedrawOnAct);

    // Morph properties
    std::string getMorphOverId() const override;
    std::shared_ptr<Obj> getMorphOverObj() const override;
    void setMorphOverId(const std::string& morphOverId);
    std::string getMorphOutId() const override;
    std::shared_ptr<Obj> getMorphOutObj() const override;
    void setMorphOutId(const std::string& morphOutId);

    // Coordinate calculations
    Coords& getRelativeCoords(bool lookInMorphs) const override;

    // Display text handling
    void setDisp(const std::string& disp);
    void setDisps(const MultiLangString& disp);
    
    // Object capabilities
    bool isTakable() const override;
    void setTakable(bool takable);
    bool isCallback() const override;
    void setCallback(bool callback);
    bool isImageInScene() const override;
    void setImageInScene(bool imageInScene);
    bool isImageInInventory() const override;
    void setImageInInventory(bool imageInInventory);

    // Container handling
    std::string getContainerId() const override;
    ContainerType getContainerType() const override;
    void setContainerId(const std::string& containerId);

    // File operations
    void writeObj(
        const std::shared_ptr<FileManipulator>& fileManipulator,
        const std::string& objsDir,
        std::shared_ptr<NonLinearBookImpl> nonLinearBook
    );
    void readObj(const std::string& objDir);

    // Text search
    std::shared_ptr<SearchResult> searchText(const SearchContract& contract) const override;

    // Text accumulation
    std::string getCumulativeText(
        const std::vector<std::string>& objIdsToBeExcluded,
        std::map<std::string, std::string>& visitedVars) const override;

    virtual std::string getFullId() const override { return AbstractNodeItem::getFullId(); }
    virtual bool isDeleted() const override { return AbstractNodeItem::isDeleted(); }
    virtual std::shared_ptr<IdentifiableItem> getParent() const override { return AbstractNodeItem::getParent(); }
    virtual bool hasDeletedParent() const override { return AbstractNodeItem::hasDeletedParent(); }
    virtual std::shared_ptr<NonLinearBook> getCurrentNLB() const override { return AbstractNodeItem::getCurrentNLB(); }
    
    virtual std::vector<std::shared_ptr<Modification>> getModifications() const override { return AbstractNodeItem::getModifications(); }
    virtual bool hasNoModifications() const override { return AbstractNodeItem::hasNoModifications(); }
    virtual std::shared_ptr<Modification> getModificationById(const std::string& modId) const override { return AbstractNodeItem::getModificationById(modId); }

    virtual std::string getDefaultTagId() const override { return AbstractNodeItem::getDefaultTagId(); }
    virtual std::string getStroke() const override { return AbstractNodeItem::getStroke(); }
    virtual std::string getFill() const override { return AbstractNodeItem::getFill(); }
    virtual std::string getTextColor() const override { return AbstractNodeItem::getTextColor(); }
    virtual std::vector<std::string> getContainedObjIds() const override { return AbstractNodeItem::getContainedObjIds(); }
    virtual std::shared_ptr<Coords> getCoords() const override { return AbstractNodeItem::getCoords(); }
    virtual std::vector<std::shared_ptr<Link>> getLinks() const override { return AbstractNodeItem::getLinks(); }

    virtual std::shared_ptr<Link> getLinkById(const std::string& linkId) const override { return AbstractNodeItem::getLinkById(linkId); }
    virtual std::string getExternalHierarchy() const override { return AbstractNodeItem::getExternalHierarchy(); }


    virtual std::string getId() const override { return AbstractIdentifiableItem::getId(); }
    virtual std::string addObserver(std::shared_ptr<NLBObserver> observer) override { return AbstractNodeItem::addObserver(observer); }
    virtual void removeObserver(const std::string& observerId) override { AbstractNodeItem::removeObserver(observerId); }
    virtual void notifyObservers() override { AbstractNodeItem::notifyObservers(); }

private:
    Coords& getRelativeCoordsByMorph(bool lookInMorphs) const;
    std::string getObjIdByMorphId(const std::string& morphId) const;
    
    // Helper methods for enum conversion
    std::string movementDirectionToString(MovementDirection direction) const;
    MovementDirection stringToMovementDirection(const std::string& str) const;
    std::string effectToString(Effect effect) const;
    Effect stringToEffect(const std::string& str) const;
    std::string coordsOriginToString(CoordsOrigin origin) const;
    CoordsOrigin stringToCoordsOrigin(const std::string& str) const;

    // Member variables
    std::string m_varId;
    std::string m_constrId;
    std::string m_commonToId;
    std::string m_name;
    MultiLangString m_disp;
    MultiLangString m_text;
    MultiLangString m_actText;
    MultiLangString m_nouseText;
    bool m_graphical;
    bool m_showOnCursor;
    bool m_preserved;
    bool m_loadOnce;
    bool m_collapsable;
    MovementDirection m_movementDirection;
    Effect m_effect;
    int m_startFrame;
    int m_maxFrame;
    int m_preloadFrames;
    int m_pauseFrames;
    CoordsOrigin m_coordsOrigin;
    bool m_clearUnderTooltip;
    bool m_actOnKey;
    bool m_cacheText;
    bool m_looped;
    bool m_noRedrawOnAct;
    std::string m_morphOverId;
    std::string m_morphOutId;
    std::string m_offset;
    bool m_takable;
    bool m_callback;
    std::string m_containerId;
    std::string m_imageFileName;
    bool m_animatedImage;
    std::string m_soundFileName;
    bool m_soundSFX;
    bool m_suppressDsc;
    bool m_imageInScene;
    bool m_imageInInventory;
};