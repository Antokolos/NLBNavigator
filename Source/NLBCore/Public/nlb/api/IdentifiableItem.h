#pragma once

#include <string>
#include <memory>

class NonLinearBook;
class SearchContract;
class SearchResult;

class IdentifiableItem : public std::enable_shared_from_this<IdentifiableItem> {
public:
    virtual ~IdentifiableItem() = default;
    virtual std::string getId() const = 0;
    virtual std::string getFullId() const = 0;
    virtual bool isDeleted() const = 0;
    virtual std::shared_ptr<IdentifiableItem> getParent() const = 0;
    virtual bool hasDeletedParent() const = 0;
    virtual std::shared_ptr<NonLinearBook> getCurrentNLB() const = 0;
    virtual std::shared_ptr<SearchResult> searchText(const SearchContract& contract) const = 0;
    IdentifiableItem* getParentRaw() const { return getParent().get(); }
    virtual bool isModificationImpl() const { return false; }
};