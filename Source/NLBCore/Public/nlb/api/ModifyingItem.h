#pragma once

#include <string>
#include <vector>
#include <memory>
#include "IdentifiableItem.h"
#include "Modification.h"

class ModifyingItem : public IdentifiableItem {
public:
    virtual ~ModifyingItem() override = default;

    virtual std::vector<Modification*> getModifications() const = 0;
    virtual bool hasNoModifications() const = 0;
    virtual Modification* getModificationById(const std::string& ModId) const = 0;
};
