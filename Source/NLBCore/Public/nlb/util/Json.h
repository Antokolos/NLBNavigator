#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

/**
 * @brief Небольшой разбор JSON (для файла настроек nlb.json): объекты, массивы, строки (с \uXXXX),
 * числа, true/false/null. Без внешних зависимостей.
 */
class Json {
public:
    enum class Type { Null, Bool, Number, String, Array, Object };

    static std::shared_ptr<Json> parse(const std::string& text, std::string& error);

    Type type() const { return m_type; }
    bool isObject() const { return m_type == Type::Object; }
    /// Поле объекта (nullptr — нет)
    const Json* get(const std::string& key) const;
    const std::map<std::string, std::shared_ptr<Json>>& object() const { return m_object; }
    const std::vector<std::shared_ptr<Json>>& array() const { return m_array; }

    bool asBool(bool def) const { return m_type == Type::Bool ? m_bool : def; }
    double asNumber(double def) const { return m_type == Type::Number ? m_number : def; }
    std::string asString(const std::string& def = "") const { return m_type == Type::String ? m_string : def; }

private:
    friend class JsonParser;
    Type m_type = Type::Null;
    bool m_bool = false;
    double m_number = 0;
    std::string m_string;
    std::vector<std::shared_ptr<Json>> m_array;
    std::map<std::string, std::shared_ptr<Json>> m_object;
};
