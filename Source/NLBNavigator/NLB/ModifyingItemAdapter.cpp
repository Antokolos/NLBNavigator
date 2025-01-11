#include "ModifyingItemAdapter.h"

void UModifyingItemAdapter::SetCoreModifyingItem(ModifyingItem* ModifyingItem) {
    CoreModifyingItem = ModifyingItem;
    SetCoreItem(ModifyingItem);
}

TArray<UModificationAdapter*> UModifyingItemAdapter::GetModifications() const {
    TArray<UModificationAdapter*> Result;
    if (CoreModifyingItem) {
        const auto& CoreMods = CoreModifyingItem->getModifications();
        for (const auto& Mod : CoreMods) {
            UModificationAdapter* Adapter = NewObject<UModificationAdapter>();
            Adapter->SetCoreModification(Mod.get());
            Result.Add(Adapter);
        }
    }
    return Result;
}

bool UModifyingItemAdapter::HasNoModifications() const {
    if (CoreModifyingItem) {
        return CoreModifyingItem->hasNoModifications();
    }
    return true;
}

UModificationAdapter* UModifyingItemAdapter::GetModificationById(const FString& ModId) const {
    if (CoreModifyingItem) {
        std::string StdModId = TCHAR_TO_UTF8(*ModId);
        auto Mod = CoreModifyingItem->getModificationById(StdModId);
        if (Mod) {
            UModificationAdapter* Adapter = NewObject<UModificationAdapter>();
            Adapter->SetCoreModification(Mod.get());
            return Adapter;
        }
    }
    return nullptr;
}