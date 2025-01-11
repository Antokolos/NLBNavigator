#ifndef PAGE_H
#define PAGE_H

#include <string>
#include <vector>
///#include "Link.h"
#include "Theme.h"
#include "MultiLangString.h"

class NLBCORE_API Page {
private:
    std::string id;                  // ID страницы
    MultiLangString content;         // Содержимое страницы
    ///std::vector<Link> links;         // Список ссылок
    Theme theme;                     // Тема страницы

public:
    Page() = default;

    // Конструкторы
    Page(const std::string& id, const MultiLangString& content/*, const std::vector<Link>& links*/, const Theme& theme);

    // Геттеры
    const std::string& getId() const;
    const MultiLangString& getContent() const;
    ///const std::vector<Link>& getLinks() const;
    const Theme& getTheme() const;

    // Сеттеры
    void setId(const std::string& newId);
    void setContent(const MultiLangString& newContent);
    ///void setLinks(const std::vector<Link>& newLinks);
    void setTheme(const Theme& newTheme);
};

#endif // PAGE_H
