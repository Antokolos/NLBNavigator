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

class AbstractModifyingItem : public AbstractIdentifiableItem, public ModifyingItem {
public:
    AbstractModifyingItem();
    explicit AbstractModifyingItem(NonLinearBook* currentNLB);
    AbstractModifyingItem(const ModifyingItem* modifyingItem,
                         NonLinearBook* currentNLB);
    virtual ~AbstractModifyingItem() override = default;

    // ModifyingItem interface implementation
    virtual std::vector<Modification*> getModifications() const override;
    virtual bool hasNoModifications() const override;
    virtual Modification* getModificationById(const std::string& modId) const override;
    
    // Additional methods
    void addModification(ModificationImpl* modification);
 
    virtual void setId(const std::string& id) override { AbstractIdentifiableItem::setId(id); }
    virtual std::string getId() const override { return AbstractIdentifiableItem::getId(); }
    virtual std::string getFullId() const override { return AbstractIdentifiableItem::getFullId(); }
    virtual bool isDeleted() const override { return AbstractIdentifiableItem::isDeleted(); }
    virtual IdentifiableItem* getParent() const override { return AbstractIdentifiableItem::getParent(); }
    virtual bool hasDeletedParent() const override { return AbstractIdentifiableItem::hasDeletedParent(); }
    virtual NonLinearBook* getCurrentNLB() const override { return AbstractIdentifiableItem::getCurrentNLB(); }
    virtual SearchResult* searchText(const SearchContract& contract) const override { return AbstractIdentifiableItem::searchText(contract); }

protected:
    void writeModifications(const FileManipulator* fileManipulator,
                          const std::string& itemDir);
    void readModifications(const std::string& itemDir);
    void writeModOrderFile(const FileManipulator* fileManipulator,
                          const std::string& itemDir);

private:
    std::vector<ModificationImpl*> m_modifications;
};
