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

    const std::vector<MediaFile>& getImageFiles() override;
    const std::vector<MediaFile>& getSoundFiles() override;

    std::map<std::string, std::shared_ptr<Page>> getPages() override;
    std::map<std::string, std::shared_ptr<Page>> getDownwardPagesHeirarchy() override;
    std::map<std::string, std::shared_ptr<Page>> getUpwardPagesHeirarchy() override;
    std::vector<std::string> getAutowiredPagesIds() override;
    std::vector<std::string> getParentGlobalAutowiredPagesIds() override;
    bool isAutowired(const std::string& pageId) override;
    std::shared_ptr<Page> getPageById(const std::string& id) override;

    // Additional methods
    void clear();
    bool load(const std::string& path, const ProgressData& progressData);
    bool loadAndSetParent(const std::string& path, std::shared_ptr<NonLinearBook> parentNLB, std::shared_ptr<Page> parentPage);

    void exportMedia(bool recursively, const std::string& mediaDir, 
                    const std::string& exportDir, 
                    const std::vector<MediaFile>& mediaFiles,
                    MediaFile::Type type) override;
                    
    std::shared_ptr<Page> createFilteredPage(const std::string& pageId, 
                                           const History& history) override;
                                           
    SearchResultTableModel getVariables(const std::string& filter) override;
    SearchResultTableModel getLeafs(const std::string& filter) override;
    SearchResultTableModel searchText(const SearchContract& contract, 
                                    const std::string& filter) override;
    bool findVariable(const std::string& variableName) override;
    SearchResultTableModel checkBook(const std::string& filter) override;
    BookStatistics getBookStatistics() override;
    VariableStatistics getVariableStatistics() override;
    std::shared_ptr<NonLinearBook> getParentNLB() override;
    bool isDummy() override;
    std::shared_ptr<Page> getParentPage() override;
    std::map<std::string, std::shared_ptr<NonLinearBook>> getExternalModules() override;
    std::shared_ptr<NonLinearBook> findExternalModule(const std::string& moduleId) override;
    std::map<std::string, Variable::DataType> getVariableDataTypes() override;
    std::map<std::string, std::string> getMediaToConstraintMap() override;
    std::map<std::string, std::string> getMediaRedirectsMap() override;
    std::map<std::string, MediaExportParameters> getMediaExportParametersMap() override;
    std::map<std::string, bool> getMediaFlagsMap() override;
    std::shared_ptr<Obj> getObjById(const std::string& id) override { return nullptr; }
    std::shared_ptr<PageImpl> getPageImplById(const std::string& id) const;
std::shared_ptr<ObjImpl> getObjImplById(const std::string& id) const;

    void exportImages(bool isRoot, const std::string& exportDir) {}
    void exportSound(bool isRoot, const std::string& exportDir) {}
    void exportToChoiceScript(const std::string& exportDir) {}
    void exportToQSPTextFile(const std::string& exportDir) {}
    void exportToURQTextFile(const std::string& exportDir) {}
    void exportToPDFFile(const std::string& exportFile) {}
    void exportToTXTFile(const std::string& exportDir) {}
    void exportToHTMLFile(const std::string& exportDir) {}
    void exportToJSIQFile(const std::string& exportDir) {}
    void exportToSTEADFile(const std::string& exportDir) {}
    void exportToVNSTEADFile(const std::string& exportDir) {}
    void exportToASMFile(const std::string& exportDir) {}
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
    std::map<std::string, std::shared_ptr<Obj>> getObjs() override { return m_objs; }
    std::vector<std::shared_ptr<Variable>> getVariables() override { return {}; }
    std::shared_ptr<Variable> getVariableById(const std::string& id) override { return nullptr; }
    
};
