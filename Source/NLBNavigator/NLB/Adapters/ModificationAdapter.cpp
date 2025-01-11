#include "ModificationAdapter.h"

void UModificationAdapter::SetCoreModification(Modification* Modification) {
    CoreModification = Modification;
    SetCoreItem(Modification);
}

bool UModificationAdapter::IsExternal() const {
    if (CoreModification) {
        return CoreModification->isExternal();
    }
    return false;
}

FString UModificationAdapter::GetVarId() const {
    if (CoreModification) {
        return FString(UTF8_TO_TCHAR(CoreModification->getVarId().c_str()));
    }
    return FString();
}

FString UModificationAdapter::GetExprId() const {
    if (CoreModification) {
        return FString(UTF8_TO_TCHAR(CoreModification->getExprId().c_str()));
    }
    return FString();
}

EModificationType UModificationAdapter::GetType() const {
    if (CoreModification) {
        return static_cast<EModificationType>(CoreModification->getType());
    }
    return EModificationType::ASSIGN; // Default value
}

bool UModificationAdapter::ReturnsValue() const {
    if (CoreModification) {
        return CoreModification->returnsValue();
    }
    return false;
}

bool UModificationAdapter::IsParametrized() const {
    if (CoreModification) {
        return CoreModification->isParametrized();
    }
    return false;
}