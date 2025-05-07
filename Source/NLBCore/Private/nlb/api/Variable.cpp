#include "nlb/api/Variable.h"

const std::string Variable::NA = "N/A";
const std::string Variable::DEFAULT_NAME = Variable::NA;
const std::string Variable::DEFAULT_VALUE = Variable::NA;
const Variable::DataType Variable::DEFAULT_DATATYPE = Variable::DataType::AUTO;

std::string Variable::dataTypeToString(DataType dataType) {
    switch (dataType) {
        case DataType::AUTO:
            return "AUTO";
        case DataType::BOOLEAN:
            return "BOOLEAN";
        case DataType::NUMBER:
            return "NUMBER";
        case DataType::STRING:
            return "STRING";
        default:
            return "AUTO";
    }
}

Variable::DataType Variable::stringToDataType(const std::string& dataTypeStr) {
    if (dataTypeStr == "BOOLEAN") {
        return DataType::BOOLEAN;
    } else if (dataTypeStr == "NUMBER") {
        return DataType::NUMBER;
    } else if (dataTypeStr == "STRING") {
        return DataType::STRING;
    } else {
        return DataType::AUTO;
    }
}

std::string Variable::typeToString(Type type) {
    switch (type) {
        case Type::PAGE:
            return "PAGE";
        case Type::TIMER:
            return "TIMER";
        case Type::OBJ:
            return "OBJ";
        case Type::OBJCONSTRAINT:
            return "OBJCONSTRAINT";
        case Type::OBJREF:
            return "OBJREF";
        case Type::LINK:
            return "LINK";
        case Type::LINKCONSTRAINT:
            return "LINKCONSTRAINT";
        case Type::VAR:
            return "VAR";
        case Type::EXPRESSION:
            return "EXPRESSION";
        case Type::TAG:
            return "TAG";
        case Type::MODCONSTRAINT:
            return "MODCONSTRAINT";
        case Type::AUTOWIRECONSTRAINT:
            return "AUTOWIRECONSTRAINT";
        default:
            return "VAR";
    }
}

Variable::Type Variable::stringToType(const std::string& typeStr) {
    if (typeStr == "PAGE") {
        return Type::PAGE;
    } else if (typeStr == "TIMER") {
        return Type::TIMER;
    } else if (typeStr == "OBJ") {
        return Type::OBJ;
    } else if (typeStr == "OBJCONSTRAINT") {
        return Type::OBJCONSTRAINT;
    } else if (typeStr == "OBJREF") {
        return Type::OBJREF;
    } else if (typeStr == "LINK") {
        return Type::LINK;
    } else if (typeStr == "LINKCONSTRAINT") {
        return Type::LINKCONSTRAINT;
    } else if (typeStr == "EXPRESSION") {
        return Type::EXPRESSION;
    } else if (typeStr == "TAG") {
        return Type::TAG;
    } else if (typeStr == "MODCONSTRAINT") {
        return Type::MODCONSTRAINT;
    } else if (typeStr == "AUTOWIRECONSTRAINT") {
        return Type::AUTOWIRECONSTRAINT;
    } else {
        return Type::VAR;
    }
}