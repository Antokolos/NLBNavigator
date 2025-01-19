#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

/*!
 * @class MultiLangString
 * @brief Class for handling multi-language string content
 */
class NLBCORE_API MultiLangString {
private:
    std::map<std::string, std::string> m_content;

public:
    /*!
     * @brief Default constructor
     */
    MultiLangString() = default;

    /*!
     * @brief Copy constructor
     * @param source Source object to copy from
     */
    MultiLangString(const MultiLangString& source);

    static MultiLangString createEmptyText();
    static MultiLangString createDefaultLinkText();
    static MultiLangString createDefaultTraverseText();
    static MultiLangString createCopy(const MultiLangString& source);

    std::set<std::string> keySet() const;
    std::vector<std::string> values() const;
    void put(const std::string& langKey, const std::string& value);
    std::string get(const std::string& langKey) const;
    bool equalsAs(const std::string& langKey, const MultiLangString& mlsToCompare) const;
    bool isSubsetOf(const MultiLangString& mlsToCompare) const;
    bool isEmpty() const;

    bool operator==(const MultiLangString& other) const;
    bool operator!=(const MultiLangString& other) const;

    std::map<std::string, std::string>& content() { return m_content; }
    const std::map<std::string, std::string>& content() const { return m_content; }
};

namespace std {
    template<>
    struct hash<MultiLangString> {
        size_t operator()(const MultiLangString& mls) const;
    };
}
