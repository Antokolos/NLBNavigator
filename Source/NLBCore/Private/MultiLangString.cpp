#include "MultiLangString.h"

std::string MultiLangString::getTranslation(const std::string& language) const {
    auto it = translations.find(language);
    if (it != translations.end()) {
        return it->second;
    }
    return "";
}

void MultiLangString::setTranslation(const std::string& language, const std::string& text) {
    translations[language] = text;
}

bool MultiLangString::hasTranslation(const std::string& language) const {
    return translations.find(language) != translations.end();
}

void MultiLangString::removeTranslation(const std::string& language) {
    translations.erase(language);
}

void MultiLangString::clear() {
    translations.clear();
}