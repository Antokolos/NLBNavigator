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
     * \return Reference to the singleton NullObj instance
     */
    static NullObj& create();

    // Obj interface method overrides
    std::string getText() override;
    std::string getActText() override;
    std::string getNouseText() override;
    MultiLangString getTexts() override;
    MultiLangString getActTexts() override;
    MultiLangString getNouseTexts() override;
    Theme getEffectiveTheme() override;
    std::string getVarId() override;
    std::string getConstrId() override;
    std::string getCommonToId() override;
    Obj* getCommonToObj(NonLinearBook* nonLinearBook) override;
    std::string getName() override;
    std::string getImageFileName() override;
    std::string getSoundFileName() override;
    bool isSoundSFX() override;
    bool isAnimatedImage() override;
    bool isSuppressDsc() override;

    std::string getDisp() override;
    MultiLangString getDisps() override;
    bool isGraphical() override;
    bool isShowOnCursor() override;
    bool isPreserved() override;
    bool isLoadOnce() override;
    bool isCollapsable() override;
    MovementDirection getMovementDirection() override;
    Effect getEffect() override;
    int getStartFrame() override;
    int getMaxFrame() override;
    int getPreloadFrames() override;
    int getPauseFrames() override;
    CoordsOrigin getCoordsOrigin() override;
    bool isClearUnderTooltip() override;
    bool isActOnKey() override;
    bool isCacheText() override;
    bool isLooped() override;
    bool isNoRedrawOnAct() override;

    std::string getMorphOverId() override;
    Obj* getMorphOverObj() override;
    std::string getMorphOutId() override;
    Obj* getMorphOutObj() override;
    std::string getOffset() override;
    Coords& getRelativeCoords(bool lookInMorphs) override;
    bool isTakable() override;
    bool isCallback() override;
    bool isImageInScene() override;
    bool isImageInInventory() override;
    std::string getContainerId() override;
    ContainerType getContainerType() override;
    std::string getCumulativeText(
        const std::vector<std::string>& objIdsToBeExcluded, 
        std::map<std::string, std::string>& visitedVars
    ) override;

    void setId(const std::string& id) override;
    std::string getId() const override;

    virtual std::string getFullId() const override { return AbstractNodeItem::getFullId(); }
    virtual bool isDeleted() const override { return AbstractNodeItem::isDeleted(); }
    virtual std::shared_ptr<IdentifiableItem> getParent() const override { return AbstractNodeItem::getParent(); }
    virtual bool hasDeletedParent() const override { return AbstractNodeItem::hasDeletedParent(); }
    virtual std::shared_ptr<NonLinearBook> getCurrentNLB() const override { return AbstractNodeItem::getCurrentNLB(); }
    virtual std::shared_ptr<SearchResult> searchText(const SearchContract& contract) const override { return AbstractNodeItem::searchText(contract); }

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