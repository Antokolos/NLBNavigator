#ifndef MODIFICATION_H
#define MODIFICATION_H

#include "IdentifiableItem.h"
#include <string>

class Modification : public IdentifiableItem {
public:
    virtual ~Modification() = default;

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

    virtual bool IsExternal() const = 0;
    virtual std::string GetVarId() const = 0;
    virtual std::string GetExprId() const = 0;
    virtual Type GetType() const = 0;
    virtual bool ReturnsValue() const = 0;
    virtual bool IsParametrized() const = 0;
};

#endif // MODIFICATION_H