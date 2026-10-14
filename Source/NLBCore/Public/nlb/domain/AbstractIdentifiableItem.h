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
    explicit AbstractIdentifiableItem(NonLinearBook* currentNLB);
    AbstractIdentifiableItem(const IdentifiableItem* identifiableItem, 
                           NonLinearBook* currentNLB);
    AbstractIdentifiableItem(const IdentifiableItem* identifiableItem,
                           const ModifyingItem* parent,
                           NonLinearBook* currentNLB);
    virtual ~AbstractIdentifiableItem() override = default;

    void copy(const IdentifiableItem* identifiableItem);
    virtual void setId(const std::string& id);
    std::string getId() const override;
    IdentifiableItem* getParent() const override;
    void setParent(const IdentifiableItem* parent);
    virtual bool hasDeletedParent() const override;
    std::string getFullId() const override;
    void setDeleted(bool deleted);
    bool isDeleted() const override;
    virtual NonLinearBook* getCurrentNLB() const override;
    virtual SearchResult* searchText(const SearchContract& contract) const override;

protected:
    bool textMatches(const MultiLangString& mlsToTest, const SearchContract& contract) const;
    bool textMatches(const std::string& stringToTest, const SearchContract& contract) const;
    std::vector<std::string> createSortedDirList(const std::vector<std::string>& dirs,
                                                const std::vector<std::string>& orderList) const;

private:
    std::string m_id;
    bool m_isDeleted;
    NonLinearBook* m_currentNLB;
    IdentifiableItem* m_parent;
};
