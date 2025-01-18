#pragma once

#include <string>

/*!
 * @brief Class representing search criteria and settings
 *
 * This class encapsulates various search parameters and options
 * used to configure text search operations.
 */
class SearchContract {
private:
    std::string m_searchText;
    bool m_searchInIds;
    bool m_searchInPages;
    bool m_searchInObjects;
    bool m_searchInLinks;
    bool m_searchInVars;
    bool m_ignoreCase;
    bool m_wholeWords;
    bool m_findUnusualQuotes;

public:
    /*!
     * @brief Constructor with all search parameters
     * @param searchText Text to search for
     * @param searchInIds Whether to search in IDs
     * @param searchInPages Whether to search in pages
     * @param searchInObjects Whether to search in objects
     * @param searchInLinks Whether to search in links
     * @param searchInVars Whether to search in variables
     * @param ignoreCase Whether to ignore case when searching
     * @param wholeWords Whether to match whole words only
     * @param findUnusualQuotes Whether to search for unusual quotes
     */
    SearchContract(
        const std::string& searchText,
        bool searchInIds,
        bool searchInPages,
        bool searchInObjects,
        bool searchInLinks,
        bool searchInVars,
        bool ignoreCase,
        bool wholeWords,
        bool findUnusualQuotes
    );

    /*!
     * @brief Get the search text
     * @return The text to search for
     */
    std::string getSearchText() const;

    /*!
     * @brief Check if searching in IDs is enabled
     * @return true if searching in IDs
     */
    bool isSearchInIds() const;

    /*!
     * @brief Check if searching in pages is enabled
     * @return true if searching in pages
     */
    bool isSearchInPages() const;

    /*!
     * @brief Check if searching in objects is enabled
     * @return true if searching in objects
     */
    bool isSearchInObjects() const;

    /*!
     * @brief Check if searching in links is enabled
     * @return true if searching in links
     */
    bool isSearchInLinks() const;

    /*!
     * @brief Check if searching in variables is enabled
     * @return true if searching in variables
     */
    bool isSearchInVars() const;

    /*!
     * @brief Check if case-insensitive search is enabled
     * @return true if ignoring case
     */
    bool isIgnoreCase() const;

    /*!
     * @brief Check if whole word search is enabled
     * @return true if matching whole words only
     */
    bool isWholeWords() const;

    /*!
     * @brief Check if unusual quotes search is enabled
     * @return true if searching for unusual quotes
     */
    bool isFindUnusualQuotes() const;
};