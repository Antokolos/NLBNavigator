#include "nlb/domain/SearchResult.h"
#include "nlb/api/Constants.h"

SearchResult::SearchResult() : m_modulePageId(nlb::Constants::EMPTY_STRING) {
}

SearchResult::SearchResult(
    const std::string& id,
    const std::string& modulePageId,
    const std::string& information
) : m_id(id), m_modulePageId(modulePageId) {
    m_information.push_back(information);
}

std::string SearchResult::getId() const {
    return m_id;
}

void SearchResult::setId(const std::string& id) {
    m_id = id;
}

std::string SearchResult::getModulePageId() const {
    return m_modulePageId;
}

void SearchResult::setModulePageId(const std::string& modulePageId) {
    m_modulePageId = modulePageId;
}

void SearchResult::setInformation(const std::string& information) {
    m_information.clear();
    addInformation(information);
}

void SearchResult::addInformation(const std::string& information) {
    m_information.push_back(information);
}

std::string SearchResult::getInformationByPosition(size_t i) const {
    return m_information.at(i);
}

const std::vector<std::string>& SearchResult::getInformation() const {
    return m_information;
}
