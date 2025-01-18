#include "nlb/api/NonLinearBook.h"

// String constants
const std::string NonLinearBook::TRUE_VARID = "TRUE";
const std::string NonLinearBook::FALSE_VARID = "FALSE";
const std::string NonLinearBook::LC_VARID_PREFIX = "LC_";
const std::string NonLinearBook::LC_VARID_SEPARATOR_OUT = "_OUT_";
const std::string NonLinearBook::SOUND_DIR_NAME = "sound";
const std::string NonLinearBook::IMAGES_DIR_NAME = "images";
const std::string NonLinearBook::DEFAULT_STARTPOINT = "";
const Theme NonLinearBook::DEFAULT_THEME = Theme::DEFAULT;
const std::string NonLinearBook::DEFAULT_LANGUAGE = "ru";
const std::string NonLinearBook::DEFAULT_LICENSE = "";
const std::string NonLinearBook::DEFAULT_TITLE = "";
const std::string NonLinearBook::DEFAULT_AUTHOR = "";
const std::string NonLinearBook::DEFAULT_VERSION = "";
const std::string NonLinearBook::DEFAULT_PERFECT_GAME_ACHIEVEMENT_NAME = "";

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