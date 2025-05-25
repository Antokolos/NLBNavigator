#pragma once

#include <string>
#include <regex>

/*!
 * \brief Helper class for working with quotation marks in text
 *
 * This class provides utility functions for detecting various types of quotation marks
 * in text strings. It supports a wide range of Unicode quotation marks including
 * standard quotes, typographic quotes, and language-specific quotation marks.
 *
 * The class is designed to be used statically and cannot be instantiated.
 * 
 * Supported quotation marks include:
 * - Backtick (`)
 * - Double prime (″) - U+2033
 * - Gershayim (״) - U+05F4  
 * - Arabic double quote (؞) - U+059E
 * - Left/Right double angle quotes (« ») - U+00AB, U+00BB
 * - Single left/right angle quotes (‹ ›) - U+2039, U+203A
 * - Double low quotation mark („) - U+201E
 * - Single low quotation mark (‚) - U+201A
 * - Left/Right double quotation marks (" " ") - U+201C, U+201D, U+201F
 * - Left/Right single quotation marks (' ' ') - U+2018, U+2019, U+201B
 */
class NLBCORE_API QuotationHelper {
public:
    // Deleted constructors to prevent instantiation
    QuotationHelper() = delete;
    QuotationHelper(const QuotationHelper&) = delete;
    QuotationHelper& operator=(const QuotationHelper&) = delete;
    ~QuotationHelper() = delete;

    /*!
     * \brief Checks if the given string contains any quotation marks
     *
     * Searches for the presence of various types of quotation marks in the provided text.
     * The method handles null and empty strings gracefully.
     *
     * \param source The text string to check for quotation marks
     * \return true if any quotation marks are found, false otherwise
     *
     * \note The function returns false if the input string is null or empty
     *
     * Example usage:
     * \code{.cpp}
     * std::string text = "Hello "world"";
     * bool hasQuotes = QuotationHelper::find(text); // Returns true
     * 
     * std::string emptyText = "";
     * bool hasQuotesEmpty = QuotationHelper::find(emptyText); // Returns false
     * \endcode
     */
    static bool find(const std::string& source);

    /*!
     * \brief Checks if the given string contains any quotation marks (overload for C-style strings)
     *
     * \param source The C-style string to check for quotation marks
     * \return true if any quotation marks are found, false otherwise
     *
     * \note The function returns false if the input string is null or empty
     */
    static bool find(const char* source);

private:
    /*!
     * \brief Regular expression pattern for matching quotation marks
     *
     * This regex pattern matches Unicode quotation marks including:
     * - ASCII backtick (`)
     * - Unicode quotation marks (U+2033, U+05F4, U+059E, etc.)
     * - Directional quotes (opening and closing variants)
     * - Language-specific quotation marks
     * 
     * The pattern is compiled once during static initialization for performance.
     */
    static const std::regex QUOTES;

    /*!
     * \brief Initialize the static regex pattern
     * This method is called during static initialization to set up the regex pattern
     */
    static std::regex initializeQuotesPattern();
};