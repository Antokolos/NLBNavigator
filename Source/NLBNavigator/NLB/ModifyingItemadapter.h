#pragma once

#include "CoreMinimal.h"
#include "ModifyingItem.h"
#include "IdentifiableItemAdapter.h"
#include "ModifyingItemAdapter.generated.h"

class UModification;

UCLASS(BlueprintType)
class NLBNAVIGATOR_API UModifyingItemAdapter : public UIdentifiableItemAdapter {
    GENERATED_BODY()

private:
    ModifyingItem* CoreModifyingItem; // Чистый C++ интерфейс

public:
    void SetCoreModifyingItem(ModifyingItem* Item);

    // Геттеры
    UFUNCTION(BlueprintCallable, Category = "ModifyingItem")
    TArray<UModificationAdapter*> GetModifications() const;

    UFUNCTION(BlueprintCallable, Category = "ModifyingItem")
    bool HasNoModifications() const;

    UFUNCTION(BlueprintCallable, Category = "ModifyingItem")
    UModificationAdapter* GetModificationById(const FString& ModId) const;
};
