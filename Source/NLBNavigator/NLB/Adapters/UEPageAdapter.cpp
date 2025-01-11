#include "UEPageAdapter.h"
#include "UEMultiLangStringAdapter.h"
#include "UELinkAdapter.h"

FString UUEPageAdapter::GetId() const {
    return FString(CorePage.getId().c_str());
}

UUEMultiLangStringAdapter* UUEPageAdapter::GetContent() const {
    UUEMultiLangStringAdapter* MultiLangStringAdapter = NewObject<UUEMultiLangStringAdapter>();
    if (MultiLangStringAdapter) {
        MultiLangStringAdapter->SetCoreMultiLangString(CorePage.getContent());
    }
    return MultiLangStringAdapter;
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
    CorePage.setId(TCHAR_TO_UTF8(*NewId));
}

void UUEPageAdapter::SetContent(const UUEMultiLangStringAdapter* NewContent) {
    CorePage.setContent(NewContent->GetCoreMultiLangString());
}

void UUEPageAdapter::SetLinks(const TArray<UUELinkAdapter*>& NewLinks) {
    std::vector<Link> CoreLinks;

    // Конвертируем адаптеры обратно в CoreLink
    for (UUELinkAdapter* LinkAdapter : NewLinks) {
        if (LinkAdapter) {
            CoreLinks.push_back(LinkAdapter->GetCoreLink());
        }
    }

    CorePage.setLinks(CoreLinks);
}
