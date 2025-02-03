#include "nlb/api/LinksTableModel.h"

LinksTableModel::LinksTableModel(const std::vector<std::shared_ptr<Link>>& links) {
    for (const auto& link : links) {
        if (!link->isDeleted()) {
            m_links.push_back(link);
        }
    }
}

std::string LinksTableModel::getColumnName(int column) const {
    switch (column) {
        case 0:
            return "Link Id";
        case 1:
            return "Link text";
        default:
            return "N/A";
    }
}

int LinksTableModel::getRowCount() const {
    return static_cast<int>(m_links.size());
}

int LinksTableModel::getColumnCount() const {
    return 2;
}

std::string LinksTableModel::getValueAt(int rowIndex, int columnIndex) const {
    const auto& link = getLinkAt(rowIndex);
    switch (columnIndex) {
        case 0:
            return link->getId();
        case 1:
            return link->getText();
        default:
            return "N/A";
    }
}

std::shared_ptr<Link> LinksTableModel::getLinkAt(int rowIndex) const {
    return m_links[rowIndex];
}

void LinksTableModel::deleteLinkAt(int rowIndex) {
    if (rowIndex >= 0 && rowIndex < static_cast<int>(m_links.size())) {
        m_links.erase(m_links.begin() + rowIndex);
    }
}

const std::vector<std::shared_ptr<Link>>& LinksTableModel::getLinks() const {
    return m_links;
}

void LinksTableModel::moveUp(int rowIndex) {
    if (rowIndex > 0 && rowIndex < static_cast<int>(m_links.size())) {
        auto item = m_links[rowIndex - 1];
        m_links.erase(m_links.begin() + rowIndex - 1);
        m_links.insert(m_links.begin() + rowIndex, item);
    }
}

void LinksTableModel::moveDown(int rowIndex) {
    if (rowIndex >= 0 && rowIndex < static_cast<int>(m_links.size()) - 1) {
        auto item = m_links[rowIndex + 1];
        m_links.erase(m_links.begin() + rowIndex + 1);
        m_links.insert(m_links.begin() + rowIndex, item);
    }
}