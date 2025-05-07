#pragma once

#include <vector>
#include <map>
#include <string>
#include <memory>

class NonLinearBook;
class ModificationImpl;
class VariableImpl;
class Modification;
class Variable;
class ModifyingItem;

/*!
 * @brief Table model for modifications
 *
 * Provides functionality for working with a table of modifications and variables,
 * supporting editing, adding, removing, and moving operations.
 */
class NLBCORE_API ModificationsTableModel {
public:
    /*!
     * @brief Constructs a new ModificationsTableModel
     *
     * Creates a model with copies of the provided modifications and their related variables
     *
     * @param nlb Current NonLinearBook instance
     * @param modifications List of modifications to populate the model
     */
    ModificationsTableModel(
        std::shared_ptr<NonLinearBook> nlb,
        const std::vector<std::shared_ptr<Modification>>& modifications
    );

    /*!
     * @brief Gets the name of the specified column
     *
     * @param column Column index
     * @return Column name string
     */
    std::string getColumnName(int column) const;

    /*!
     * @brief Gets the number of rows in the model
     *
     * Only counts non-deleted modifications
     *
     * @return Number of rows
     */
    int getRowCount() const;

    /*!
     * @brief Gets the number of columns in the model
     *
     * @return Number of columns (always 6)
     */
    int getColumnCount() const;

    /*!
     * @brief Gets the value at specified position
     *
     * @param rowIndex Row index
     * @param columnIndex Column index
     * @return Value at the specified position
     */
    std::string getValueAt(int rowIndex, int columnIndex) const;

    /*!
     * @brief Sets the value at specified position
     *
     * @param aValue New value
     * @param rowIndex Row index 
     * @param columnIndex Column index
     * @return True if value was set successfully, false otherwise
     */
    bool setValueAt(const std::string& aValue, int rowIndex, int columnIndex);
    
    /*!
     * @brief Sets the boolean value at specified position
     *
     * Special case for boolean values (external flag)
     *
     * @param aValue New boolean value
     * @param rowIndex Row index
     * @param columnIndex Column index
     * @return True if value was set successfully, false otherwise
     */
    bool setValueAt(bool aValue, int rowIndex, int columnIndex);

    /*!
     * @brief Checks if a cell is editable
     *
     * @param rowIndex Row index
     * @param columnIndex Column index
     * @return True if cell is editable, false otherwise
     */
    bool isCellEditable(int rowIndex, int columnIndex) const;

    /*!
     * @brief Gets the modification at specified row
     *
     * @param rowIndex Row index
     * @return Shared pointer to the modification
     */
    std::shared_ptr<ModificationImpl> getModificationAt(int rowIndex) const;

    /*!
     * @brief Gets the modification IDs at specified rows
     *
     * @param rowIndices Array of row indices
     * @param count Number of indices in the array
     * @return Vector of modification IDs
     */
    std::vector<std::string> getModificationIdsAt(const int* rowIndices, int count) const;

    /*!
     * @brief Adds a new modification to the model
     *
     * @param modifyingItem Item that owns the modification
     */
    void add(std::shared_ptr<ModifyingItem> modifyingItem);

    /*!
     * @brief Removes modifications with the specified IDs
     *
     * @param modificationIds List of modification IDs to remove
     */
    void remove(const std::vector<std::string>& modificationIds);

    /*!
     * @brief Gets all modifications in the model
     *
     * @return Vector of modifications
     */
    std::vector<std::shared_ptr<Modification>> getModifications() const;

    /*!
     * @brief Gets the variable map
     *
     * @return Map of variable IDs to variables
     */
    std::map<std::string, std::shared_ptr<Variable>> getVariableMap() const;

    /*!
     * @brief Moves the modification at specified row up
     *
     * @param rowIndex Row index
     */
    void moveUp(int rowIndex);

    /*!
     * @brief Moves the modification at specified row down
     *
     * @param rowIndex Row index
     */
    void moveDown(int rowIndex);

private:
    /*!
     * @brief Sets the data type for a modification's variables
     *
     * @param modification Modification to update
     * @param dataType Data type string
     */
    void setDataType(std::shared_ptr<ModificationImpl> modification, const std::string& dataType);

    /*!
     * @brief Gets the variable associated with a modification
     *
     * @param modification Modification
     * @return The associated variable or nullptr
     */
    std::shared_ptr<VariableImpl> getVariable(const std::shared_ptr<Modification>& modification) const;

    /*!
     * @brief Gets the expression associated with a modification
     *
     * @param modification Modification
     * @return The associated expression or nullptr
     */
    std::shared_ptr<VariableImpl> getExpression(const std::shared_ptr<Modification>& modification) const;

    std::shared_ptr<NonLinearBook> m_currentNLB;
    std::vector<std::shared_ptr<ModificationImpl>> m_modifications;
    std::map<std::string, std::shared_ptr<VariableImpl>> m_variableMap;
};