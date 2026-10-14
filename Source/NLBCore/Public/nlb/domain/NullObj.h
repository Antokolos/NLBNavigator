#pragma once

#include "nlb/api/Obj.h"
#include "nlb/api/Variable.h"
#include "nlb/api/Constants.h"
#include "nlb/util/MultiLangString.h"
#include "nlb/domain/AbstractNodeItem.h"
#include "nlb/api/CoordsLw.h"

/*!
 * \class NullObj
 * \brief Null Object implementation of the Obj interface, providing default/empty values
 *
 * This class implements the Obj interface with default or empty implementations,
 * following the Null Object design pattern. It is a singleton class that provides
 * safe default values for all Obj interface methods.
 */
class NullObj : public AbstractNodeItem, public Obj {
public:
    /*!
     * \brief Get the singleton instance of NullObj
     * \note This method follows the Singleton pattern from the Java implementation
     * where getInstance() method was used to get the singleton instance
     * \return Reference to the singleton NullObj instance
     */
    static NullObj& create();
    
    /*!
     * \brief Virtual destructor for proper cleanup
     */
    virtual ~NullObj() = default;

    // Obj interface method overrides
    std::string getText() const override;
    std::string getActText() const override;
    std::string getNouseText() const override;
    MultiLangString getTexts() const override;
    MultiLangString getActTexts() const override;
    MultiLangString getNouseTexts() const override;
    Theme getEffectiveTheme() const override;
    std::string getVarId() const override;
    std::string getConstrId() const override;
    std::string getCommonToId() const override;
    Obj* getCommonToObj(NonLinearBook* nonLinearBook) const override;
    std::string getName() const override;
    std::string getImageFileName() const override;
    std::string getSoundFileName() const override;
    bool isSoundSFX() const override;
    bool isAnimatedImage() const override;
    bool isSuppressDsc() const override;

    std::string getDisp() const override;
    MultiLangString getDisps() const override;
    bool isGraphical() const override;
    bool isShowOnCursor() const override;
    bool isPreserved() const override;
    bool isLoadOnce() const override;
    bool isCollapsable() const override;
    MovementDirection getMovementDirection() const override;
    Effect getEffect() const override;
    int getStartFrame() const override;
    int getMaxFrame() const override;
    int getPreloadFrames() const override;
    int getPauseFrames() const override;
    CoordsOrigin getCoordsOrigin() const override;
    bool isClearUnderTooltip() const override;
    bool isActOnKey() const override;
    bool isCacheText() const override;
    bool isLooped() const override;
    bool isNoRedrawOnAct() const override;

    std::string getMorphOverId() const override;
    Obj* getMorphOverObj() const override;
    std::string getMorphOutId() const override;
    Obj* getMorphOutObj() const override;
    std::string getOffset() const override;
    Coords& getRelativeCoords(bool lookInMorphs) const override;
    bool isTakable() const override;
    bool isCallback() const override;
    bool isImageInScene() const override;
    bool isImageInInventory() const override;
    std::string getContainerId() const override;
    ContainerType getContainerType() const override;
    virtual std::string getCumulativeText(
        const std::vector<std::string>& objIdsToBeExcluded, 
        std::map<std::string, std::string>& visitedVars
    ) const;

    /*!
     * \brief Override setId to do nothing (Null Object pattern)
     */
    void setId(const std::string& id) override {}
    std::string getId() const override;

    virtual std::string getFullId() const override { return AbstractNodeItem::getFullId(); }
    virtual bool isDeleted() const override { return AbstractNodeItem::isDeleted(); }
    virtual IdentifiableItem* getParent() const override { return AbstractNodeItem::getParent(); }
    virtual bool hasDeletedParent() const override { return AbstractNodeItem::hasDeletedParent(); }
    virtual NonLinearBook* getCurrentNLB() const override { return AbstractNodeItem::getCurrentNLB(); }
    virtual SearchResult* searchText(const SearchContract& contract) const override { return AbstractNodeItem::searchText(contract); }

    virtual std::vector<Modification*> getModifications() const override { return AbstractNodeItem::getModifications(); }
    virtual bool hasNoModifications() const override { return AbstractNodeItem::hasNoModifications(); }
    virtual Modification* getModificationById(const std::string& modId) const override { return AbstractNodeItem::getModificationById(modId); }

    virtual std::string getDefaultTagId() const override { return AbstractNodeItem::getDefaultTagId(); }
    virtual std::string getStroke() const override { return AbstractNodeItem::getStroke(); }
    virtual std::string getFill() const override { return AbstractNodeItem::getFill(); }
    virtual std::string getTextColor() const override { return AbstractNodeItem::getTextColor(); }
    virtual std::vector<std::string> getContainedObjIds() const override { return AbstractNodeItem::getContainedObjIds(); }
    virtual Coords* getCoords() const override { return AbstractNodeItem::getCoords(); }
    virtual std::vector<Link*> getLinks() const override { return AbstractNodeItem::getLinks(); }

    virtual Link* getLinkById(const std::string& linkId) const override { return AbstractNodeItem::getLinkById(linkId); }
    virtual std::string getExternalHierarchy() const override { return AbstractNodeItem::getExternalHierarchy(); }

    virtual std::string addObserver(NLBObserver* observer) override { return AbstractNodeItem::addObserver(observer); }
    virtual void removeObserver(const std::string& observerId) override { AbstractNodeItem::removeObserver(observerId); }
    virtual void notifyObservers() override { AbstractNodeItem::notifyObservers(); }

private:
    /*!
     * \brief Private constructor to ensure singleton pattern
     */
    NullObj();

    // Prevent copying and assignment
    NullObj(const NullObj&) = delete;
    NullObj& operator=(const NullObj&) = delete;

    // Singleton instance
    static NullObj s_instance;
};