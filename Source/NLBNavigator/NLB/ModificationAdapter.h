#pragma once

#include "CoreMinimal.h"
#include "IdentifiableItemAdapter.h"
#include "ModificationAdapter.generated.h"

class Modification;

UCLASS(BlueprintType)
class NLBNAVIGATOR_API UModificationAdapter : public UIdentifiableItemAdapter {
    GENERATED_BODY()

private:
    Modification* CoreModification; // Чистый C++ интерфейс

public:
    void SetCoreModification(Modification* Mod);

    // Геттеры
    UFUNCTION(BlueprintCallable, Category = "Modification")
    bool IsExternal() const;

    UFUNCTION(BlueprintCallable, Category = "Modification")
    FString GetVarId() const;

    UFUNCTION(BlueprintCallable, Category = "Modification")
    FString GetExprId() const;

    UFUNCTION(BlueprintCallable, Category = "Modification")
    FString GetType() const;

    UFUNCTION(BlueprintCallable, Category = "Modification")
    bool ReturnsValue() const;

    UFUNCTION(BlueprintCallable, Category = "Modification")
    bool IsParametrized() const;
};
