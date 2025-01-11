#pragma once

#include "CoreMinimal.h"
#include "NLBCore/Public/IdentifiableItem.h"
#include "IdentifiableItemAdapter.generated.h"

UCLASS(BlueprintType)
class NLBNAVIGATOR_API UIdentifiableItemAdapter : public UObject {
    GENERATED_BODY()
private:
    IdentifiableItem* CoreItem;

public:
    void SetCoreItem(IdentifiableItem* Item);

    UFUNCTION(BlueprintCallable, Category = "IdentifiableItem")
    FString GetId() const;

    UFUNCTION(BlueprintCallable, Category = "IdentifiableItem")
    FString GetFullId() const;

    UFUNCTION(BlueprintCallable, Category = "IdentifiableItem")
    UIdentifiableItemAdapter* GetParent() const;
};