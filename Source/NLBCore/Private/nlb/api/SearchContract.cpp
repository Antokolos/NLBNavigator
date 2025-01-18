#include "nlb/api/SearchContract.h"

SearchContract::SearchContract(
    const std::string& searchText,
    bool searchInIds,
    bool searchInPages,
    bool searchInObjects,
    bool searchInLinks,
    bool searchInVars,
    bool ignoreCase,
    bool wholeWords,
    bool findUnusualQuotes
) : m_searchText(searchText),
    m_searchInIds(searchInIds),
    m_searchInPages(searchInPages),
    m_searchInObjects(searchInObjects),
    m_searchInLinks(searchInLinks),
    m_searchInVars(searchInVars),
    m_ignoreCase(ignoreCase),
    m_wholeWords(wholeWords),
    m_findUnusualQuotes(findUnusualQuotes)
{
}

std::string SearchContract::getSearchText() const {
    return m_searchText;
}

bool SearchContract::isSearchInIds() const {
    return m_searchInIds;
}

bool SearchContract::isSearchInPages() const {
    return m_searchInPages;
}

bool SearchContract::isSearchInObjects() const {
    return m_searchInObjects;
}

bool SearchContract::isSearchInLinks() const {
    return m_searchInLinks;
}

bool SearchContract::isSearchInVars() const {
    return m_searchInVars;
}

bool SearchContract::isIgnoreCase() const {
    return m_ignoreCase;
}

bool SearchContract::isWholeWords() const {
    return m_wholeWords;
}

bool SearchContract::isFindUnusualQuotes() const {
    return m_findUnusualQuotes;
}