#ifndef LINK_H
#define LINK_H

#include <string>

class NLBCORE_API Link {
private:
    std::string id;           // Идентификатор ссылки
    std::string description;  // Описание ссылки
    std::string targetPageId;

public:
    // Конструкторы
    Link() = default;
    Link(const std::string& id, const std::string& description);

    // Геттеры
    const std::string& getId() const;
    const std::string& getDescription() const;
    const std::string& getTargetPageId() const;

    // Сеттеры
    void setId(const std::string& newId);
    void setDescription(const std::string& newDescription);
    void setTargetPageId(const std::string& newTargetPageId);
};

#endif // LINK_H
