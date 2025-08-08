#include "nlb/api/ModificationsTableModel.h"
#include "nlb/api/NonLinearBook.h"
#include "nlb/domain/ModificationImpl.h"
#include "nlb/domain/VariableImpl.h"
#include "nlb/api/Variable.h"
#include "nlb/api/ModifyingItem.h"
#include "nlb/util/StringHelper.h"

#include <algorithm>

ModificationsTableModel::ModificationsTableModel(
    NonLinearBook* nlb,
    const std::vector<Modification*>& modifications
) : m_currentNLB(nlb) {
    // Adds COPIES of original modifications to the list
    for (const auto& modification : modifications) {
        auto varId = modification->getVarId();
        auto exprId = modification->getExprId();
        
        auto variable = nlb->getVariableById(varId);
        if (variable) {
            m_variableMap[varId] = new VariableImpl(variable, m_currentNLB);
        }

        auto expression = nlb->getVariableById(exprId);
        if (expression) {
            m_variableMap[exprId] = new VariableImpl(expression, m_currentNLB);
        }

        m_modifications.push_back(new ModificationImpl(modification, m_currentNLB));
    }
}

std::string ModificationsTableModel::getColumnName(int column) const {
    switch (column) {
        case 0:
            return "External";
        case 1:
            return "Modification Id";
        case 2:
            return "DataType";
        case 3:
            return "Variable";
        case 4:
            return "Operation";
        case 5:
            return "Expression";
        default:
            return "N/A";
    }
}

int ModificationsTableModel::getRowCount() const {
    int count = 0;
    for (const auto& modification : m_modifications) {
        if (!modification->isDeleted()) {
            ++count;
        }
    }
    return count;
}

int ModificationsTableModel::getColumnCount() const {
    return 6;
}

std::string ModificationsTableModel::getValueAt(int rowIndex, int columnIndex) const {
    auto modification = getModificationAt(rowIndex);
    if (!modification) {
        return "N/A";
    }

    switch (columnIndex) {
        case 0:
            return modification->isExternal() ? "true" : "false";
        case 1:
            return modification->getId();
        case 2: {
            auto expression = getExpression(modification);
            return (expression != nullptr) 
                ? Variable::dataTypeToString(expression->getDataType()) 
                : Variable::dataTypeToString(Variable::DataType::AUTO);
        }
        case 3: {
            auto variable = getVariable(modification);
            return (variable != nullptr) ? variable->getName() : "";
        }
        case 4:
            return Modification::typeToString(modification->getType());
        case 5: {
            auto expression = getExpression(modification);
            return (expression != nullptr) ? expression->getValue() : "";
        }
        default:
            return "N/A";
    }
}

bool ModificationsTableModel::setValueAt(const std::string& aValue, int rowIndex, int columnIndex) {
    auto modification = getModificationAt(rowIndex);
    if (!modification) {
        return false;
    }

    switch (columnIndex) {
        // case 0: handled by boolean overload
        // case 1: it is UNID, not changeable
        case 2:
            setDataType(modification, aValue);
            break;
        case 3: {
            auto variable = getVariable(modification);
            if (variable) {
                if (StringHelper::isEmpty(aValue)) {
                    variable->AbstractIdentifiableItem::setDeleted(true);
                    modification->setVarId("");
                } else {
                    variable->setName(aValue);
                }
            } else {
                if (!StringHelper::isEmpty(aValue)) {
                    auto expression = getExpression(modification);
                    auto newVariable = new VariableImpl(
                        m_currentNLB,
                        Variable::Type::VAR,
                        (expression != nullptr)
                            ? expression->getDataType()
                            : Variable::DataType::AUTO,
                        aValue,
                        Variable::DEFAULT_VALUE,
                        modification->getFullId()
                    );
                    m_variableMap[newVariable->getId()] = newVariable;
                    modification->setVarId(newVariable->getId());
                }
            }
            break;
        }
        case 4: {
            modification->setType(Modification::stringToType(aValue));
            auto expr = getExpression(modification);
            if (expr) {
                if (modification->getType() == Modification::Type::TAG) {
                    expr->setType(Variable::Type::TAG);
                } else {
                    expr->setType(Variable::Type::EXPRESSION);
                }
            }
            break;
        }
        case 5: {
            auto expression = getExpression(modification);
            if (expression) {
                if (StringHelper::isEmpty(aValue)) {
                    expression->AbstractIdentifiableItem::setDeleted(true);
                    modification->setExprId("");
                } else {
                    expression->setValue(aValue);
                }
            } else {
                if (!StringHelper::isEmpty(aValue)) {
                    auto newExpression = new VariableImpl(
                        m_currentNLB,
                        (modification->getType() == Modification::Type::TAG)
                            ? Variable::Type::TAG
                            : Variable::Type::EXPRESSION,
                        Variable::DataType::AUTO,
                        Variable::DEFAULT_NAME,
                        aValue,
                        modification->getFullId()
                    );
                    m_variableMap[newExpression->getId()] = newExpression;
                    modification->setExprId(newExpression->getId());
                }
            }
            break;
        }
        default:
            return false;
    }
    return true;
}

