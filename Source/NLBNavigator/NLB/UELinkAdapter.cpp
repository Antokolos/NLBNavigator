#include "UELinkAdapter.h"

FString UUELinkAdapter::GetId() const {
    return FString(CoreLink.getId().c_str());
}

FString UUELinkAdapter::GetDescription() const {
    return FString(CoreLink.getDescription().c_str());
}

void UUELinkAdapter::SetId(const FString& NewId) {
    CoreLink.setId(TCHAR_TO_UTF8(*NewId));
}

void UUELinkAdapter::SetDescription(const FString& NewDescription) {
    CoreLink.setDescription(TCHAR_TO_UTF8(*NewDescription));
}

FString UUELinkAdapter::GetTargetPageId() const {
    return FString(CoreLink.getTargetPageId().c_str());
}

void UUELinkAdapter::SetCoreLink(const Link& link) {
    CoreLink = link;
}

Link UUELinkAdapter::GetCoreLink() const {
    return CoreLink;
}