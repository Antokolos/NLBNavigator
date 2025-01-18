#pragma once

#include "nlb/api/Page.h"
#include <memory>

class NonLinearBook;

/*!
 * @class RootModulePage
 * @brief Implementation of root module page in non-linear book
 * @implements Page
 */
class NLBCORE_API RootModulePage : public Page {
public:
    RootModulePage(std::shared_ptr<NonLinearBook> nlb, const std::string& pageId);

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
    std::shared_ptr<IdentifiableItem> getParent() const override;
    bool hasDeletedParent() const override;
    std::shared_ptr<NonLinearBook> getCurrentNLB() const override;
    std::shared_ptr<SearchResult> searchText(const SearchContract& contract) const override;
    
private:
    std::shared_ptr<NonLinearBook> m_nlb;
    std::string m_pageId;
};