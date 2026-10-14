#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

/*!
 * @class MultiLangString
 * @brief Class for handling multi-language string content
 */
class MultiLangString {
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

    /*!
     * @brief Assignment operator
     * @param other Source object to copy from
     * @return Reference to this object
     */
    MultiLangString& operator=(const MultiLangString& other);

    /*!
     * @brief Create empty MultiLangString
     * @return Empty MultiLangString instance
     */
    static MultiLangString createEmptyText();

    /*!
     * @brief Create MultiLangString with default link text
     * @return MultiLangString with default link text for different languages
     */
    static MultiLangString createDefaultLinkText();

    /*!
     * @brief Create MultiLangString with default traverse text
     * @return MultiLangString with default traverse text for different languages
     */
    static MultiLangString createDefaultTraverseText();

    /*!
     * @brief Create copy of existing MultiLangString
     * @param source Source MultiLangString to copy
     * @return Copy of the source MultiLangString
     */
    static MultiLangString createCopy(const MultiLangString& source);

    /*!
     * @brief Get set of all language keys
     * @return Set of language keys
     */
    std::set<std::string> keySet() const;

    /*!
     * @brief Get vector of all values
     * @return Vector of all values
     */
    std::vector<std::string> values() const;

    /*!
     * @brief Put value for specified language key
     * @param langKey Language key (cannot be empty)
     * @param value Value to store (can be empty)
     */
    void put(const std::string& langKey, const std::string& value);

    /*!
     * @brief Get value for specified language key
     * @param langKey Language key
     * @return Value for the key or empty string if not found
     */
    std::string get(const std::string& langKey) const;

    /*!
     * @brief Check if values for specific language key are equal
     * @param langKey Language key to compare
     * @param mlsToCompare MultiLangString to compare with
     * @return true if values are equal for the specified language
     */
    bool equalsAs(const std::string& langKey, const MultiLangString& mlsToCompare) const;

    /*!
     * @brief Check if this MultiLangString is subset of another
     * @param mlsToCompare MultiLangString to compare with
     * @return true if this is subset of mlsToCompare
     */
    bool isSubsetOf(const MultiLangString& mlsToCompare) const;

    /*!
     * @brief Check if MultiLangString is empty (all values are empty)
     * @return true if empty
     */
    bool isEmpty() const;

    /*!
     * @brief Equality operator
     * @param other MultiLangString to compare with
     * @return true if equal
     */
    bool operator==(const MultiLangString& other) const;

    /*!
     * @brief Inequality operator
     * @param other MultiLangString to compare with
     * @return true if not equal
     */
    bool operator!=(const MultiLangString& other) const;

    /*!
     * @brief Get mutable reference to content map
     * @return Mutable reference to content map
     */
    std::map<std::string, std::string>& content() { return m_content; }

    /*!
     * @brief Get const reference to content map
     * @return Const reference to content map
     */
    const std::map<std::string, std::string>& content() const { return m_content; }

    /*!
     * @brief Get hash code for this MultiLangString
     * @return Hash value
     */
    std::size_t hashCode() const;
};

namespace std {
    template<>
    struct hash<MultiLangString> {
        size_t operator()(const MultiLangString& mls) const {
            return mls.hashCode();
        }
    };
}