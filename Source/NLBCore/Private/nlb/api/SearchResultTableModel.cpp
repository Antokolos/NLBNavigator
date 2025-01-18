#include "nlb/api/SearchResultTableModel.h"

const std::string SearchResultTableModel::NA = "N/A";

SearchResultTableModel::SearchResultTableModel() 
    : m_columnCount(3) {
    m_columnNames.push_back("Information");
}

SearchResultTableModel::SearchResultTableModel(const std::vector<std::string>& columnNames)
    : m_columnNames(columnNames)
    , m_columnCount(columnNames.size() + 2) {
}

SearchResultTableModel::SearchResultTableModel(std::initializer_list<std::string> columnNames)
    : m_columnNames(columnNames)
    , m_columnCount(columnNames.size() + 2) {
}

void SearchResultTableModel::addSearchResult(const SearchResult& searchResult) {
    if (searchResult.getId().empty()) {
        return;
    }

    auto& searchResults = m_searchResultMap[searchResult.getId()];
    searchResults.push_back(searchResult);
}

void SearchResultTableModel::addSearchResults(const std::vector<SearchResult>& searchResults) {
    for (const auto& searchResult : searchResults) {
        addSearchResult(searchResult);
    }
}

void SearchResultTableModel::addSearchResultTableModel(const SearchResultTableModel& searchResultTableModel) {
    if (searchResultTableModel.m_columnCount != m_columnCount) {
        return;
    }

    // Merge maps
    for (const auto& [id, results] : searchResultTableModel.m_searchResultMap) {
        auto& targetResults = m_searchResultMap[id];
        targetResults.insert(targetResults.end(), results.begin(), results.end());
    }
}

size_t SearchResultTableModel::getRowCount() const {
    size_t result = 0;
    for (const auto& [id, results] : m_searchResultMap) {
        result += results.size();
    }
    return result;
}

size_t SearchResultTableModel::getColumnCount() const {
    return m_columnCount;
}

std::string SearchResultTableModel::getValueAt(size_t rowIndex, size_t columnIndex) const {
    size_t currentRow = 0;
    
    for (const auto& [id, results] : m_searchResultMap) {
        for (const auto& searchResult : results) {
            if (currentRow == rowIndex) {
                if (columnIndex == 0) {
                    return id;
                } else if (columnIndex == 1) {
                    return searchResult.getModulePageId();
                } else if (columnIndex >= m_columnCount) {
                    return NA;
                } else {
                    return searchResult.getInformationByPosition(columnIndex - 2);
                }
            }
            ++currentRow;
        }
    }
    
    return NA;
}

std::string SearchResultTableModel::getColumnName(size_t column) const {
    if (column == 0) {
        return "Id";
    } else if (column == 1) {
        return "Module Page Id";
    } else if (column >= m_columnCount) {
        return NA;
    } else {
        return m_columnNames[column - 2];
    }
}