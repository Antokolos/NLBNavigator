#include "nlb/api/LinksTableModel.h"
//#include "nlb/api/Link.h"
//#include <stdexcept> // Для исключений
/*
// Конструктор, фильтрующий удаленные ссылки
LinksTableModel::LinksTableModel(const std::vector<Link>& links) {
    // TODO: compilation error
    // for (const auto& link : links) {
    //     if (!link.isDeleted()) {
    //         m_links.push_back(link);
    //     }
    // }
}

// Получение имени столбца
std::string LinksTableModel::getColumnName(int column) const {
    switch (column) {
        case 0: return "Link Id";
        case 1: return "Link text";
        default: return "N/A";
    }
}

// Количество строк
int LinksTableModel::getRowCount() const {
    return m_links.size();
}

// Количество столбцов
int LinksTableModel::getColumnCount() const {
    return 2;
}

// Получение значения в ячейке
std::string LinksTableModel::getValueAt(int rowIndex, int columnIndex) const {
    if (rowIndex < 0 || rowIndex >= m_links.size()) {
        throw std::out_of_range("Invalid row index");
    }

    const Link& link = m_links[rowIndex];
    switch (columnIndex) {
        case 0: return link.getId();
        case 1: return link.getText();
        default: return "N/A";
    }
}

// Получение ссылки по индексу строки
const Link &LinksTableModel::getLinkAt(int rowIndex) const {
    if (rowIndex < 0 || rowIndex >= m_links.size()) {
        throw std::out_of_range("Invalid row index");
    }
    return m_links[rowIndex];
}

// Удаление ссылки по индексу строки
void LinksTableModel::deleteLinkAt(int rowIndex) {
    if (rowIndex < 0 || rowIndex >= m_links.size()) {
        throw std::out_of_range("Invalid row index");
    }
    m_links.erase(m_links.begin() + rowIndex);
}

// Получение всех ссылок
std::vector<Link> LinksTableModel::getLinks() const {
    return m_links;
}

// Перемещение ссылки вверх
void LinksTableModel::moveUp(int rowIndex) {
    if (rowIndex > 0 && rowIndex < m_links.size()) {
        Link &tmp = m_links[rowIndex];
        m_links[rowIndex] = m_links[rowIndex - 1];
        m_links[rowIndex - 1] = tmp;
    }
}

// Перемещение ссылки вниз
void LinksTableModel::moveDown(int rowIndex) {
    if (rowIndex >= 0 && rowIndex < m_links.size() - 1) {
        Link &tmp = m_links[rowIndex];
        m_links[rowIndex] = m_links[rowIndex + 1];
        m_links[rowIndex + 1] = tmp;
    }
}
*/