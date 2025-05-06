#include "nlb/api/DummyNLB.h"
#include "nlb/api/SearchResultTableModel.h"
#include "nlb/domain/MediaExportParameters.h"

#include <stdexcept>

std::set<std::string> DummyNLB::getAllAchievementNames(bool recursive) const {
    return std::set<std::string>();
}

std::string DummyNLB::getPerfectGameAchievementName() const {
    return DEFAULT_PERFECT_GAME_ACHIEVEMENT_NAME;
}

bool DummyNLB::isEmpty() const {
    return true;
}

std::string DummyNLB::getStartPoint() const {
    return DEFAULT_STARTPOINT;
}

std::string DummyNLB::getLanguage() const {
    return DEFAULT_LANGUAGE;
}

std::string DummyNLB::getLicense() const {
    return DEFAULT_LICENSE;
}

Theme DummyNLB::getTheme() const {
    return Theme::DEFAULT;
}

bool DummyNLB::isFullAutowire() const {
    return DEFAULT_FULL_AUTOWIRE;
}

bool DummyNLB::isSuppressMedia() const {
    return DEFAULT_SUPPRESS_MEDIA;
}

bool DummyNLB::isSuppressSound() const {
    return DEFAULT_SUPPRESS_SOUND;
}

std::string DummyNLB::getTitle() const {
    return DEFAULT_TITLE;
}

std::string DummyNLB::getAuthor() const {
    return DEFAULT_AUTHOR;
}

std::string DummyNLB::getVersion() const {
    return DEFAULT_VERSION;
}

std::string DummyNLB::getRootDir() const {
    return std::string();
}

std::string DummyNLB::getImagesDir() const {
    return std::string();
}

std::set<std::string> DummyNLB::getUsedImages() const {
    return std::set<std::string>();
}

std::set<std::string> DummyNLB::getUsedSounds() const {
    return std::set<std::string>();
}

const std::vector<MediaFile>& DummyNLB::getImageFiles() const {
    return m_imageFiles; //std::vector<MediaFile>();
}

const std::vector<MediaFile>& DummyNLB::getSoundFiles() const {
    return m_soundFiles; //std::vector<MediaFile>();
}

void DummyNLB::exportMedia(
    bool isRoot,
    const std::string& mainExportDir,
    const std::string& mediaDirName,
    const std::vector<MediaFile>& mediaFiles,
    MediaFile::Type mediaType
) const {
    // do nothing
}

std::map<std::string, std::shared_ptr<Page>> DummyNLB::getPages() const {
    return std::map<std::string, std::shared_ptr<Page>>();
}

std::map<std::string, std::shared_ptr<Page>> DummyNLB::getDownwardPagesHeirarchy() const {
    return std::map<std::string, std::shared_ptr<Page>>();
}

std::map<std::string, std::shared_ptr<Page>> DummyNLB::getUpwardPagesHeirarchy() const {
    return std::map<std::string, std::shared_ptr<Page>>();
}

std::vector<std::string> DummyNLB::getAutowiredPagesIds() const {
    return std::vector<std::string>();
}

std::vector<std::string> DummyNLB::getParentGlobalAutowiredPagesIds() const {
    return std::vector<std::string>();
}

bool DummyNLB::isAutowired(const std::string& pageId) const {
    return false;
}

std::shared_ptr<Page> DummyNLB::getPageById(const std::string& id) const {
    return nullptr;
}

std::map<std::string, std::shared_ptr<Obj>> DummyNLB::getObjs() const {
    return std::map<std::string, std::shared_ptr<Obj>>();
}

std::shared_ptr<Obj> DummyNLB::getObjById(const std::string& objId) const {
    return nullptr;
}

std::shared_ptr<Page> DummyNLB::createFilteredPage(
    const std::string& sourceId,
    const History& history
) const {
    throw std::runtime_error("This operation is unsupported!");
}

bool DummyNLB::load(
    const std::string& path,
    const ProgressData& progressData
) {
    throw std::runtime_error("This operation is unsupported!");
}

std::shared_ptr<Variable> DummyNLB::getVariableById(const std::string& varId) const {
    return nullptr;
}

std::vector<std::shared_ptr<Variable>> DummyNLB::getVariables() const {
    return std::vector<std::shared_ptr<Variable>>();
}

SearchResultTableModel DummyNLB::getLeafs(const std::string& modulePageId) const {
    return SearchResultTableModel();
}

SearchResultTableModel DummyNLB::searchText(
    const SearchContract& searchContract,
    const std::string& modulePageId
) const {
    return SearchResultTableModel();
}

SearchResultTableModel DummyNLB::getVariables(const std::string& modulePageId) const {
    return SearchResultTableModel();
}

bool DummyNLB::findVariable(const std::string& variableNameToFind) const {
    return false;
}

SearchResultTableModel DummyNLB::checkBook(const std::string& modulePageId) const {
    return SearchResultTableModel();
}

NonLinearBook::BookStatistics DummyNLB::getBookStatistics() const {
    return BookStatistics();
}

NonLinearBook::VariableStatistics DummyNLB::getVariableStatistics() const {
    return VariableStatistics();
}

std::shared_ptr<NonLinearBook> DummyNLB::getParentNLB() const {
    return nullptr;
}

bool DummyNLB::isDummy() const {
    return true;
}

std::shared_ptr<Page> DummyNLB::getParentPage() const {
    return nullptr;
}

std::map<std::string, std::shared_ptr<NonLinearBook>> DummyNLB::getExternalModules() const {
    return std::map<std::string, std::shared_ptr<NonLinearBook>>();
}

std::shared_ptr<NonLinearBook> DummyNLB::findExternalModule(const std::string& name) const {
    return nullptr;
}

std::map<std::string, Variable::DataType> DummyNLB::getVariableDataTypes() const {
    return std::map<std::string, Variable::DataType>();
}

std::map<std::string, std::string> DummyNLB::getMediaToConstraintMap() const {
    return std::map<std::string, std::string>();
}

std::map<std::string, std::string> DummyNLB::getMediaRedirectsMap() const {
    return std::map<std::string, std::string>();
}

std::map<std::string, MediaExportParameters> DummyNLB::getMediaExportParametersMap() const {
    return std::map<std::string, MediaExportParameters>();
}

std::map<std::string, bool> DummyNLB::getMediaFlagsMap() const {
    return std::map<std::string, bool>();
}