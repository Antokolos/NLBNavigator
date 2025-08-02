#pragma once

#include <string>
#include <vector>
#include <map>
#include <regex>
#include <memory>
#include <any>

class MultiLangString;
class TextChunk;

/*!
 * @class StringHelper
 * @brief Utility class for string operations and text processing
 */
class StringHelper {
private:
    static const std::regex LINE_PATTERN;
    static const std::regex VAR_PATTERN;
    static const std::string DELIMITER;

public:
    static const std::string EOL_STRING;
    static const std::string ACTION_TEXT_DEF;

    /*!
     * @brief Replace variables in text with their values
     * @param pageText Text containing variables in format $varname$
     * @param visitedVars Map of variable names to their values
     * @return Text with replaced variables
     */
    static std::string replaceVariables(const std::string& pageText, 
                                       const std::map<std::string, std::any>& visitedVars);

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
     * @brief Check if MultiLangString is not empty in at least one language
     * @param multiLangString MultiLangString to check
     * @return true if at least one string is not empty
     */
    static bool notEmpty(const MultiLangString& multiLangString);

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

    /*!
     * @brief Tokenize string by delimiter (similar to Java StringTokenizer)
     * @param str String to tokenize
     * @param delimiters String containing delimiter characters
     * @return Vector of tokens
     */
    static std::vector<std::string> tokenize(const std::string& str, const std::string& delimiters);

    /*!
     * @brief Convert any value to string representation
     * @param value Value to convert
     * @return String representation
     */
    static std::string anyToString(const std::any& value);

	/*!
	 * @brief Convert string to lowercase
	 * @param str Input string
	 * @return Lowercase string
	 */
	static std::string toLowerCase(const std::string& str);
};