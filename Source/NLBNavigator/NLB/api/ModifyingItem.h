#pragma once

#include <string>
#include <vector>
#include <memory>
#include "IdentifiableItem.h"
#include "Modification.h"

class ModifyingItem : public IdentifiableItem {
public:
    virtual ~ModifyingItem() = default;

    virtual std::vector<std::shared_ptr<Modification>> GetModifications() const = 0;
    virtual bool HasNoModifications() const = 0;
    virtual std::shared_ptr<Modification> GetModificationById(const std::string& ModId) const = 0;
};
