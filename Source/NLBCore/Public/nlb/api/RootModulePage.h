#pragma once

#include "nlb/api/Page.h"
#include "nlb/api/Constants.h"
#include "nlb/api/CoordsLw.h"
#include <memory>

class NonLinearBook;

/*!
 * @class RootModulePage
 * @brief Implementation of root module page in non-linear book
 * @implements Page
 */
class RootModulePage : public Page {
public:
    RootModulePage(NonLinearBook* nlb, const std::string& pageId);

    // Image related methods
    std::string getImageFileName() const override;
    bool isImageBackground() const override;
    bool isImageAnimated() const override;

    // Sound related methods
    std::string getSoundFileName() const override;
    bool isSoundSFX() const override;

    // Text and content methods
    std::string getText() const override;
    MultiLangString getTexts() const override;
    Theme getTheme() const override;
    Theme getEffectiveTheme() const override;
    std::string getVarId() const override;
    std::string getTimerVarId() const override;
    std::string getCaption() const override;
    std::string getNotes() const override;
    MultiLangString getCaptions() const override;
    bool isUseCaption() const override;
    bool isUseMPL() const override;
    bool isLeaf() const override;
    bool isFinish() const override;

    // Traverse related methods
    std::string getTraverseText() const override;
    MultiLangString getTraverseTexts() const override;
    bool isAutoTraverse() const override;
    bool isAutoReturn() const override;

    // Return related methods
    std::string getReturnText() const override;
    MultiLangString getReturnTexts() const override;
    std::string getReturnPageId() const override;
    bool shouldReturn() const override;

    // Module related methods
    std::string getModuleConstrId() const override;
    std::string getModuleName() const override;
    bool isModuleExternal() const override;
    NonLinearBook* getModule() const override;

    // Autowire related methods
    bool isAutowire() const override;
    bool isGlobalAutowire() const override;
    bool isNoSave() const override;
    bool isAutosFirst() const override;
    std::string getAutowireInText() const override;
    MultiLangString getAutowireInTexts() const override;
    std::string getAutowireOutText() const override;
    MultiLangString getAutowireOutTexts() const override;
    bool isAutoIn() const override;
    bool isNeedsAction() const override;
    bool isAutoOut() const override;
    std::string getAutowireInConstrId() const override;
    std::string getAutowireOutConstrId() const override;

    // NodeItem interface methods
    std::string getId() const override;
    std::string getFullId() const override;
    bool isDeleted() const override;
    IdentifiableItem* getParent() const override;
    bool hasDeletedParent() const override;
    NonLinearBook* getCurrentNLB() const override;
    SearchResult* searchText(const SearchContract& contract) const override;
    virtual std::vector<Modification*> getModifications() const override;
    virtual bool hasNoModifications() const override { return true; }
    virtual Modification* getModificationById(const std::string& modId) const override { return nullptr; }

    // Методы из NodeItem
    virtual std::string getDefaultTagId() const override { return DEFAULT_TAG_ID; }
    virtual std::string getStroke() const override { return nlb::Constants::EMPTY_STRING; }
    virtual std::string getFill() const override { return nlb::Constants::EMPTY_STRING; }
    virtual std::string getTextColor() const override { return nlb::Constants::EMPTY_STRING; }
    virtual std::vector<std::string> getContainedObjIds() const override { return std::vector<std::string>(); }
    virtual Coords* getCoords() const override { return new CoordsLw(); }
    virtual std::vector<Link*> getLinks() const override { return std::vector<Link*>(); }
    virtual Link* getLinkById(const std::string& id) const override { return nullptr; }
    virtual std::string getExternalHierarchy() const override { return nlb::Constants::EMPTY_STRING; }

    virtual std::string addObserver(NLBObserver* observer) override { return nlb::Constants::EMPTY_STRING; }
    virtual void removeObserver(const std::string& observerId) override {}
    virtual void notifyObservers() override {}

private:
    NonLinearBook* m_nlb;
    std::string m_pageId;
};
