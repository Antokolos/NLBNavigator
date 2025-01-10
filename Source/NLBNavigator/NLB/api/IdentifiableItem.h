#pragma once

#include <string>

class IdentifiableItem {
public:
    virtual ~IdentifiableItem() = default;

    virtual std::string GetId() const = 0;
    virtual std::string GetFullId() const = 0;
    virtual bool IsDeleted() const = 0;
    virtual IdentifiableItem* GetParent() const = 0;
    virtual bool HasDeletedParent() const = 0;
};
