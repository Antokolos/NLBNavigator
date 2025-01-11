#pragma once

#include <string>
#include "IdentifiableItem.h"

class Modification : public IdentifiableItem {
public:
    static const bool DEFAULT_EXTERNAL = false;
    
    enum class Type {
        ASSIGN,
        TAG,
        GETTAG,
        WHILE,
        IF,
        IFHAVE,
        ELSE,
        ELSEIF,
        END,
        RETURN,
        HAVE,
        CLONE,
        CNTNR,
        ID,
        ADD,
        ADDU,
        ADDINV,
        ADDALL,
        ADDALLU,
        REMOVE,
        RMINV,
        CLEAR,
        CLRINV,
        OBJS,
        SSND,
        WSND,
        SND,
        SPUSH,
        WPUSH,
        PUSH,
        POP,
        SINJECT,
        INJECT,
        EJECT,
        SHUFFLE,
        PRN,
        DSC,
        PDSC,
        PDSCS,
        ACT,
        ACTT,
        ACTF,
        USE,
        SIZE,
        RND,
        ACHMAX,
        ACHIEVE,
        ACHIEVED,
        GOTO,
        SNAPSHOT,
        COUNTGET,
        COUNTRST,
        OPENURL,
        WINGEOM,
        INVGEOM,
        WINCOLOR,
        INVCOLOR
    };

    // Pure virtual functions
    virtual bool isExternal() = 0;
    virtual std::string getVarId() = 0;
    virtual std::string getExprId() = 0;
    virtual Type getType() = 0;

    /*!
     * @brief Determines whether this modification has void return type or not.
     * @return true if this modification returns some result,
     *         false otherwise
     */
    virtual bool returnsValue() = 0;

    /*!
     * @brief Determines whether this modification has parameters or not.
     * @return true if this modification has parameters,
     *         false otherwise
     */
    virtual bool isParametrized() = 0;

    // Virtual destructor for proper cleanup in derived classes
    virtual ~Modification() = default;
};