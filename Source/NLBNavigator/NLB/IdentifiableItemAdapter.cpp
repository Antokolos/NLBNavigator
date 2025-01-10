#include "IdentifiableItemAdapter.h"

void UIdentifiableItemAdapter::SetCoreItem(IdentifiableItem* Item) {
    CoreItem = Item;
}

FString UIdentifiableItemAdapter::GetId() const {
    return CoreItem ? FString(CoreItem->GetId().c_str()) : FString();
}

FString UIdentifiableItemAdapter::GetFullId() const {
    return CoreItem ? FString(CoreItem->GetFullId().c_str()) : FString();
}

bool UIdentifiableItemAdapter::IsDeleted() const {
    return CoreItem ? CoreItem->IsDeleted() : false;
}

UIdentifiableItemAdapter* UIdentifiableItemAdapter::GetParent() const {
    if (!CoreItem || !CoreItem->GetParent()) {
        return nullptr;
    }

    UIdentifiableItemAdapter* ParentAdapter = NewObject<UIdentifiableItemAdapter>();
    ParentAdapter->SetCoreItem(CoreItem->GetParent());
    return ParentAdapter;
}

bool UIdentifiableItemAdapter::HasDeletedParent() const {
    return CoreItem ? CoreItem->HasDeletedParent() : false;
}
