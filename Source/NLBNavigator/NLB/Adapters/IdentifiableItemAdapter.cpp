#include "IdentifiableItemAdapter.h"

#include "NLBCore/Public/nlb/api/IdentifiableItem.h"

void UIdentifiableItemAdapter::SetCoreItem(IdentifiableItem* Item) {
    CoreItem = Item;
}

FString UIdentifiableItemAdapter::GetId() const {
    if (CoreItem) {
        return FString(UTF8_TO_TCHAR(CoreItem->getId().c_str()));
    }
    return FString();
}

FString UIdentifiableItemAdapter::GetFullId() const {
    if (CoreItem) {
        return FString(UTF8_TO_TCHAR(CoreItem->getFullId().c_str()));
    }
    return FString();
}

UIdentifiableItemAdapter* UIdentifiableItemAdapter::GetParent() const {
    if (CoreItem && CoreItem->getParent()) {
        UIdentifiableItemAdapter* ParentAdapter = NewObject<UIdentifiableItemAdapter>();
        ParentAdapter->SetCoreItem(CoreItem->getParentRaw());
        return ParentAdapter;
    }
    return nullptr;
}