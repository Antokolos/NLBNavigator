#ifndef MULTILANGSTRING_H
#define MULTILANGSTRING_H

#include <string>
#include <unordered_map>

class NLBCORE_API MultiLangString {
private:
    // Map of languages and translations
    std::unordered_map<std::string, std::string> translations;

public:
    // Default constructor
    MultiLangString() = default;
    
    // Get translation for specified language
    std::string getTranslation(const std::string& language) const;
    
    // Set translation for specified language
    void setTranslation(const std::string& language, const std::string& text);
    
    // Check if translation exists
    bool hasTranslation(const std::string& language) const;
    
    // Remove translation for specified language
    void removeTranslation(const std::string& language);
    
    // Clear all translations
    void clear();

    inline static MultiLangString createDefaultLinkText() {
        return MultiLangString();
    };
    
    inline static MultiLangString createEmptyText() {
        return MultiLangString();
    };
};

#endif // MULTILANGSTRING_H