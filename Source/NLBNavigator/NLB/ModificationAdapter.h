#pragma once

#include "CoreMinimal.h"
#include "Modification.h"
#include "IdentifiableItemAdapter.h"
#include "ModificationAdapter.generated.h"

UENUM(BlueprintType)
enum class EModificationType : uint8 {
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

UCLASS(BlueprintType)
class NLBNAVIGATOR_API UModificationAdapter : public UIdentifiableItemAdapter {
    GENERATED_BODY()
private:
    Modification* CoreModification;

public:
    void SetCoreModification(Modification* Modification);

    UFUNCTION(BlueprintCallable, Category = "Modification")
    bool IsExternal() const;

    UFUNCTION(BlueprintCallable, Category = "Modification")
    FString GetVarId() const;

    UFUNCTION(BlueprintCallable, Category = "Modification")
    FString GetExprId() const;

    UFUNCTION(BlueprintCallable, Category = "Modification")
    EModificationType GetType() const;

    UFUNCTION(BlueprintCallable, Category = "Modification")
    bool ReturnsValue() const;

    UFUNCTION(BlueprintCallable, Category = "Modification")
    bool IsParametrized() const;
};