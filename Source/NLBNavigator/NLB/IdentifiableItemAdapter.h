#pragma once

#include "CoreMinimal.h"
#include "IdentifiableItem.h"
#include "IdentifiableItemAdapter.generated.h"

UCLASS(BlueprintType)
class NLBNAVIGATOR_API UIdentifiableItemAdapter : public UObject {
    GENERATED_BODY()

private:
    IdentifiableItem* CoreItem; // Чистый C++ интерфейс

public:
    void SetCoreItem(IdentifiableItem* Item);

    // Геттеры
    UFUNCTION(BlueprintCallable, Category = "IdentifiableItem")
    FString GetId() const;

    UFUNCTION(BlueprintCallable, Category = "IdentifiableItem")
    FString GetFullId() const;

    UFUNCTION(BlueprintCallable, Category = "IdentifiableItem")
    bool IsDeleted() const;

    UFUNCTION(BlueprintCallable, Category = "IdentifiableItem")
    UIdentifiableItemAdapter* GetParent() const;

    UFUNCTION(BlueprintCallable, Category = "IdentifiableItem")
    bool HasDeletedParent() const;
};
