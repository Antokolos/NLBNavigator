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
    std::vector<MediaFile*> m_imageFiles;
    std::vector<MediaFile*> m_soundFiles;
    
public:
    // Prevent copying and assignment
    DummyNLB(const DummyNLB&) = delete;
    DummyNLB& operator=(const DummyNLB&) = delete;

    static NonLinearBook* singleton() {
        static NonLinearBook* instance(new DummyNLB());
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
    const std::vector<MediaFile*>& getImageFiles() const override;
    const std::vector<MediaFile*>& getSoundFiles() const override;
    
    void exportMedia(
        bool isRoot,
        const std::string& mainExportDir,
        const std::string& mediaDirName,
        const std::vector<MediaFile*>& mediaFiles,
        MediaFile::Type mediaType
    ) const override;

    std::map<std::string, Page*> getPages() const override;
    std::map<std::string, Page*> getDownwardPagesHeirarchy() const override;
    std::map<std::string, Page*> getUpwardPagesHeirarchy() const override;
    std::vector<std::string> getAutowiredPagesIds() const override;
    std::vector<std::string> getParentGlobalAutowiredPagesIds() const override;
    bool isAutowired(const std::string& pageId) const override;
    Page* getPageById(const std::string& id) const override;

    std::map<std::string, Obj*> getObjs() const override;
    Obj* getObjById(const std::string& objId) const override;
    
    Page* createFilteredPage(
        const std::string& sourceId,
        const History& history
    ) const override;

    bool load(
        const std::string& path,
        const ProgressData& progressData
    ) override;

    Variable* getVariableById(const std::string& varId) const override;
    std::vector<Variable*> getVariables() const override;
    
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
    NonLinearBook* getParentNLB() const override;
    bool isDummy() const override;
    Page* getParentPage() const override;
    
    std::map<std::string, NonLinearBook*> getExternalModules() const override;
    NonLinearBook* findExternalModule(const std::string& name) const override;
    std::map<std::string, Variable::DataType> getVariableDataTypes() const override;
    
    std::map<std::string, std::string> getMediaToConstraintMap() const override;
    std::map<std::string, std::string> getMediaRedirectsMap() const override;
    std::map<std::string, MediaExportParameters> getMediaExportParametersMap() const override;
    std::map<std::string, bool> getMediaFlagsMap() const override;
};