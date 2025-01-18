#include <stdexcept>
#include "nlb/api/DummyNLB.h"
#include "nlb/api/SearchResultTableModel.h"
#include "nlb/domain/MediaExportParameters.h"

std::set<std::string> DummyNLB::getAllAchievementNames(bool recursive) {
    return std::set<std::string>();
}

std::string DummyNLB::getPerfectGameAchievementName() {
    return DEFAULT_PERFECT_GAME_ACHIEVEMENT_NAME;
}

bool DummyNLB::isEmpty() {
    return true;
}

std::string DummyNLB::getStartPoint() {
    return DEFAULT_STARTPOINT;
}

std::string DummyNLB::getLanguage() {
    return DEFAULT_LANGUAGE;
}

std::string DummyNLB::getLicense() {
    return DEFAULT_LICENSE;
}

Theme DummyNLB::getTheme() {
    return Theme::DEFAULT;
}

bool DummyNLB::isFullAutowire() {
    return DEFAULT_FULL_AUTOWIRE;
}

bool DummyNLB::isSuppressMedia() {
    return DEFAULT_SUPPRESS_MEDIA;
}

bool DummyNLB::isSuppressSound() {
    return DEFAULT_SUPPRESS_SOUND;
}

std::string DummyNLB::getTitle() {
    return DEFAULT_TITLE;
}

std::string DummyNLB::getAuthor() {
    return DEFAULT_AUTHOR;
}

std::string DummyNLB::getVersion() {
    return DEFAULT_VERSION;
}

std::string DummyNLB::getRootDir() {
    return std::string();
}

std::string DummyNLB::getImagesDir() {
    return std::string();
}

std::set<std::string> DummyNLB::getUsedImages() {
    return std::set<std::string>();
}

std::set<std::string> DummyNLB::getUsedSounds() {
    return std::set<std::string>();
}

std::vector<MediaFile> DummyNLB::getImageFiles() {
    return std::vector<MediaFile>();
}

std::vector<MediaFile> DummyNLB::getSoundFiles() {
    return std::vector<MediaFile>();
}

void DummyNLB::exportMedia(
    bool isRoot,
    const std::string& mainExportDir,
    const std::string& mediaDirName,
    const std::vector<MediaFile>& mediaFiles,
    MediaFile::Type mediaType
) {
    // do nothing
}

std::map<std::string, std::shared_ptr<Page>> DummyNLB::getPages() {
    return std::map<std::string, std::shared_ptr<Page>>();
}

std::map<std::string, std::shared_ptr<Page>> DummyNLB::getDownwardPagesHeirarchy() {
    return std::map<std::string, std::shared_ptr<Page>>();
}

std::map<std::string, std::shared_ptr<Page>> DummyNLB::getUpwardPagesHeirarchy() {
    return std::map<std::string, std::shared_ptr<Page>>();
}

std::vector<std::string> DummyNLB::getAutowiredPagesIds() {
    return std::vector<std::string>();
}

std::vector<std::string> DummyNLB::getParentGlobalAutowiredPagesIds() {
    return std::vector<std::string>();
}

bool DummyNLB::isAutowired(const std::string& pageId) {
    return false;
}

std::shared_ptr<Page> DummyNLB::getPageById(const std::string& id) {
    return nullptr;
}

std::map<std::string, std::shared_ptr<Obj>> DummyNLB::getObjs() {
    return std::map<std::string, std::shared_ptr<Obj>>();
}

std::shared_ptr<Obj> DummyNLB::getObjById(const std::string& objId) {
    return nullptr;
}

std::shared_ptr<Page> DummyNLB::createFilteredPage(
    const std::string& sourceId,
    const History& history
) {
    throw std::runtime_error("This operation is unsupported!");
}

bool DummyNLB::load(
    const std::string& path,
    const ProgressData& progressData
) {
    throw std::runtime_error("This operation is unsupported!");
}

std::shared_ptr<Variable> DummyNLB::getVariableById(const std::string& varId) {
    return nullptr;
}

std::vector<std::shared_ptr<Variable>> DummyNLB::getVariables() {
    return std::vector<std::shared_ptr<Variable>>();
}

SearchResultTableModel DummyNLB::getLeafs(const std::string& modulePageId) {
    return SearchResultTableModel();
}

SearchResultTableModel DummyNLB::searchText(
    const SearchContract& searchContract,
    const std::string& modulePageId
) {
    return SearchResultTableModel();
}

SearchResultTableModel DummyNLB::getVariables(const std::string& modulePageId) {
    return SearchResultTableModel();
}

bool DummyNLB::findVariable(const std::string& variableNameToFind) {
    return false;
}

SearchResultTableModel DummyNLB::checkBook(const std::string& modulePageId) {
    return SearchResultTableModel();
}

NonLinearBook::BookStatistics DummyNLB::getBookStatistics() {
    return BookStatistics();
}

NonLinearBook::VariableStatistics DummyNLB::getVariableStatistics() {
    return VariableStatistics();
}

std::shared_ptr<NonLinearBook> DummyNLB::getParentNLB() {
    return nullptr;
}

bool DummyNLB::isDummy() {
    return true;
}

std::shared_ptr<Page> DummyNLB::getParentPage() {
    return nullptr;
}

std::map<std::string, std::shared_ptr<NonLinearBook>> DummyNLB::getExternalModules() {
    return std::map<std::string, std::shared_ptr<NonLinearBook>>();
}

std::shared_ptr<NonLinearBook> DummyNLB::findExternalModule(const std::string& name) {
    return nullptr;
}

std::map<std::string, Variable::DataType> DummyNLB::getVariableDataTypes() {
    return std::map<std::string, Variable::DataType>();
}

std::map<std::string, std::string> DummyNLB::getMediaToConstraintMap() {
    return std::map<std::string, std::string>();
}

std::map<std::string, std::string> DummyNLB::getMediaRedirectsMap() {
    return std::map<std::string, std::string>();
}

std::map<std::string, MediaExportParameters> DummyNLB::getMediaExportParametersMap() {
    return std::map<std::string, MediaExportParameters>();
}

std::map<std::string, bool> DummyNLB::getMediaFlagsMap() {
    return std::map<std::string, bool>();
}