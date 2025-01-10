#include "UEPageAdapter.h"
#include "UELinkAdapter.h"

FString UUEPageAdapter::GetId() const {
    return FString(CorePage.getId().c_str());
}

FString UUEPageAdapter::GetContent() const {
    return FString(CorePage.getContent().c_str());
}

TArray<UUELinkAdapter*> UUEPageAdapter::GetLinks() const {
    TArray<UUELinkAdapter*> LinkAdapters;

    // Получаем ссылки из CorePage и конвертируем их в адаптеры
    const std::vector<Link>& Links = CorePage.getLinks();
    for (const Link& CoreLink : Links) {
        UUELinkAdapter* LinkAdapter = NewObject<UUELinkAdapter>();
        if (LinkAdapter) {
            // Передаем CoreLink в адаптер
            LinkAdapter->SetCoreLink(CoreLink);
            LinkAdapters.Add(LinkAdapter);
        }
    }

    return LinkAdapters;
}

void UUEPageAdapter::SetId(const FString& NewId) {
    CorePage.SetId(TCHAR_TO_UTF8(*NewId));
}

void UUEPageAdapter::SetContent(const FString& NewContent) {
    CorePage.SetContent(TCHAR_TO_UTF8(*NewContent));
}

void UUEPageAdapter::SetLinks(const TArray<UUELinkAdapter*>& NewLinks) {
    std::vector<Link> CoreLinks;

    // Конвертируем адаптеры обратно в CoreLink
    for (UUELinkAdapter* LinkAdapter : NewLinks) {
        if (LinkAdapter) {
            CoreLinks.push_back(LinkAdapter->GetCoreLink());
        }
    }

    CorePage.SetLinks(CoreLinks);
}
