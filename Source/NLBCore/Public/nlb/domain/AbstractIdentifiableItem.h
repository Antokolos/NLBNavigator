#pragma once

#include <string>
#include <list>
#include <vector>
#include <memory>
#include <regex>
#include "nlb/api/IdentifiableItem.h"

class NonLinearBook;
class ModifyingItem;
class MultiLangString;
class SearchContract;
class SearchResult;

class AbstractIdentifiableItem : public IdentifiableItem {
public:
    AbstractIdentifiableItem();
    explicit AbstractIdentifiableItem(std::shared_ptr<NonLinearBook> currentNLB);
    AbstractIdentifiableItem(const std::shared_ptr<IdentifiableItem>& identifiableItem, 
                           std::shared_ptr<NonLinearBook> currentNLB);
    AbstractIdentifiableItem(const std::shared_ptr<IdentifiableItem>& identifiableItem,
                           const std::shared_ptr<ModifyingItem>& parent,
                           std::shared_ptr<NonLinearBook> currentNLB);
    virtual ~AbstractIdentifiableItem() override = default;

    void copy(const std::shared_ptr<IdentifiableItem>& identifiableItem);
    virtual void setId(const std::string& id);
    std::string getId() const override;
    std::shared_ptr<IdentifiableItem> getParent() const override;
    void setParent(const std::shared_ptr<IdentifiableItem>& parent);
    virtual bool hasDeletedParent() const override;
    std::string getFullId() const override;
    void setDeleted(bool deleted);
    bool isDeleted() const override;
    virtual std::shared_ptr<NonLinearBook> getCurrentNLB() const override;
    virtual std::shared_ptr<SearchResult> searchText(const SearchContract& contract) const override;

protected:
    bool textMatches(const MultiLangString& mlsToTest, const SearchContract& contract) const;
    bool textMatches(const std::string& stringToTest, const SearchContract& contract) const;
    std::vector<std::string> createSortedDirList(const std::vector<std::string>& dirs,
                                                const std::vector<std::string>& orderList) const;

private:
    std::string m_id;
    bool m_isDeleted;
    std::shared_ptr<NonLinearBook> m_currentNLB;
    std::shared_ptr<IdentifiableItem> m_parent;
};