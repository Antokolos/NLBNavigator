#pragma once

#include "nlb/api/NonLinearBook.h"
#include "nlb/api/Theme.h"
#include "nlb/api/MediaFile.h"
#include "nlb/api/Page.h"
#include "nlb/api/Obj.h"
#include "nlb/api/Variable.h"
#include "nlb/api/SearchResultTableModel.h"
#include "nlb/domain/MediaExportParameters.h"
#include <map>
#include <vector>
#include <set>
#include <memory>
#include <string>
#include <regex>

class PageImpl;
class ObjImpl;
class FileManipulator;

class NonLinearBookImpl : public NonLinearBook {
    public:
void append(const std::shared_ptr<NonLinearBook>& source, bool generateNewIds, bool overwriteTheme) {}


    NonLinearBookImpl();
    NonLinearBookImpl(std::shared_ptr<NonLinearBook> parentNLB, std::shared_ptr<Page> parentPage);

    // Override methods from NonLinearBook
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

    const std::vector<MediaFile>& getImageFiles() const override;
    const std::vector<MediaFile>& getSoundFiles() const override;

    std::map<std::string, std::shared_ptr<Page>> getPages() const override;
    std::map<std::string, std::shared_ptr<Page>> getDownwardPagesHeirarchy() const override;
    std::map<std::string, std::shared_ptr<Page>> getUpwardPagesHeirarchy() const override;
    std::vector<std::string> getAutowiredPagesIds() const override;
    std::vector<std::string> getParentGlobalAutowiredPagesIds() const override;
    bool isAutowired(const std::string& pageId) const override;
    std::shared_ptr<Page> getPageById(const std::string& id) const override;

    // Additional methods
    void clear();
    bool load(const std::string& path, const ProgressData& progressData);
    bool loadAndSetParent(const std::string& path, std::shared_ptr<NonLinearBook> parentNLB, std::shared_ptr<Page> parentPage);

    void exportMedia(bool recursively, const std::string& mediaDir, 
                    const std::string& exportDir, 
                    const std::vector<MediaFile>& mediaFiles,
                    MediaFile::Type type) const override;
                    
    std::shared_ptr<Page> createFilteredPage(const std::string& pageId, 
                                           const History& history) const override;
                                           
    SearchResultTableModel getVariables(const std::string& filter) const override;
    SearchResultTableModel getLeafs(const std::string& filter) const override;
    SearchResultTableModel searchText(const SearchContract& contract, 
                                    const std::string& filter) const override;
    bool findVariable(const std::string& variableName) const override;
    SearchResultTableModel checkBook(const std::string& filter) const override;
    BookStatistics getBookStatistics() const override;
    VariableStatistics getVariableStatistics() const override;
    std::shared_ptr<NonLinearBook> getParentNLB() const override;
    bool isDummy() const override;
    std::shared_ptr<Page> getParentPage() const override;
    std::map<std::string, std::shared_ptr<NonLinearBook>> getExternalModules() const override;
    std::shared_ptr<NonLinearBook> findExternalModule(const std::string& moduleId) const override;
    std::map<std::string, Variable::DataType> getVariableDataTypes() const override;
    std::map<std::string, std::string> getMediaToConstraintMap() const override;
    std::map<std::string, std::string> getMediaRedirectsMap() const override;
    std::map<std::string, MediaExportParameters> getMediaExportParametersMap() const override;
    std::map<std::string, bool> getMediaFlagsMap() const override;
    std::shared_ptr<Obj> getObjById(const std::string& id) const override { return nullptr; }
    std::shared_ptr<PageImpl> getPageImplById(const std::string& id) const;
    std::shared_ptr<ObjImpl> getObjImplById(const std::string& id) const;

    void exportImages(bool isRoot, const std::string& exportDir) const {}
    void exportSound(bool isRoot, const std::string& exportDir) const {}
    void exportToChoiceScript(const std::string& exportDir) const {}
    void exportToQSPTextFile(const std::string& exportDir) const {}
    void exportToURQTextFile(const std::string& exportDir) const {}
    void exportToPDFFile(const std::string& exportFile) const {}
    void exportToTXTFile(const std::string& exportDir) const {}
    void exportToHTMLFile(const std::string& exportDir) const {}
    void exportToJSIQFile(const std::string& exportDir) const {}
    void exportToSTEADFile(const std::string& exportDir) const {}
    void exportToVNSTEADFile(const std::string& exportDir) const {}
    void exportToASMFile(const std::string& exportDir) const {}
    void setMediaFileConstrId(
    MediaFile::Type mediaType,
    const std::string& fileName,
    const std::string& constrId
    ) {
        
    }

    void setMediaFileRedirect(
        MediaFile::Type mediaType,
        const std::string& fileName,
        const std::string& redirect
    ) {
        
    }

    void setMediaFileFlag(
        MediaFile::Type mediaType,
        const std::string& fileName,
        bool flag
    ) {
    
    }

    void setMediaFileExportParametersPreset(
        MediaFile::Type mediaType,
        const std::string& fileName,
        MediaExportParameters::Preset preset
    ) {
    
    }

    std::vector<std::shared_ptr<Link>> getAssociatedLinks(std::shared_ptr<NodeItem> nodeItem) {
        return {};
    }

    void copyAndAddImageFile(
        const FileManipulator& fileManipulator,
        const std::string& imageFile,
        const std::string& imageFileName
    ) {}

    void copyAndAddSoundFile(
        const FileManipulator& fileManipulator,
        const std::string& imageFile,
        const std::string& imageFileName
    ) {}

    void  removeImageFile(
        const FileManipulator& fileManipulator,
        const std::string& imageFileName
    ) {}

    void removeSoundFile(
        const FileManipulator& fileManipulator,
        const std::string& soundFileName
    ) {}

private:
    // Member variables
    std::shared_ptr<NonLinearBook> m_parentNLB;
    std::shared_ptr<Page> m_parentPage;
    std::string m_startPoint;
    Theme m_theme;
    std::string m_language;
    std::string m_license;
    std::string m_title;
    std::string m_author;
    std::string m_version;
    std::string m_perfectGameAchievementName;
    bool m_fullAutowire;
    bool m_suppressMedia;
    bool m_suppressSound;
    std::map<std::string, std::shared_ptr<Page>> m_pages;
    std::vector<std::string> m_autowiredPages;
    std::map<std::string, std::shared_ptr<Obj>> m_objs;
    std::vector<std::shared_ptr<Variable>> m_variables;
    std::vector<MediaFile> m_imageFiles;
    std::vector<MediaFile> m_soundFiles;

    // Private helper methods
    void readBookProperties(const std::string& rootDir);
    void loadPages(const std::string& rootDir);
    void loadObjs(const std::string& rootDir);
    void loadVariables(const std::string& rootDir);
    void loadMediaFiles(const std::string& rootDir, const std::string& mediaDirName, std::vector<MediaFile>& mediaFiles);
    void writeMediaFiles(const std::string& rootDir, const std::vector<MediaFile>& mediaFiles, const std::string& mediaDirName);

    // Override remaining virtual methods from NonLinearBook
    std::map<std::string, std::shared_ptr<Obj>> getObjs() const override { return m_objs; }
    std::vector<std::shared_ptr<Variable>> getVariables() const override { return {}; }
    std::shared_ptr<Variable> getVariableById(const std::string& id) const override { return nullptr; }
    
};
