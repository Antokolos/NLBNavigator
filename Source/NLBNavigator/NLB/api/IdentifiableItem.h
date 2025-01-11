#pragma once

#include <string>

class IdentifiableItem {
public:
    virtual ~IdentifiableItem() = default;

    virtual std::string getId() const = 0;
    virtual std::string getFullId() const = 0;
    virtual IdentifiableItem* getParent() const = 0;
};
