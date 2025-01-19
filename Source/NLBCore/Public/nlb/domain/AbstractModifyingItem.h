#pragma once

#include <vector>
#include <memory>
#include <string>
#include "nlb/domain/AbstractIdentifiableItem.h"
#include "nlb/api/ModifyingItem.h"
#include "nlb/api/Modification.h"
#include "nlb/domain/ModificationImpl.h"

class FileManipulator;
class NonLinearBook;

class AbstractModifyingItem : public AbstractIdentifiableItem, public ModifyingItem, 
                            public std::enable_shared_from_this<AbstractModifyingItem> {
public:
    AbstractModifyingItem();
    explicit AbstractModifyingItem(std::shared_ptr<NonLinearBook> currentNLB);
    AbstractModifyingItem(const std::shared_ptr<ModifyingItem>& modifyingItem, 
                         std::shared_ptr<NonLinearBook> currentNLB);
    virtual ~AbstractModifyingItem() override = default;

    // ModifyingItem interface implementation
    std::vector<std::shared_ptr<Modification>> getModifications() const override;
    bool hasNoModifications() const override;
    std::shared_ptr<Modification> getModificationById(const std::string& modId) const override;
    
    // Additional methods
    void addModification(std::shared_ptr<ModificationImpl> modification);
    using AbstractIdentifiableItem::getId; // Resolve ambiguity

protected:
    void writeModifications(const std::shared_ptr<FileManipulator>& fileManipulator,
                          const std::string& itemDir);
    void readModifications(const std::string& itemDir);
    void writeModOrderFile(const std::shared_ptr<FileManipulator>& fileManipulator,
                          const std::string& itemDir);

private:
    std::vector<std::shared_ptr<ModificationImpl>> m_modifications;
};