bool ModificationsTableModel::setValueAt(bool aValue, int rowIndex, int columnIndex) {
    if (columnIndex != 0) {
        return false;
    }

    auto modification = getModificationAt(rowIndex);
    if (!modification) {
        return false;
    }

    modification->setExternal(aValue);
    return true;
}

void ModificationsTableModel::setDataType(ModificationImpl* modification, const std::string& dataType) {
    auto variable = getVariable(modification);
    auto expression = getExpression(modification);
    
    Variable::DataType type = Variable::DataType::AUTO;
    
    if (dataType == "STRING") {
        type = Variable::DataType::STRING;
    } else if (dataType == "BOOLEAN") {
        type = Variable::DataType::BOOLEAN;
    } else if (dataType == "NUMBER") {
        type = Variable::DataType::NUMBER;
    }
    
    if (variable) {
        variable->setDataType(type);
    }
    
    if (expression) {
        expression->setDataType(type);
    }
}

VariableImpl* ModificationsTableModel::getVariable(const Modification* modification) const {
    auto varId = modification->getVarId();
    auto it = m_variableMap.find(varId);
    return (it != m_variableMap.end()) ? it->second : nullptr;
}

VariableImpl* ModificationsTableModel::getExpression(const Modification* modification) const {
    auto exprId = modification->getExprId();
    auto it = m_variableMap.find(exprId);
    return (it != m_variableMap.end()) ? it->second : nullptr;
}

bool ModificationsTableModel::isCellEditable(int rowIndex, int columnIndex) const {
    return columnIndex != 1;
}

ModificationImpl* ModificationsTableModel::getModificationAt(int rowIndex) const {
    int i = 0;
    for (const auto& modification : m_modifications) {
        if (!modification->isDeleted()) {
            if (i == rowIndex) {
                return modification;
            }
            ++i;
        }
    }
    return nullptr;
}

std::vector<std::string> ModificationsTableModel::getModificationIdsAt(const int* rowIndices, int count) const {
    std::vector<std::string> result;
    for (int i = 0; i < count; ++i) {
        auto mod = getModificationAt(rowIndices[i]);
        if (mod) {
            result.push_back(mod->getId());
        }
    }
    return result;
}

void ModificationsTableModel::add(ModifyingItem* modifyingItem) {
    m_modifications.push_back(new ModificationImpl(modifyingItem));
}

void ModificationsTableModel::remove(const std::vector<std::string>& modificationIds) {
    for (auto& modification : m_modifications) {
        if (std::find(modificationIds.begin(), modificationIds.end(), modification->getId()) != modificationIds.end()) {
            modification->AbstractIdentifiableItem::setDeleted(true);
        }
    }
}

std::vector<Modification*> ModificationsTableModel::getModifications() const {
    std::vector<Modification*> result;
    for (const auto& mod : m_modifications) {
        result.push_back(mod);
    }
    return result;
}

std::map<std::string, Variable*> ModificationsTableModel::getVariableMap() const {
    std::map<std::string, Variable*> result;
    for (const auto& [key, var] : m_variableMap) {
        result[key] = var;
    }
    return result;
}

void ModificationsTableModel::moveUp(int rowIndex) {
    if (rowIndex <= 0 || rowIndex >= static_cast<int>(m_modifications.size())) {
        return;
    }
    
    int currentIndex = -1;
    int previousNonDeletedIndex = -1;
    
    // Find the actual indices in the vector
    for (size_t i = 0; i < m_modifications.size(); ++i) {
        if (!m_modifications[i]->isDeleted()) {
            if (currentIndex == rowIndex - 1) {
                previousNonDeletedIndex = i;
            } else if (currentIndex == rowIndex) {
                // Swap with previous non-deleted item
                if (previousNonDeletedIndex != -1) {
                    std::swap(m_modifications[previousNonDeletedIndex], m_modifications[i]);
                }
                break;
            }
            ++currentIndex;
        }
    }
}

void ModificationsTableModel::moveDown(int rowIndex) {
    if (rowIndex < 0 || rowIndex >= getRowCount() - 1) {
        return;
    }
    
    int currentIndex = -1;
    int nextNonDeletedIndex = -1;
    
    // Find the actual indices in the vector
    for (size_t i = 0; i < m_modifications.size(); ++i) {
        if (!m_modifications[i]->isDeleted()) {
            ++currentIndex;
            if (currentIndex == rowIndex) {
                // Find next non-deleted item
                for (size_t j = i + 1; j < m_modifications.size(); ++j) {
                    if (!m_modifications[j]->isDeleted()) {
                        nextNonDeletedIndex = j;
                        break;
                    }
                }
                
                // Swap with next non-deleted item
                if (nextNonDeletedIndex != -1) {
                    std::swap(m_modifications[i], m_modifications[nextNonDeletedIndex]);
                }
                break;
            }
        }
    }
}
