#include "MultiLangString.h"
#include "Constants.h"
#include "StringHelper.h"

MultiLangString::MultiLangString(const MultiLangString& source) 
    : m_content(source.m_content) {
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
    m_content[langKey] = value;
}

std::string MultiLangString::get(const std::string& langKey) const {
    auto it = m_content.find(langKey);
    return it != m_content.end() ? it->second : nlb::Constants::EMPTY_STRING;
}

bool MultiLangString::equalsAs(const std::string& langKey, const MultiLangString& mlsToCompare) const {
    auto it1 = m_content.find(langKey);
    auto it2 = mlsToCompare.m_content.find(langKey);
    
    const std::string& contentText = (it1 != m_content.end()) ? it1->second : "";
    const std::string& contentTextToCompare = (it2 != mlsToCompare.m_content.end()) ? it2->second : "";

    if (StringHelper::isEmpty(contentText)) {
        return StringHelper::isEmpty(contentTextToCompare);
    }
    return contentText == contentTextToCompare;
}

bool MultiLangString::isSubsetOf(const MultiLangString& mlsToCompare) const {
    for (const auto& entry : m_content) {
        const std::string& valueToCompare = mlsToCompare.get(entry.first);
        if (StringHelper::isEmpty(valueToCompare) && !StringHelper::isEmpty(entry.second)) {
            return false;
        }
        if (valueToCompare != entry.second) {
            return false;
        }
    }
    return true;
}

bool MultiLangString::isEmpty() const {
    for (const auto& entry : m_content) {
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

size_t std::hash<MultiLangString>::operator()(const MultiLangString& mls) const {
    size_t h = 0;
    for (const auto& key : mls.keySet()) {
        h ^= std::hash<std::string>{}(key) << 1;
    }
    return h;
}