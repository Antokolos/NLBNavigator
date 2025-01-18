#include "UELinkAdapter.h"
#include "NLBCore/Public/nlb/api/Link.h"

FString UUELinkAdapter::GetId() const {
    return FString(CoreLink->getId().c_str());
}

void UUELinkAdapter::SetId(const FString& NewId) {
    //CoreLink->setId(TCHAR_TO_UTF8(*NewId));
}

void UUELinkAdapter::SetCoreLink(Link* link) {
    CoreLink = link;
}

const Link* UUELinkAdapter::GetCoreLink() const {
    return CoreLink;
}