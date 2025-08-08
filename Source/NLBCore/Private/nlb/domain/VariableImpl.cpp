#include "nlb/domain/VariableImpl.h"
#include "nlb/util/EnumUtils.h"
#include "nlb/api/Constants.h"
#include <fstream>
#include <sstream>

#include "nlb/domain/SearchResult.h"

// Static constant definitions
const std::string VariableImpl::TYPE_FILE_NAME = "type";
const std::string VariableImpl::DATATYPE_FILE_NAME = "datatype";
const std::string VariableImpl::NAME_FILE_NAME = "name";
const std::string VariableImpl::TARGET_FILE_NAME = "target";
const std::string VariableImpl::VALUE_FILE_NAME = "value";

// Helper function to read file contents
std::string readFileContents(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw NLBIOException("Cannot open file: " + filePath);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Helper function to write file contents
void writeFileContents(const std::string& filePath, const std::string& content) {
    std::ofstream file(filePath);
    if (!file.is_open()) {
        throw NLBIOException("Cannot open file for writing: " + filePath);
    }
    
    file << content;
    file.close();
}

// Constructors
VariableImpl::VariableImpl()
    : AbstractIdentifiableItem()
    , m_type(Type::VAR)
    , m_dataType(DEFAULT_DATATYPE)
    , m_name(DEFAULT_NAME)
    , m_target("")
    , m_value(DEFAULT_VALUE) {
}

VariableImpl::VariableImpl(NonLinearBook* currentNLB)
    : AbstractIdentifiableItem(currentNLB)
    , m_type(Type::VAR)
    , m_dataType(DEFAULT_DATATYPE)
    , m_name(DEFAULT_NAME)
    , m_target("")
    , m_value(DEFAULT_VALUE) {
}

VariableImpl::VariableImpl(const Variable* variable,
                           NonLinearBook* currentNLB)
    : AbstractIdentifiableItem(variable, currentNLB)
    , m_type(variable->getType())
    , m_dataType(variable->getDataType())
    , m_name(variable->getName())
    , m_target(variable->getTarget())
    , m_value(variable->getValue()) {
}

VariableImpl::VariableImpl(NonLinearBook* currentNLB,
                           Type type,
                           DataType dataType,
                           const std::string& name,
                           const std::string& value,
                           const std::string& target)
    : AbstractIdentifiableItem(currentNLB)
    , m_type(type)
    , m_dataType(dataType)
    , m_name(name)
    , m_target(target)
    , m_value(value) {
}

void VariableImpl::copy(const Variable* variable) {
    AbstractIdentifiableItem::copy(variable);
    m_type = variable->getType();
    m_dataType = variable->getDataType();
    m_name = variable->getName();
    m_target = variable->getTarget();
    m_value = variable->getValue();
}

SearchResult* VariableImpl::searchText(const SearchContract& contract) const {
    auto result = AbstractIdentifiableItem::searchText(contract);
    if (result) {
        return result;
    }
    
    result = new SearchResult();
    if (m_name != DEFAULT_NAME && textMatches(m_name, contract)) {
        result->setId(getId());
        result->setInformation(m_name);
        return result;
    } else if (m_value != DEFAULT_VALUE && textMatches(m_value, contract)) {
        result->setId(getId());
        result->setInformation(m_value);
        return result;
    }
    
    return nullptr;
}

void VariableImpl::readVariable(const std::string& varDir) {
    setId(FileUtils::getFileName(varDir));

    std::string typeFilePath = FileUtils::combinePath(varDir, TYPE_FILE_NAME);
    std::string type = readFileContents(typeFilePath);
    
    // Remove any whitespace
    type.erase(type.find_last_not_of(" \n\r\t") + 1);
    type.erase(0, type.find_first_not_of(" \n\r\t"));

    // Type conversion logic (handle legacy COMMONTO type)
    if (type == "PAGE") m_type = Type::PAGE;
    else if (type == "TIMER") m_type = Type::TIMER;
    else if (type == "OBJ") m_type = Type::OBJ;
    else if (type == "OBJCONSTRAINT") m_type = Type::OBJCONSTRAINT;
    else if (type == "COMMONTO" || type == "OBJREF") m_type = Type::OBJREF;
    else if (type == "LINK") m_type = Type::LINK;
    else if (type == "LINKCONSTRAINT") m_type = Type::LINKCONSTRAINT;
    else if (type == "VAR") m_type = Type::VAR;
    else if (type == "TAG") m_type = Type::TAG;
    else if (type == "EXPRESSION") m_type = Type::EXPRESSION;
    else if (type == "MODCONSTRAINT") m_type = Type::MODCONSTRAINT;
    else if (type == "AUTOWIRECONSTRAINT") m_type = Type::AUTOWIRECONSTRAINT;
    else {
        throw NLBConsistencyException(
            "Variable type '" + type + "' cannot be determined for variable with Id = " + getId()
        );
    }

    // DataType determination logic
    switch (m_type) {
        case Type::PAGE:
        case Type::OBJ:
        case Type::LINK:
        case Type::OBJCONSTRAINT:
        case Type::LINKCONSTRAINT:
        case Type::MODCONSTRAINT:
        case Type::AUTOWIRECONSTRAINT:
            m_dataType = DataType::BOOLEAN;
            break;
        case Type::TIMER:
            m_dataType = DataType::NUMBER;
            break;
        case Type::OBJREF:
            m_dataType = DataType::STRING;
            break;
        default: {
            std::string dataTypeFilePath = FileUtils::combinePath(varDir, DATATYPE_FILE_NAME);
            std::string dataType = EnumUtils::name(DEFAULT_DATATYPE);
            
            if (FileUtils::exists(dataTypeFilePath)) {
                dataType = readFileContents(dataTypeFilePath);
                // Remove any whitespace
                dataType.erase(dataType.find_last_not_of(" \n\r\t") + 1);
                dataType.erase(0, dataType.find_first_not_of(" \n\r\t"));
            }

            if (dataType == "AUTO") m_dataType = DataType::AUTO;
            else if (dataType == "BOOLEAN") m_dataType = DataType::BOOLEAN;
            else if (dataType == "NUMBER") m_dataType = DataType::NUMBER;
            else if (dataType == "STRING") m_dataType = DataType::STRING;
            else {
                throw NLBConsistencyException(
                    "Variable datatype '" + dataType +
                    "' cannot be determined for variable with Id = " + getId()
                );
            }
            break;
        }
    }

    // Read name (optional)
    std::string nameFilePath = FileUtils::combinePath(varDir, NAME_FILE_NAME);
    m_name = DEFAULT_NAME;
    if (FileUtils::exists(nameFilePath)) {
        m_name = readFileContents(nameFilePath);
        // Remove any whitespace
        m_name.erase(m_name.find_last_not_of(" \n\r\t") + 1);
        m_name.erase(0, m_name.find_first_not_of(" \n\r\t"));
    }

    // Read value (optional)
    std::string valueFilePath = FileUtils::combinePath(varDir, VALUE_FILE_NAME);
    m_value = DEFAULT_VALUE;
    if (FileUtils::exists(valueFilePath)) {
        m_value = readFileContents(valueFilePath);
        // Remove any whitespace
        m_value.erase(m_value.find_last_not_of(" \n\r\t") + 1);
        m_value.erase(0, m_value.find_first_not_of(" \n\r\t"));
    }

    // Read target (required)
    std::string targetFilePath = FileUtils::combinePath(varDir, TARGET_FILE_NAME);
    m_target = readFileContents(targetFilePath);
    // Remove any whitespace
    m_target.erase(m_target.find_last_not_of(" \n\r\t") + 1);
    m_target.erase(0, m_target.find_first_not_of(" \n\r\t"));
}

void VariableImpl::writeVariable(
    FileManipulator& fileManipulator,
    const std::string& varsDir
) {
    std::string varDir = FileUtils::combinePath(varsDir, getId());
    
    if (isDeleted()) {
        // Completely remove variable directory
        FileUtils::remove(varDir, true);
    } else {
        // Create directory
        if (!FileUtils::exists(varDir)) {
            if (!FileUtils::createDirectory(varDir)) {
                throw NLBIOException(
                    "Cannot create NLB variable directory for variable with Id = " + getId()
                );
            }
        }

        // Write type (required)
        std::string typeFilePath = FileUtils::combinePath(varDir, TYPE_FILE_NAME);
        writeFileContents(typeFilePath, EnumUtils::name(m_type));

        // Write datatype (optional, only if different from default)
        if (m_dataType != DEFAULT_DATATYPE) {
            std::string dataTypeFilePath = FileUtils::combinePath(varDir, DATATYPE_FILE_NAME);
            writeFileContents(dataTypeFilePath, EnumUtils::name(m_dataType));
        } else {
            std::string dataTypeFilePath = FileUtils::combinePath(varDir, DATATYPE_FILE_NAME);
            if (FileUtils::exists(dataTypeFilePath)) {
                FileUtils::remove(dataTypeFilePath);
            }
        }

        // Write name (optional, only if different from default)
        if (m_name != DEFAULT_NAME) {
            std::string nameFilePath = FileUtils::combinePath(varDir, NAME_FILE_NAME);
            writeFileContents(nameFilePath, m_name);
        } else {
            std::string nameFilePath = FileUtils::combinePath(varDir, NAME_FILE_NAME);
            if (FileUtils::exists(nameFilePath)) {
                FileUtils::remove(nameFilePath);
            }
        }

        // Write value (optional, only if different from default)
        if (m_value != DEFAULT_VALUE) {
            std::string valueFilePath = FileUtils::combinePath(varDir, VALUE_FILE_NAME);
            writeFileContents(valueFilePath, m_value);
        } else {
            std::string valueFilePath = FileUtils::combinePath(varDir, VALUE_FILE_NAME);
            if (FileUtils::exists(valueFilePath)) {
                FileUtils::remove(valueFilePath);
            }
        }

        // Write target (required)
        std::string targetFilePath = FileUtils::combinePath(varDir, TARGET_FILE_NAME);
        writeFileContents(targetFilePath, m_target);
    }
}

// Setter implementations
void VariableImpl::setType(Type type) {
    m_type = type;
}

void VariableImpl::setDataType(DataType dataType) {
    m_dataType = dataType;
}

void VariableImpl::setTarget(const std::string& target) {
    m_target = target;
}

void VariableImpl::setName(const std::string& name) {
    m_name = name;
}

void VariableImpl::setValue(const std::string& value) {
    m_value = value;
}
