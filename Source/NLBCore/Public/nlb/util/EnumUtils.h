#pragma once

#include <string>
#include <stdexcept>

namespace EnumUtils {
    /*!
     * @brief Converts an enum to its string representation
     * @tparam E Enum type
     * @param value Enum value to convert
     * @return String representation of the enum value
     * @throws std::invalid_argument If no string mapping exists
     */
    template<typename E>
    std::string name(E value);

    // Specialization for DataType
    template<>
    std::string name(Variable::DataType value) {
        switch (value) {
            case Variable::DataType::AUTO: return "AUTO";
            case Variable::DataType::BOOLEAN: return "BOOLEAN";
            case Variable::DataType::NUMBER: return "NUMBER";
            case Variable::DataType::STRING: return "STRING";
            default: 
                throw std::invalid_argument("Unknown DataType");
        }
    }

    // Specialization for Type
    template<>
    std::string name(Variable::Type value) {
        switch (value) {
            case Variable::Type::PAGE: return "PAGE";
            case Variable::Type::TIMER: return "TIMER";
            case Variable::Type::OBJ: return "OBJ";
            case Variable::Type::OBJCONSTRAINT: return "OBJCONSTRAINT";
            case Variable::Type::OBJREF: return "OBJREF";
            case Variable::Type::LINK: return "LINK";
            case Variable::Type::LINKCONSTRAINT: return "LINKCONSTRAINT";
            case Variable::Type::VAR: return "VAR";
            case Variable::Type::EXPRESSION: return "EXPRESSION";
            case Variable::Type::TAG: return "TAG";
            case Variable::Type::MODCONSTRAINT: return "MODCONSTRAINT";
            case Variable::Type::AUTOWIRECONSTRAINT: return "AUTOWIRECONSTRAINT";
            default: 
                throw std::invalid_argument("Unknown Variable Type");
        }
    }

    /*!
     * @brief Converts a string to an enum value
     * @tparam E Enum type
     * @param str String to convert
     * @return Corresponding enum value
     * @throws std::invalid_argument If no enum mapping exists
     */
    template<typename E>
    E fromName(const std::string& str);

    // Specialization for DataType from string
    template<>
    Variable::DataType fromName(const std::string& str) {
        if (str == "AUTO") return Variable::DataType::AUTO;
        if (str == "BOOLEAN") return Variable::DataType::BOOLEAN;
        if (str == "NUMBER") return Variable::DataType::NUMBER;
        if (str == "STRING") return Variable::DataType::STRING;
        throw std::invalid_argument("Unknown DataType string: " + str);
    }

    // Specialization for Type from string
    template<>
    Variable::Type fromName(const std::string& str) {
        if (str == "PAGE") return Variable::Type::PAGE;
        if (str == "TIMER") return Variable::Type::TIMER;
        if (str == "OBJ") return Variable::Type::OBJ;
        if (str == "OBJCONSTRAINT") return Variable::Type::OBJCONSTRAINT;
        if (str == "OBJREF") return Variable::Type::OBJREF;
        if (str == "LINK") return Variable::Type::LINK;
        if (str == "LINKCONSTRAINT") return Variable::Type::LINKCONSTRAINT;
        if (str == "VAR") return Variable::Type::VAR;
        if (str == "EXPRESSION") return Variable::Type::EXPRESSION;
        if (str == "TAG") return Variable::Type::TAG;
        if (str == "MODCONSTRAINT") return Variable::Type::MODCONSTRAINT;
        if (str == "AUTOWIRECONSTRAINT") return Variable::Type::AUTOWIRECONSTRAINT;
        throw std::invalid_argument("Unknown Variable Type string: " + str);
    }
}