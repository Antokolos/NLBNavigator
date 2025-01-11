#pragma once

#include "CoreMinimal.h"
#include "NLBCore/Public/ModifyingItem.h"
#include "IdentifiableItemAdapter.h"
#include "ModificationAdapter.h"
#include "ModifyingItemAdapter.generated.h"

UCLASS(BlueprintType)
class NLBNAVIGATOR_API UModifyingItemAdapter : public UIdentifiableItemAdapter {
    GENERATED_BODY()
private:
    ModifyingItem* CoreModifyingItem;

public:
    void SetCoreModifyingItem(ModifyingItem* ModifyingItem);

    UFUNCTION(BlueprintCallable, Category = "ModifyingItem")
    TArray<UModificationAdapter*> GetModifications() const;

    UFUNCTION(BlueprintCallable, Category = "ModifyingItem")
    bool HasNoModifications() const;

    UFUNCTION(BlueprintCallable, Category = "ModifyingItem")
    UModificationAdapter* GetModificationById(const FString& ModId) const;
};