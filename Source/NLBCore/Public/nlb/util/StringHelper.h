#pragma once

#include <string>
#include <vector>
#include <map>
#include <regex>
#include <memory>

class MultiLangString;
class TextChunk;

/*!
 * @class StringHelper
 * @brief Utility class for string operations and text processing
 */
class NLBCORE_API StringHelper {
private:
    static const std::regex LINE_PATTERN;
    static const std::regex VAR_PATTERN;
    static const std::string DELIMITER;

public:
    /*!
     * @brief Replace variables in text with their values
     * @param pageText Text containing variables in format $varname$
     * @param visitedVars Map of variable names to their values
     * @return Text with replaced variables
     */
    //static std::string replaceVariables(const std::string& pageText, const std::map<std::string, std::any>& visitedVars);

    /*!
     * @brief Split text into chunks (text, variables, newlines)
     * @param text Input text
     * @return Vector of text chunks
     */
    static std::vector<TextChunk> getTextChunks(const std::string& text);

    /*!
     * @brief Check if string is null or empty
     * @param str String to check
     * @return true if string is empty
     */
    static bool isEmpty(const std::string& str);

    /*!
     * @brief Check if string is not empty
     * @param str String to check
     * @return true if string is not empty
     */
    static bool notEmpty(const std::string& str);

    /*!
     * @brief Check if MultiLangString is empty in all languages
     * @param multiLangString MultiLangString to check
     * @return true if all strings are empty
     */
    static bool isEmpty(const MultiLangString& multiLangString);

    /*!
     * @brief Format sequence of strings with delimiter
     * @param strings Vector of strings to format
     * @return Formatted string
     */
    static std::string formatSequence(const std::vector<std::string>& strings);

    /*!
     * @brief Split string into items by delimiter
     * @param sequenceString String to split
     * @return Vector of split items
     */
    static std::vector<std::string> getItems(const std::string& sequenceString);

    /*!
     * @brief Create string by repeating a fill string
     * @param length Target length
     * @param fill String to repeat
     * @return Resulting string
     */
    static std::string createRepeatedString(size_t length, const std::string& fill);
};
