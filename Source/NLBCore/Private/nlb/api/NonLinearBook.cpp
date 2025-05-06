#include "nlb/api/NonLinearBook.h"

// Implementation of ModuleInfo::toString()
std::string NonLinearBook::ModuleInfo::toString() const {
    return m_moduleName + " [" + m_modulePageId + "]";
}

// Implementation of BookStatistics::addBookStatistics
void NonLinearBook::BookStatistics::addBookStatistics(const BookStatistics& bookStatistics) {
    m_pagesCount += bookStatistics.m_pagesCount;
    m_objsCount += bookStatistics.m_objsCount;
    m_uniqueEndings += bookStatistics.m_uniqueEndings;
    m_charactersCount += bookStatistics.m_charactersCount;
    
    for (const auto& moduleInfo : bookStatistics.m_moduleInfos) {
        m_moduleInfos.push_back(moduleInfo);
    }
}

// Implementation of BookStatistics::addDeletedModulesFromBookStatistics
void NonLinearBook::BookStatistics::addDeletedModulesFromBookStatistics(const BookStatistics& bookStatistics) {
    for (const auto& moduleInfo : bookStatistics.m_modulesToBeDeletedInfos) {
        m_modulesToBeDeletedInfos.push_back(moduleInfo);
    }
}