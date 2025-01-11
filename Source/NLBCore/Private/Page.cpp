#include "Page.h"

Page::Page(const std::string& id, const MultiLangString& content/*, const std::vector<Link>& links*/, const Theme& theme)
    : id(id), content(content)/*, links(links)*/, theme(theme) {}

const std::string& Page::getId() const {
    return id;
}

const MultiLangString& Page::getContent() const {
    return content;
}

// const std::vector<Link>& Page::getLinks() const {
//     return links;
// }

const Theme& Page::getTheme() const {
    return theme;
}

void Page::setId(const std::string& newId) {
    id = newId;
}

void Page::setContent(const MultiLangString& newContent) {
    content = newContent;
}

// void Page::setLinks(const std::vector<Link>& newLinks) {
//     links = newLinks;
// }

void Page::setTheme(const Theme& newTheme) {
    theme = newTheme;
}
