#include "ModifyingItemAdapter.h"
#include "ModificationAdapter.h"
#include "Modification.h"

void UModifyingItemAdapter::SetCoreModifyingItem(ModifyingItem* Item) {
    CoreModifyingItem = Item;
    SetCoreItem(Item); // Установка базового объекта
}

TArray<UModificationAdapter*> UModifyingItemAdapter::GetModifications() const {
    TArray<UModificationAdapter*> Result;
    if (CoreModifyingItem) {
        const auto& CoreModifications = CoreModifyingItem->GetModifications();
        for (const auto& CoreModification : CoreModifications) {
            if (CoreModification) {
                UModificationAdapter* Adapter = NewObject<UModificationAdapter>();
                Adapter->SetCoreModification(CoreModification.get());
                Result.Add(Adapter);
            }
        }
    }
    return Result;
}

bool UModifyingItemAdapter::HasNoModifications() const {
    return CoreModifyingItem ? CoreModifyingItem->HasNoModifications() : true;
}

UModificationAdapter* UModifyingItemAdapter::GetModificationById(const FString& ModId) const {
    if (!CoreModifyingItem) {
        return nullptr;
    }

    auto CoreModification = CoreModifyingItem->GetModificationById(TCHAR_TO_UTF8(*ModId));
    if (CoreModification) {
        UModificationAdapter* Adapter = NewObject<UModificationAdapter>();
        Adapter->SetCoreModification(CoreModification.get());
        return Adapter;
    }
    return nullptr;
}
