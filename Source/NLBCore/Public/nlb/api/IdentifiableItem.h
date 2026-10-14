#pragma once

#include <string>
#include <memory>

class NonLinearBook;
class SearchContract;
class SearchResult;

class IdentifiableItem {
public:
    virtual ~IdentifiableItem() = default;
    virtual std::string getId() const = 0;
    virtual std::string getFullId() const = 0;
    virtual bool isDeleted() const = 0;
    virtual IdentifiableItem* getParent() const = 0;
    virtual bool hasDeletedParent() const = 0;
    virtual NonLinearBook* getCurrentNLB() const = 0;
    virtual SearchResult* searchText(const SearchContract& contract) const = 0;
    IdentifiableItem* getParentRaw() const { return getParent(); }
    virtual bool isModificationImpl() const { return false; }
};
