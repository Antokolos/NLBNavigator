#pragma once

#include <string>
#include <vector>
#include <memory>
#include "IdentifiableItem.h"
#include "Modification.h"

class ModifyingItem : public IdentifiableItem {
public:
    virtual ~ModifyingItem() = default;

    virtual std::vector<std::shared_ptr<Modification>> getModifications() const = 0;
    virtual bool hasNoModifications() const = 0;
    virtual std::shared_ptr<Modification> getModificationById(const std::string& ModId) const = 0;
};
