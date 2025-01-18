#pragma once

#include <string>
#include <set>
#include <map>
#include <vector>
#include <memory>
#include "nlb/api/NonLinearBook.h"

/*!
 * @brief Dummy implementation of NonLinearBook interface
 * 
 * This class provides a null object pattern implementation of NonLinearBook
 * with default/empty responses for all interface methods.
 */
class DummyNLB : public NonLinearBook {
protected:
    // Protected constructor for singleton, to allow object creation via new
    DummyNLB() = default;
    
public:
    // Prevent copying and assignment
    DummyNLB(const DummyNLB&) = delete;
    DummyNLB& operator=(const DummyNLB&) = delete;

    static std::shared_ptr<NonLinearBook> singleton() {
        static std::shared_ptr<NonLinearBook> instance(new DummyNLB());
        return instance;
    }

    std::set<std::string> getAllAchievementNames(bool recursive) override;
    std::string getPerfectGameAchievementName() override;
    bool isEmpty() override;
    std::string getStartPoint() override;
    std::string getLanguage() override;
    std::string getLicense() override;
    Theme getTheme() override;
    bool isFullAutowire() override;
    bool isSuppressMedia() override;
    bool isSuppressSound() override;
    std::string getTitle() override;
    std::string getAuthor() override;
    std::string getVersion() override;
    
    std::string getRootDir() override;
    std::string getImagesDir() override;
    std::set<std::string> getUsedImages() override;
    std::set<std::string> getUsedSounds() override;
    std::vector<MediaFile> getImageFiles() override;
    std::vector<MediaFile> getSoundFiles() override;
    
    void exportMedia(
        bool isRoot,
        const std::string& mainExportDir,
        const std::string& mediaDirName,
        const std::vector<MediaFile>& mediaFiles,
        MediaFile::Type mediaType
    ) override;

    std::map<std::string, std::shared_ptr<Page>> getPages() override;
    std::map<std::string, std::shared_ptr<Page>> getDownwardPagesHeirarchy() override;
    std::map<std::string, std::shared_ptr<Page>> getUpwardPagesHeirarchy() override;
    std::vector<std::string> getAutowiredPagesIds() override;
    std::vector<std::string> getParentGlobalAutowiredPagesIds() override;
    bool isAutowired(const std::string& pageId) override;
    std::shared_ptr<Page> getPageById(const std::string& id) override;

    std::map<std::string, std::shared_ptr<Obj>> getObjs() override;
    std::shared_ptr<Obj> getObjById(const std::string& objId) override;
    
    std::shared_ptr<Page> createFilteredPage(
        const std::string& sourceId,
        const History& history
    ) override;

    bool load(
        const std::string& path,
        const ProgressData& progressData
    ) override;

    std::shared_ptr<Variable> getVariableById(const std::string& varId) override;
    std::vector<std::shared_ptr<Variable>> getVariables() override;
    
    SearchResultTableModel getLeafs(const std::string& modulePageId) override;
    SearchResultTableModel searchText(
        const SearchContract& searchContract,
        const std::string& modulePageId
    ) override;
    SearchResultTableModel getVariables(const std::string& modulePageId) override;
    bool findVariable(const std::string& variableNameToFind) override;
    SearchResultTableModel checkBook(const std::string& modulePageId) override;
    
    BookStatistics getBookStatistics() override;
    VariableStatistics getVariableStatistics() override;
    std::shared_ptr<NonLinearBook> getParentNLB() override;
    bool isDummy() override;
    std::shared_ptr<Page> getParentPage() override;
    
    std::map<std::string, std::shared_ptr<NonLinearBook>> getExternalModules() override;
    std::shared_ptr<NonLinearBook> findExternalModule(const std::string& name) override;
    std::map<std::string, Variable::DataType> getVariableDataTypes() override;
    
    std::map<std::string, std::string> getMediaToConstraintMap() override;
    std::map<std::string, std::string> getMediaRedirectsMap() override;
    std::map<std::string, MediaExportParameters> getMediaExportParametersMap() override;
    std::map<std::string, bool> getMediaFlagsMap() override;
};