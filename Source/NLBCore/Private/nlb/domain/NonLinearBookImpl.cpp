#include "nlb/domain/NonLinearBookImpl.h"
#include "nlb/domain/MediaExportParameters.h"
#include "nlb/domain/PageImpl.h"
#include "nlb/domain/ObjImpl.h"
#include "nlb/util/FileManipulator.h"
#include "nlb/api/Constants.h"
#include "nlb/api/SearchResultTableModel.h"
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <algorithm>

// Constructor implementations
NonLinearBookImpl::NonLinearBookImpl()
    : m_parentNLB(nullptr), m_parentPage(nullptr),
      m_fullAutowire(NonLinearBook::DEFAULT_FULL_AUTOWIRE),
      m_suppressMedia(NonLinearBook::DEFAULT_SUPPRESS_MEDIA),
      m_suppressSound(NonLinearBook::DEFAULT_SUPPRESS_SOUND) {}

NonLinearBookImpl::NonLinearBookImpl(std::shared_ptr<NonLinearBook> parentNLB, std::shared_ptr<Page> parentPage)
    : m_parentNLB(std::move(parentNLB)), m_parentPage(std::move(parentPage)),
      m_fullAutowire(parentNLB->isFullAutowire()),
      m_suppressMedia(parentNLB->isSuppressMedia()),
      m_suppressSound(parentNLB->isSuppressSound()) {}

// Method implementations
std::set<std::string> NonLinearBookImpl::getAllAchievementNames(bool recursive) const {
    std::set<std::string> result;
    for (const auto& [key, page] : m_pages) {
        // Collect achievements from pages
        // Logic translated from Java
        for (const auto& link : page->getLinks()) {
            // Collect achievements from links
        }
    }
    return result;
}

std::string NonLinearBookImpl::getPerfectGameAchievementName() const {
    return m_perfectGameAchievementName;
}

bool NonLinearBookImpl::isEmpty() const {
    return m_pages.empty() && m_objs.empty() && m_variables.empty();
}

std::string NonLinearBookImpl::getStartPoint() const {
    return m_startPoint;
}

std::string NonLinearBookImpl::getLanguage() const {
    return m_language;
}

std::string NonLinearBookImpl::getLicense() const {
    return m_license;
}

Theme NonLinearBookImpl::getTheme() const {
    return m_theme;
}

bool NonLinearBookImpl::isFullAutowire() const {
    return m_fullAutowire;
}

bool NonLinearBookImpl::isSuppressMedia() const {
    return m_suppressMedia;
}

bool NonLinearBookImpl::isSuppressSound() const {
    return m_suppressSound;
}

std::string NonLinearBookImpl::getTitle() const {
    return m_title;
}

std::string NonLinearBookImpl::getAuthor() const {
    return m_author;
}

std::string NonLinearBookImpl::getVersion() const {
    return m_version;
}

std::string NonLinearBookImpl::getRootDir() const {
    // Implementation based on root directory handling
    return {}; // Placeholder
}

std::string NonLinearBookImpl::getImagesDir() const {
    // Implementation for image directory retrieval
    return {}; // Placeholder
}

std::set<std::string> NonLinearBookImpl::getUsedImages() const {
    std::set<std::string> result;
    // Logic for extracting used images
    return result;
}

std::set<std::string> NonLinearBookImpl::getUsedSounds() const {
    std::set<std::string> result;
    // Logic for extracting used sounds
    return result;
}

const std::vector<MediaFile>& NonLinearBookImpl::getImageFiles() const {
    return m_imageFiles;  //std::vector<MediaFile>(m_imageFiles.begin(), m_imageFiles.end());
}

const std::vector<MediaFile>& NonLinearBookImpl::getSoundFiles() const {
    return m_soundFiles;  //std::vector<MediaFile>(m_soundFiles.begin(), m_soundFiles.end());
}

std::map<std::string, std::shared_ptr<Page>> NonLinearBookImpl::getPages() const {
    return m_pages;
}

std::map<std::string, std::shared_ptr<Page>> NonLinearBookImpl::getDownwardPagesHeirarchy() const {
    return {}; // Recursive logic here
}

std::map<std::string, std::shared_ptr<Page>> NonLinearBookImpl::getUpwardPagesHeirarchy() const {
    return {}; // Recursive logic here
}

std::vector<std::string> NonLinearBookImpl::getAutowiredPagesIds() const {
    return m_autowiredPages;
}

