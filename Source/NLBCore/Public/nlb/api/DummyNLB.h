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
    std::vector<std::shared_ptr<MediaFile>> m_imageFiles;
    std::vector<std::shared_ptr<MediaFile>> m_soundFiles;
    
public:
    // Prevent copying and assignment
    DummyNLB(const DummyNLB&) = delete;
    DummyNLB& operator=(const DummyNLB&) = delete;

    static std::shared_ptr<NonLinearBook> singleton() {
        static std::shared_ptr<NonLinearBook> instance(new DummyNLB());
        return instance;
    }

    std::set<std::string> getAllAchievementNames(bool recursive) const override;
    std::string getPerfectGameAchievementName() const override;
    bool isEmpty() const override;
    std::string getStartPoint() const override;
    std::string getLanguage() const override;
    std::string getLicense() const override;
    Theme getTheme() const override;
    bool isFullAutowire() const override;
    bool isSuppressMedia() const override;
    bool isSuppressSound() const override;
    std::string getTitle() const override;
    std::string getAuthor() const override;
    std::string getVersion() const override;
    
    std::string getRootDir() const override;
    std::string getImagesDir() const override;
    std::set<std::string> getUsedImages() const override;
    std::set<std::string> getUsedSounds() const override;
    const std::vector<std::shared_ptr<MediaFile>>& getImageFiles() const override;
    const std::vector<std::shared_ptr<MediaFile>>& getSoundFiles() const override;
    
    void exportMedia(
        bool isRoot,
        const std::string& mainExportDir,
        const std::string& mediaDirName,
        const std::vector<std::shared_ptr<MediaFile>>& mediaFiles,
        MediaFile::Type mediaType
    ) const override;

    std::map<std::string, std::shared_ptr<Page>> getPages() const override;
    std::map<std::string, std::shared_ptr<Page>> getDownwardPagesHeirarchy() const override;
    std::map<std::string, std::shared_ptr<Page>> getUpwardPagesHeirarchy() const override;
    std::vector<std::string> getAutowiredPagesIds() const override;
    std::vector<std::string> getParentGlobalAutowiredPagesIds() const override;
    bool isAutowired(const std::string& pageId) const override;
    std::shared_ptr<Page> getPageById(const std::string& id) const override;

    std::map<std::string, std::shared_ptr<Obj>> getObjs() const override;
    std::shared_ptr<Obj> getObjById(const std::string& objId) const override;
    
    std::shared_ptr<Page> createFilteredPage(
        const std::string& sourceId,
        const History& history
    ) const override;

    bool load(
        const std::string& path,
        const ProgressData& progressData
    ) override;

    std::shared_ptr<Variable> getVariableById(const std::string& varId) const override;
    std::vector<std::shared_ptr<Variable>> getVariables() const override;
    
    SearchResultTableModel getLeafs(const std::string& modulePageId) const override;
    SearchResultTableModel searchText(
        const SearchContract& searchContract,
        const std::string& modulePageId
    ) const override;
    SearchResultTableModel getVariables(const std::string& modulePageId) const override;
    bool findVariable(const std::string& variableNameToFind) const override;
    SearchResultTableModel checkBook(const std::string& modulePageId) const override;
    
    BookStatistics getBookStatistics() const override;
    VariableStatistics getVariableStatistics() const override;
    std::shared_ptr<NonLinearBook> getParentNLB() const override;
    bool isDummy() const override;
    std::shared_ptr<Page> getParentPage() const override;
    
    std::map<std::string, std::shared_ptr<NonLinearBook>> getExternalModules() const override;
    std::shared_ptr<NonLinearBook> findExternalModule(const std::string& name) const override;
    std::map<std::string, Variable::DataType> getVariableDataTypes() const override;
    
    std::map<std::string, std::string> getMediaToConstraintMap() const override;
    std::map<std::string, std::string> getMediaRedirectsMap() const override;
    std::map<std::string, MediaExportParameters> getMediaExportParametersMap() const override;
    std::map<std::string, bool> getMediaFlagsMap() const override;
};