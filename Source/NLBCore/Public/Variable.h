#pragma once

#include "IdentifiableItem.h"
#include <string>

/*!
 * @class Variable
 * @brief Interface for variable representation
 */
class NLBCORE_API Variable : public IdentifiableItem {
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

    virtual Type getType() = 0;
    virtual DataType getDataType() = 0;
    virtual std::string getName() = 0;
    virtual std::string getTarget() = 0;
    virtual std::string getValue() = 0;

    virtual ~Variable() = default;
};