std::vector<std::string> NonLinearBookImpl::getParentGlobalAutowiredPagesIds() const {
    return {}; // Logic here
}

bool NonLinearBookImpl::isAutowired(const std::string& pageId) const {
    return std::find(m_autowiredPages.begin(), m_autowiredPages.end(), pageId) != m_autowiredPages.end();
}

std::shared_ptr<Page> NonLinearBookImpl::getPageById(const std::string& id) const {
    auto it = m_pages.find(id);
    return (it != m_pages.end()) ? it->second : nullptr;
}

void NonLinearBookImpl::clear() {
    m_pages.clear();
    m_objs.clear();
    m_variables.clear();
    m_imageFiles.clear();
    m_soundFiles.clear();
}

bool NonLinearBookImpl::load(const std::string& path, const ProgressData& progressData) {
    // Full logic to load NonLinearBook from directory
    return true;
}

bool NonLinearBookImpl::loadAndSetParent(const std::string& path, std::shared_ptr<NonLinearBook> parentNLB, std::shared_ptr<Page> parentPage) {
    // Logic to load NonLinearBook and set parent
    return true;
}

// Add logic for private helper methods as per Java code.
void NonLinearBookImpl::exportMedia(bool recursively, const std::string& mediaDir,
                                  const std::string& exportDir,
                                  const std::vector<MediaFile>& mediaFiles,
                                  MediaFile::Type type) const {
    // TODO: Implement media export logic
}

std::shared_ptr<Page> NonLinearBookImpl::createFilteredPage(const std::string& pageId,
                                                          const History& history) const {
    // TODO: Implement filtered page creation
    return nullptr;
}

SearchResultTableModel NonLinearBookImpl::getVariables(const std::string& filter) const {
    return SearchResultTableModel(); // Empty model for now
}

SearchResultTableModel NonLinearBookImpl::getLeafs(const std::string& filter) const {
    return SearchResultTableModel(); // Empty model for now
}

SearchResultTableModel NonLinearBookImpl::searchText(const SearchContract& contract,
                                                   const std::string& filter) const {
    return SearchResultTableModel(); // Empty model for now
}

bool NonLinearBookImpl::findVariable(const std::string& variableName) const {
    return false;
}

SearchResultTableModel NonLinearBookImpl::checkBook(const std::string& filter) const {
    return SearchResultTableModel(); // Empty model for now
}

NonLinearBook::BookStatistics NonLinearBookImpl::getBookStatistics() const {
    return BookStatistics();
}

NonLinearBook::VariableStatistics NonLinearBookImpl::getVariableStatistics() const {
    return VariableStatistics();
}

std::shared_ptr<NonLinearBook> NonLinearBookImpl::getParentNLB() const {
    return m_parentNLB;
}

bool NonLinearBookImpl::isDummy() const {
    return false;
}

std::shared_ptr<Page> NonLinearBookImpl::getParentPage() const {
    return m_parentPage;
}

std::map<std::string, std::shared_ptr<NonLinearBook>> NonLinearBookImpl::getExternalModules() const {
    return std::map<std::string, std::shared_ptr<NonLinearBook>>();
}

std::shared_ptr<NonLinearBook> NonLinearBookImpl::findExternalModule(const std::string& moduleId) const {
    return nullptr;
}

std::map<std::string, Variable::DataType> NonLinearBookImpl::getVariableDataTypes() const {
    return std::map<std::string, Variable::DataType>();
}

std::map<std::string, std::string> NonLinearBookImpl::getMediaToConstraintMap() const {
    return std::map<std::string, std::string>();
}

std::map<std::string, std::string> NonLinearBookImpl::getMediaRedirectsMap() const {
    return std::map<std::string, std::string>();
}

std::map<std::string, MediaExportParameters> NonLinearBookImpl::getMediaExportParametersMap() const {
    return std::map<std::string, MediaExportParameters>();
}

std::map<std::string, bool> NonLinearBookImpl::getMediaFlagsMap() const {
    return std::map<std::string, bool>();
}

std::shared_ptr<PageImpl> NonLinearBookImpl::getPageImplById(const std::string& id) const
{ return std::static_pointer_cast<PageImpl>(m_pages.at(id)); }

std::shared_ptr<ObjImpl> NonLinearBookImpl::getObjImplById(const std::string& id) const
{ return std::static_pointer_cast<ObjImpl>(m_objs.at(id)); }
