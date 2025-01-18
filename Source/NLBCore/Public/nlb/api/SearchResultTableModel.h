#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "nlb/domain/SearchResult.h"

/*!
 * @brief Model class for displaying search results in a table format
 */
class NLBCORE_API SearchResultTableModel {
public:
    /*!
     * @brief Default constructor
     * Creates model with single "Information" column
     */
    SearchResultTableModel();

    /*!
     * @brief Constructor with column names
     * @param columnNames List of column names
     */
    explicit SearchResultTableModel(const std::vector<std::string>& columnNames);

    /*!
     * @brief Constructor with initializer list of column names
     * @param columnNames Initializer list of column names
     */
    SearchResultTableModel(std::initializer_list<std::string> columnNames);

    /*!
     * @brief Add single search result to the model
     * @param searchResult Search result to add
     */
    void addSearchResult(const SearchResult& searchResult);

    /*!
     * @brief Add multiple search results to the model
     * @param searchResults Vector of search results to add
     */
    void addSearchResults(const std::vector<SearchResult>& searchResults);

    /*!
     * @brief Merge another table model into this one
     * @param searchResultTableModel Model to merge, must have same column count
     */
    void addSearchResultTableModel(const SearchResultTableModel& searchResultTableModel);

    /*!
     * @brief Get total number of rows
     * @return Row count
     */
    size_t getRowCount() const;

    /*!
     * @brief Get total number of columns
     * @return Column count
     */
    size_t getColumnCount() const;

    /*!
     * @brief Get value at specific position
     * @param rowIndex Row index
     * @param columnIndex Column index
     * @return Value at specified position
     */
    std::string getValueAt(size_t rowIndex, size_t columnIndex) const;

    /*!
     * @brief Get column name
     * @param column Column index
     * @return Column name
     */
    std::string getColumnName(size_t column) const;

private:
    static const std::string NA;
    std::map<std::string, std::vector<SearchResult>> m_searchResultMap;
    std::vector<std::string> m_columnNames;
    size_t m_columnCount;
};