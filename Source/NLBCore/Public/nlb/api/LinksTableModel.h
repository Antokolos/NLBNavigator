#pragma once

#include <vector>
#include <string>
#include <memory>
#include "Link.h"

/*!
 * @brief Model for displaying links in a table format
 */
class LinksTableModel {
public:
    /*!
     * @brief Constructs a new table model from a vector of links
     * @param links Vector of links to populate the model
     */
    explicit LinksTableModel(const std::vector<std::shared_ptr<Link>>& links);

    /*!
     * @brief Gets the name of the specified column
     * @param column Column index
     * @return Column name string
     */
    std::string getColumnName(int column) const;

    /*!
     * @brief Gets the number of rows in the model
     * @return Number of rows
     */
    int getRowCount() const;

    /*!
     * @brief Gets the number of columns in the model
     * @return Number of columns
     */
    int getColumnCount() const;

    /*!
     * @brief Gets the value at specified position
     * @param rowIndex Row index
     * @param columnIndex Column index
     * @return Value at the specified position
     */
    std::string getValueAt(int rowIndex, int columnIndex) const;

    /*!
     * @brief Gets the link at specified row
     * @param rowIndex Row index
     * @return Shared pointer to the link
     */
    std::shared_ptr<Link> getLinkAt(int rowIndex) const;

    /*!
     * @brief Deletes the link at specified row
     * @param rowIndex Row index
     */
    void deleteLinkAt(int rowIndex);

    /*!
     * @brief Gets all links in the model
     * @return Vector of links
     */
    const std::vector<std::shared_ptr<Link>>& getLinks() const;

    /*!
     * @brief Moves the link at specified row up
     * @param rowIndex Row index
     */
    void moveUp(int rowIndex);

    /*!
     * @brief Moves the link at specified row down
     * @param rowIndex Row index
     */
    void moveDown(int rowIndex);

private:
    std::vector<std::shared_ptr<Link>> m_links;
};