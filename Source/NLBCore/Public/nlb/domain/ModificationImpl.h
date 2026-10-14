#pragma once

#include <string>
#include <set>
#include <memory>
#include "nlb/api/Modification.h"
#include "nlb/domain/AbstractIdentifiableItem.h"

class NonLinearBook;
class ModifyingItem;
class FileManipulator;
class SearchContract;
class SearchResult;

class ModificationImpl : public AbstractIdentifiableItem, public Modification {
public:
    ModificationImpl();
    explicit ModificationImpl(const ModifyingItem* parent);
    ModificationImpl(const Modification* modification,
                    NonLinearBook* currentNLB);
    ModificationImpl(const Modification* modification,
                    const ModifyingItem* parent,
                    NonLinearBook* currentNLB);

    virtual ~ModificationImpl() override = default;

    // Явно указываем, что используем реализации из AbstractIdentifiableItem
    // using AbstractIdentifiableItem::getId;
    // using AbstractIdentifiableItem::getParent;
    // using AbstractIdentifiableItem::hasDeletedParent;
    // using AbstractIdentifiableItem::getFullId;
    // using AbstractIdentifiableItem::isDeleted;
    // using AbstractIdentifiableItem::getCurrentNLB;
    virtual std::string getId() const { return AbstractIdentifiableItem::getId(); };
    virtual std::string getFullId() const { return AbstractIdentifiableItem::getFullId(); };
    virtual bool isDeleted() const { return AbstractIdentifiableItem::isDeleted(); };
    virtual IdentifiableItem* getParent() const { return AbstractIdentifiableItem::getParent(); };
    virtual bool hasDeletedParent() const { return AbstractIdentifiableItem::hasDeletedParent(); };
    virtual NonLinearBook* getCurrentNLB() const { return AbstractIdentifiableItem::getCurrentNLB(); };
    
    void copy(const Modification* modification);
    
    // Implement Modification interface
    bool isExternal() const override;
    void setExternal(bool external);
    std::string getVarId() const override;
    void setVarId(const std::string& varId);
    std::string getExprId() const override;
    void setExprId(const std::string& exprId);
    Type getType() const override;
    void setType(const Type& type);
    void setType(const std::string& type);
    bool returnsValue() const override;
    bool isParametrized() const override;

    SearchResult* searchText(const SearchContract& contract) const override;

    void writeModification(const FileManipulator* fileManipulator,
                         const std::string& modificationsDir);
    void readModification(const std::string& modificationDir);
    
    bool isModificationImpl() const override { return true; }

	std::string toString() const;
	std::string toStringWithType() const;
    bool equals(const Modification* other) const;
	bool operator==(const ModificationImpl& other) const;
	size_t hash() const;
    
private:
    static const std::string EXTERNAL_FILE_NAME;
    static const std::string VARID_FILE_NAME;
    static const std::string TYPE_FILE_NAME;
    static const std::string EXPRID_FILE_NAME;
    static const std::set<Type> UNPARAMETRIZED_TYPES;

    bool m_external;
    Type m_type;
    std::string m_varId;
    std::string m_exprId;
};
