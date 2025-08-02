#pragma once

#include "IdentifiableItem.h"
#include <string>

/*!
 * @class Variable
 * @brief Interface for variable representation
 */
class Variable : public IdentifiableItem {
public:
    static const std::string NA;
    static const std::string DEFAULT_NAME;
    static const std::string DEFAULT_VALUE;

    /*!
     * @brief The DataType enum represents possible data type of the variable
     * (as the result of JS evaluation).
     * 
     * @author Anton P. Kolosov
     */
    enum class DataType {
        AUTO,
        BOOLEAN,
        NUMBER,
        STRING
    };

    static const DataType DEFAULT_DATATYPE;

    enum class Type {
        PAGE,
        TIMER,
        OBJ,
        OBJCONSTRAINT,
        OBJREF,
        LINK,
        LINKCONSTRAINT,
        VAR,
        EXPRESSION,
        TAG,
        MODCONSTRAINT,
        AUTOWIRECONSTRAINT
    };

    virtual Type getType() const = 0;
    virtual DataType getDataType() const = 0;
    virtual std::string getName() const = 0;
    virtual std::string getTarget() const = 0;
    virtual std::string getValue() const = 0;

    /*!
     * @brief Converts DataType enum to string representation
     * @param dataType DataType enum value
     * @return String representation of the data type
     */
    static std::string dataTypeToString(DataType dataType);

    /*!
     * @brief Converts string to DataType enum
     * @param dataTypeStr String representation of data type
     * @return DataType enum value
     */
    static DataType stringToDataType(const std::string& dataTypeStr);

    /*!
     * @brief Converts Type enum to string representation
     * @param type Type enum value
     * @return String representation of the type
     */
    static std::string typeToString(Type type);

    /*!
     * @brief Converts string to Type enum
     * @param typeStr String representation of type
     * @return Type enum value
     */
    static Type stringToType(const std::string& typeStr);

    virtual ~Variable() = default;
};