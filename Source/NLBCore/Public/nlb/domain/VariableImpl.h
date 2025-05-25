#pragma once

#include "nlb/domain/AbstractIdentifiableItem.h"
#include "nlb/api/Variable.h"
#include "nlb/util/FileManipulator.h"
#include "nlb/util/FileUtils.h"
#include "nlb/exception/NLBExceptions.h"
#include <string>
#include <memory>
#include <vector>

/*!
 * @brief Implementation of the Variable interface
 */
class VariableImpl : public AbstractIdentifiableItem, public Variable {
private:
    static const std::string TYPE_FILE_NAME;
    static const std::string DATATYPE_FILE_NAME;
    static const std::string NAME_FILE_NAME;
    static const std::string TARGET_FILE_NAME;
    static const std::string VALUE_FILE_NAME;

    Type m_type;
    DataType m_dataType;
    std::string m_name;
    std::string m_target;
    std::string m_value;

public:
    /*!
     * @brief Default constructor
     * @note Needed for JAXB conversion, do not use for other purposes
     */
    VariableImpl();

    /*!
     * @brief Constructor with current NonLinearBook
     * @param currentNLB Current NonLinearBook instance
     */
    explicit VariableImpl(std::shared_ptr<NonLinearBook> currentNLB);

    /*!
     * @brief Copy constructor
     * @param variable Source variable to copy
     * @param currentNLB Current NonLinearBook instance
     */
    VariableImpl(const std::shared_ptr<Variable>& variable, 
                 std::shared_ptr<NonLinearBook> currentNLB);

    /*!
     * @brief Detailed constructor
     * @param currentNLB Current NonLinearBook instance
     * @param type Variable type
     * @param dataType Variable data type
     * @param name Variable name
     * @param value Variable value
     * @param target Variable target
     */
    VariableImpl(std::shared_ptr<NonLinearBook> currentNLB,
                 Type type,
                 DataType dataType,
                 const std::string& name,
                 const std::string& value,
                 const std::string& target);

    /*!
     * @brief Copy method to copy variable properties
     * @param variable Source variable to copy from
     */
    void copy(const std::shared_ptr<Variable>& variable);

    // Getter methods from Variable interface
    Type getType() const override { return m_type; }
    DataType getDataType() const override { return m_dataType; }
    std::string getName() const override { return m_name; }
    std::string getTarget() const override { return m_target; }
    std::string getValue() const override { return m_value; }

    // Setter methods
    void setType(Type type);
    void setDataType(DataType dataType);
    void setTarget(const std::string& target);
    void setName(const std::string& name);
    void setValue(const std::string& value);

    /*!
     * @brief Search text method implementation
     * @param contract Search contract
     * @return SearchResult pointer
     */
    virtual std::shared_ptr<SearchResult> searchText(const SearchContract& contract) const override {
        // TODO: Implement search logic for VariableImpl
        // This is a placeholder implementation
        return AbstractIdentifiableItem::searchText(contract);
    }

    /*!
     * @brief Read variable from file directory
     * @param varDir Directory containing variable files
     * @throws NLBIOException If IO error occurs
     * @throws NLBConsistencyException If data consistency issues occur
     */
    void readVariable(const std::string& varDir);

    /*!
     * @brief Write variable to file directory
     * @param fileManipulator File manipulation utility
     * @param varsDir Directory to write variables
     * @throws NLBIOException If IO error occurs
     * @throws NLBFileManipulationException If file manipulation error occurs
     * @throws NLBVCSException If version control system error occurs
     */
    void writeVariable(
        FileManipulator& fileManipulator,
        const std::string& varsDir
    );

    virtual std::string getId() const override { return AbstractIdentifiableItem::getId(); }
    virtual std::string getFullId() const override { return AbstractIdentifiableItem::getFullId(); }
    virtual bool isDeleted() const override { return AbstractIdentifiableItem::isDeleted(); }
    virtual std::shared_ptr<IdentifiableItem> getParent() const override { return AbstractIdentifiableItem::getParent(); }
    virtual bool hasDeletedParent() const override { return AbstractIdentifiableItem::hasDeletedParent(); }
    virtual std::shared_ptr<NonLinearBook> getCurrentNLB() const override { return AbstractIdentifiableItem::getCurrentNLB(); }
};