// QuotationHelper.h
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
 */
class QuotationHelper {
public:
    // Deleted constructors to prevent instantiation
    QuotationHelper() = delete;
    QuotationHelper(const QuotationHelper&) = delete;
    QuotationHelper& operator=(const QuotationHelper&) = delete;

    /*!
     * \brief Checks if the given string contains any quotation marks
     *
     * Searches for the presence of various types of quotation marks in the provided text.
     * The following quotation marks are supported:
     * - Backtick (`)
     * - Double prime (″)
     * - Gershayim (״)
     * - Arabic double quote (؞)
     * - Left/Right double angle quotes (««)
     * - Single left/right angle quotes (‹›)
     * - Double low quotation mark („)
     * - Single low quotation mark (‚)
     * - Left/Right double quotation marks (""")
     * - Left/Right single quotation marks (''')
     *
     * \param source The text string to check for quotation marks
     * \return true if any quotation marks are found, false otherwise
     *
     * \note The function returns false if the input string is empty
     *
     * Example usage:
     * \code{.cpp}
     * std::string text = "Hello "world"";
     * bool hasQuotes = QuotationHelper::find(text); // Returns true
     * \endcode
     */
    static bool find(const std::string& source);

private:
    /*!
     * \brief Regular expression pattern for matching quotation marks
     *
     * This regex pattern matches Unicode quotation marks including:
     * - ASCII quotes
     * - Unicode quotation marks (U+2033, U+05F4, U+059E, etc.)
     * - Directional quotes (opening and closing variants)
     * - Language-specific quotation marks
     */
    static const std::regex QUOTES;
};