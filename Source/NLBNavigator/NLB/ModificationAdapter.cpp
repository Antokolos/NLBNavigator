#include "ModificationAdapter.h"
#include "Modification.h"

void UModificationAdapter::SetCoreModification(Modification* Mod) {
    CoreModification = Mod;
    SetCoreItem(Mod); // Устанавливаем базовый объект
}

bool UModificationAdapter::IsExternal() const {
    return CoreModification ? CoreModification->IsExternal() : false;
}

FString UModificationAdapter::GetVarId() const {
    return CoreModification ? FString(CoreModification->GetVarId().c_str()) : FString();
}

FString UModificationAdapter::GetExprId() const {
    return CoreModification ? FString(CoreModification->GetExprId().c_str()) : FString();
}

FString UModificationAdapter::GetType() const {
    if (!CoreModification) {
        return FString();
    }
    switch (CoreModification->GetType()) {
        case Modification::Type::ASSIGN: return TEXT("ASSIGN");
        case Modification::Type::TAG: return TEXT("TAG");
        case Modification::Type::GETTAG: return TEXT("GETTAG");
        // Добавить все остальные типы...
        default: return TEXT("UNKNOWN");
    }
}

bool UModificationAdapter::ReturnsValue() const {
    return CoreModification ? CoreModification->ReturnsValue() : false;
}

bool UModificationAdapter::IsParametrized() const {
    return CoreModification ? CoreModification->IsParametrized() : false;
}
