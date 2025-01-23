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
    virtual std::vector<std::shared_ptr<Modification>> getModifications() const override;
    virtual bool hasNoModifications() const override;
    virtual std::shared_ptr<Modification> getModificationById(const std::string& modId) const override;
    
    // Additional methods
    void addModification(std::shared_ptr<ModificationImpl> modification);
 
    virtual void setId(const std::string& id) override { AbstractIdentifiableItem::setId(id); }
    virtual std::string getId() const override { return AbstractIdentifiableItem::getId(); }
    virtual std::string getFullId() const override { return AbstractIdentifiableItem::getFullId(); }
    virtual bool isDeleted() const override { return AbstractIdentifiableItem::isDeleted(); }
    virtual std::shared_ptr<IdentifiableItem> getParent() const override { return AbstractIdentifiableItem::getParent(); }
    virtual bool hasDeletedParent() const override { return AbstractIdentifiableItem::hasDeletedParent(); }
    virtual std::shared_ptr<NonLinearBook> getCurrentNLB() const override { return AbstractIdentifiableItem::getCurrentNLB(); }
    virtual std::shared_ptr<SearchResult> searchText(const SearchContract& contract) const override { return AbstractIdentifiableItem::searchText(contract); }

protected:
    void writeModifications(const std::shared_ptr<FileManipulator>& fileManipulator,
                          const std::string& itemDir);
    void readModifications(const std::string& itemDir);
    void writeModOrderFile(const std::shared_ptr<FileManipulator>& fileManipulator,
                          const std::string& itemDir);

private:
    std::vector<std::shared_ptr<ModificationImpl>> m_modifications;
};