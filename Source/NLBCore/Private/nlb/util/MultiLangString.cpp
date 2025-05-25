#include "nlb/util/MultiLangString.h"
#include "nlb/api/Constants.h"
#include "nlb/util/StringHelper.h"

MultiLangString::MultiLangString(const MultiLangString& source) 
    : m_content(source.m_content) {
}

MultiLangString& MultiLangString::operator=(const MultiLangString& other) {
    if (this != &other) {
        m_content = other.m_content;
    }
    return *this;
}

MultiLangString MultiLangString::createEmptyText() {
    return MultiLangString();
}

MultiLangString MultiLangString::createDefaultLinkText() {
    MultiLangString linkText;
    linkText.put(nlb::Constants::RU, "Далее");
    linkText.put(nlb::Constants::EN, "Next");
    return linkText;
}

MultiLangString MultiLangString::createDefaultTraverseText() {
    MultiLangString linkText;
    linkText.put(nlb::Constants::RU, "Зайти внутрь");
    linkText.put(nlb::Constants::EN, "Go inside");
    return linkText;
}

MultiLangString MultiLangString::createCopy(const MultiLangString& source) {
    return MultiLangString(source);
}

std::set<std::string> MultiLangString::keySet() const {
    std::set<std::string> keys;
    for (const auto& pair : m_content) {
        keys.insert(pair.first);
    }
    return keys;
}

std::vector<std::string> MultiLangString::values() const {
    std::vector<std::string> vals;
    vals.reserve(m_content.size());
    for (const auto& pair : m_content) {
        vals.push_back(pair.second);
    }
    return vals;
}

void MultiLangString::put(const std::string& langKey, const std::string& value) {
    // Проверяем, что ключ не пустой (аналог проверки на null в Java)
    if (langKey.empty()) {
        return; // Игнорируем пустые ключи
    }
    
    // В Java допускаются пустые значения, поэтому добавляем любое значение
    m_content[langKey] = value;
}

std::string MultiLangString::get(const std::string& langKey) const {
    // Проверяем, что ключ не пустой
    if (langKey.empty()) {
        return nlb::Constants::EMPTY_STRING;
    }
    
    auto it = m_content.find(langKey);
    return it != m_content.end() ? it->second : nlb::Constants::EMPTY_STRING;
}

bool MultiLangString::equalsAs(const std::string& langKey, const MultiLangString& mlsToCompare) const {
    // Проверяем, что ключ не пустой
    if (langKey.empty()) {
        return true; // Пустые ключи считаются равными
    }
    
    auto it1 = m_content.find(langKey);
    auto it2 = mlsToCompare.m_content.find(langKey);
    
    const std::string& contentText = (it1 != m_content.end()) ? it1->second : "";
    const std::string& contentTextToCompare = (it2 != mlsToCompare.m_content.end()) ? it2->second : "";

    // Проверяем случай, когда текущий текст пустой
    if (StringHelper::isEmpty(contentText)) {
        return StringHelper::isEmpty(contentTextToCompare);
    }
    
    return contentText == contentTextToCompare;
}

bool MultiLangString::isSubsetOf(const MultiLangString& mlsToCompare) const {
    for (const auto& entry : m_content) {
        const std::string& key = entry.first;
        const std::string& value = entry.second;
        
        // Пропускаем пустые ключи
        if (key.empty()) {
            continue;
        }
        
        const std::string& valueToCompare = mlsToCompare.get(key);
        
        // Если сравниваемое значение пустое, а наше не пустое - не подмножество
        if (StringHelper::isEmpty(valueToCompare) && !StringHelper::isEmpty(value)) {
            return false;
        }
        
        // Если значения не равны - не подмножество
        if (valueToCompare != value) {
            return false;
        }
    }
    return true;
}

bool MultiLangString::isEmpty() const {
    for (const auto& entry : m_content) {
        // Пропускаем пустые ключи
        if (entry.first.empty()) {
            continue;
        }
        
        if (StringHelper::notEmpty(entry.second)) {
            return false;
        }
    }
    return true;
}

bool MultiLangString::operator==(const MultiLangString& other) const {
    return m_content == other.m_content;
}

bool MultiLangString::operator!=(const MultiLangString& other) const {
    return !(*this == other);
}

std::size_t MultiLangString::hashCode() const {
    std::size_t result = 0;
    
    // Используем алгоритм хеширования, аналогичный Java HashMap
    for (const auto& entry : m_content) {
        // Пропускаем пустые ключи
        if (entry.first.empty()) {
            continue;
        }
        
        std::size_t keyHash = std::hash<std::string>{}(entry.first);
        std::size_t valueHash = std::hash<std::string>{}(entry.second);
        
        // Комбинируем хеши ключа и значения (аналог Java Map.Entry.hashCode())
        std::size_t entryHash = keyHash ^ valueHash;
        
        // Добавляем к общему хешу (аналог Java HashMap.hashCode())
        result += entryHash;
    }
    
    return result;
}