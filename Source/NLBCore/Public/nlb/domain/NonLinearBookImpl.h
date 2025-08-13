#pragma once

#include "nlb/api/NonLinearBook.h"
#include "nlb/api/Theme.h"
#include "nlb/api/MediaFile.h"
#include "nlb/api/Page.h"
#include "nlb/api/Obj.h"
#include "nlb/api/Variable.h"
#include "nlb/api/SearchResultTableModel.h"
#include "nlb/domain/MediaExportParameters.h"
#include "nlb/util/MultiLangString.h"
#include <map>
#include <vector>
#include <set>
#include <memory>
#include <string>
#include <regex>

#include "ModificationImpl.h"
#include "nlb/api/NLBCommand.h"

#include "nlb/util/UUID.h"
#include "nlb/domain/Clipboard.h"

class PageImpl;
class ObjImpl;
class VariableImpl;
class MediaFileImpl;
class FileManipulator;
class ProgressData;
class PartialProgressData;
class SearchContract;
class History;
class Link;
class ModifyingItem;
class NodeItem;
class Modification;
class NLBCommand;
class ModificationsTableModel;
class LinksTableModel;

class NonLinearBookImpl : public NonLinearBook {
public:
    // Константы
    static const std::string PAGES_DIR_NAME;
    static const std::string OBJECTS_DIR_NAME;
    static const std::string VARIABLES_DIR_NAME;
    static const std::string STARTPOINT_FILE_NAME;
    static const std::string LANGUAGE_FILE_NAME;
    static const std::string LICENSE_FILE_NAME;  
    static const std::string THEME_FILE_NAME;
    static const std::string TITLE_FILE_NAME;
    static const std::string AUTHOR_FILE_NAME;
    static const std::string VERSION_FILE_NAME;
    static const std::string PERFECTGAMEACHIEVEMENTNAME_FILE_NAME;
    static const std::string FULL_AUTOWIRE_FILE_NAME;
    static const std::string SUPPRESS_MEDIA_FILE_NAME;
    static const std::string SUPPRESS_SOUND_FILE_NAME;
    static const std::string AUTOWIRED_PAGES_FILE_NAME;
    static const std::string PAGE_ORDER_FILE_NAME;
    static const std::string OBJ_ORDER_FILE_NAME;
    static const std::string VAR_ORDER_FILE_NAME;
    static const std::string MEDIA_EXPORT_PARAMETERS_DIR_NAME;
    static const std::string MEDIA_CONSTRAINTS_DIR_NAME;
    static const std::string MEDIA_REDIRECTS_DIR_NAME;
    static const std::string MEDIA_FLAGS_DIR_NAME;
    static const std::string MEDIA_EXPORT_PARAMETERS_SUFFIX;
    static const std::string MEDIA_CONSTRAINT_SUFFIX;
    static const std::string MEDIA_REDIRECT_SUFFIX;
    static const std::string MEDIA_FLAG_SUFFIX;
    static const std::string PAGEORDER_SEPARATOR;
    static const std::string OBJORDER_SEPARATOR;
    static const std::string VARORDER_SEPARATOR;
    static const std::string AUTOWIRED_PAGES_SEPARATOR;
    static const std::string NLB_EXTENSION;
	static const std::string CONSTRID_EXT;
	static const std::string REDIRECT_EXT;
	static const std::string FLAG_EXT;
	static const std::string PRESET_EXT;
	static const std::string MEDIA_FILE_NAME_TEMPLATE;

    // Конструкторы
    NonLinearBookImpl();
    NonLinearBookImpl(NonLinearBook* parentNLB, Page* parentPage);
    NonLinearBookImpl(const NonLinearBook* source,
                     NonLinearBook* parentNLB,
                     Page* parentPage);

    // Деструктор
    virtual ~NonLinearBookImpl() = default;

    // Переопределения методов из NonLinearBook
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

    std::map<std::string, Page*> getPages() const override;
    std::map<std::string, Page*> getDownwardPagesHeirarchy() const override;
    std::map<std::string, Page*> getUpwardPagesHeirarchy() const override;
    std::vector<std::string> getAutowiredPagesIds() const override;
    std::vector<std::string> getParentGlobalAutowiredPagesIds() const override;
    bool isAutowired(const std::string& pageId) const override;
    Page* getPageById(const std::string& id) const override;

    std::map<std::string, Obj*> getObjs() const override;
    Obj* getObjById(const std::string& objId) const override;

	void exportMedia(
		bool recursively,
		const std::string& mediaDir,
		const std::string& exportDir,
        const std::vector<MediaFile*>& mediaFiles,
		MediaFile::Type type) const override;
                    
    Page* createFilteredPage(const std::string& pageId,
                                           const History& history) const override;
                                           
    SearchResultTableModel getVariables(const std::string& filter) const override;
    SearchResultTableModel getLeafs(const std::string& filter) const override;
    SearchResultTableModel searchText(const SearchContract& contract, 
                                    const std::string& filter) const override;
    bool findVariable(const std::string& variableName) const override;
    SearchResultTableModel checkBook(const std::string& filter) const override;
    BookStatistics getBookStatistics() const override;
    VariableStatistics getVariableStatistics() const override;
    NonLinearBook* getParentNLB() const override;
    bool isDummy() const override;
    Page* getParentPage() const override;
    std::map<std::string, NonLinearBook*> getExternalModules() const override;
    NonLinearBook* findExternalModule(const std::string& moduleId) const override;
    std::map<std::string, Variable::DataType> getVariableDataTypes() const override;
    std::map<std::string, std::string> getMediaToConstraintMap() const override;
    std::map<std::string, std::string> getMediaRedirectsMap() const override;
    std::map<std::string, MediaExportParameters> getMediaExportParametersMap() const override;
    std::map<std::string, bool> getMediaFlagsMap() const override;

    std::vector<Variable*> getVariables() const override;
    Variable* getVariableById(const std::string& id) const override;
    void save(FileManipulator* fileManipulator, ProgressData* progressData,
              PartialProgressData* partialProgressData);
    bool load(const std::string& path, ProgressData &progressData) override;

    // Дополнительные методы
    void clear();
    bool loadAndSetParent(const std::string& path, NonLinearBook* parentNLB, Page* parentPage);
    void append(const NonLinearBook* source, bool generateNewIds, bool overwriteTheme);

    // Методы экспорта
    void exportImages(bool isRoot, const std::string& exportDir) const;
    void exportSound(bool isRoot, const std::string& exportDir) const;
    void exportToChoiceScript(const std::string& exportDir) const;
    void exportToQSPTextFile(const std::string& exportDir) const;
    void exportToURQTextFile(const std::string& exportDir) const;
    void exportToPDFFile(const std::string& exportFile) const;
    void exportToTXTFile(const std::string& exportDir) const;
    void exportToHTMLFile(const std::string& exportDir) const;
    void exportToJSIQFile(const std::string& exportDir) const;
    void exportToSTEADFile(const std::string& exportDir) const;
    void exportToVNSTEADFile(const std::string& exportDir) const;
    void exportToASMFile(const std::string& exportDir) const;

    // Методы работы с медиафайлами
    void setMediaFileConstrId(MediaFile::Type mediaType, const std::string& fileName, const std::string& constrId);
    void setMediaFileRedirect(MediaFile::Type mediaType, const std::string& fileName, const std::string& redirect);
    void setMediaFileFlag(MediaFile::Type mediaType, const std::string& fileName, bool flag);
    void setMediaFileExportParametersPreset(MediaFile::Type mediaType, const std::string& fileName, MediaExportParameters::Preset preset);

	/*!
	 * @brief Add image file to collection
	 * @param imageFile MediaFileImpl to add
	 */
    void addImageFile(MediaFileImpl* imageFile);
    
	/*!
	 * @brief Add sound file to collection  
	 * @param soundFile MediaFileImpl to add
	 */
    void addSoundFile(MediaFileImpl* soundFile);
	
    void copyAndAddImageFile(const FileManipulator& fileManipulator, const std::string& imageFile, const std::string& imageFileName);
    void copyAndAddSoundFile(const FileManipulator& fileManipulator, const std::string& soundFile, const std::string& soundFileName);
    void removeImageFile(const FileManipulator& fileManipulator, const std::string& imageFileName);
    void removeSoundFile(const FileManipulator& fileManipulator, const std::string& soundFileName);

    // Методы получения реализаций
    PageImpl* getPageImplById(const std::string& id) const;
    ObjImpl* getObjImplById(const std::string& id) const;
    VariableImpl* getVariableImplById(const std::string& id) const;

    // Вспомогательные методы для получения связанных элементов
    std::vector<Link*> getAssociatedLinks(NodeItem* nodeItem);

    // Методы для работы с командами (заглушки)
    NLBCommand* createUpdateModificationsCommand(ModifyingItem* modifyingItem, ModificationsTableModel* modificationsTableModel);
    void setStartPoint(const std::string& startPoint);
    void setLicense(const std::string& license);
    void setTheme(Theme theme);
    void setLanguage(const std::string& language);
    void setTitle(const std::string& title);
    void setAuthor(const std::string& author);
    void setVersion(const std::string& version);
    void setPerfectGameAchievementName(const std::string& name);
    void setFullAutowire(bool fullAutowire);
    void setSuppressMedia(bool suppressMedia);
    void setSuppressSound(bool suppressSound);
    void setRootDir(const std::string& rootDir);
    int getEffectivePagesCountForSave() const;
    NLBCommand* createUpdateBookPropertiesCommand(const std::string& license, Theme theme, const std::string& language, const std::string& title, const std::string& author, const std::string& version, const std::string& perfectGameAchievementName, bool fullAutowire, bool suppressMedia, bool suppressSound, bool propagateToSubmodules);
    NLBCommand* createUpdatePageCommand(Page* page, const std::string& imageFileName, bool imageBackground, bool imageAnimated, const std::string& soundFileName, bool soundSFX, const std::string& pageVariableName, const std::string& pageTimerVariableName, const std::string& pageDefTagVariableValue, MultiLangString* pageText, MultiLangString* pageCaptionText, Theme theme, bool useCaption, bool useMPL, const std::string& moduleName, bool moduleExternal, MultiLangString* traverseText, bool autoTraverse, bool autoReturn, MultiLangString* returnText, const std::string& returnPageId, const std::string& moduleConsraintVariableName, bool autowire, MultiLangString* autowireInText, MultiLangString* autowireOutText, bool autoIn, bool needsAction, bool autoOut, const std::string& autowireInConstraint, const std::string& autowireOutConstraint, bool globalAutowire, bool noSave, bool autosFirst, LinksTableModel* linksTableModel);
    NLBCommand* createChangeStartPointCommand(const std::string& startPoint);
    NLBCommand* createCopyCommand(const std::vector<std::string>& pageIds, const std::vector<std::string>& objIds);
    NLBCommand* createDeleteCommand(const std::vector<std::string>& pageIds, const std::vector<std::string>& objIds);
    NLBCommand* createPasteCommand(NonLinearBookImpl* nlbToPaste);

    // Методы загрузки и сохранения
    void writeNLB(FileManipulator* fileManipulator, const std::string& nlbDir, PartialProgressData* partialProgressData);
    void readNLB(const std::string& nlbDir, ProgressData &progressData);
    NLBCommand* createAddPageCommand(const PageImpl* pageImpl);
    NLBCommand* createAddObjCommand(const ObjImpl* objImpl);
    NLBCommand* createDeletePageCommand(PageImpl* page,
                                                        const std::vector<Link*>& adjacentLinks);
    NLBCommand* createDeleteObjCommand(ObjImpl* obj,
                                                       const std::vector<Link*>& adjacentLinks);

private:
    // Приватные методы
    void readBookProperties(const std::string& rootDir);
    void writeBookProperties(FileManipulator* fileManipulator, const std::string& rootDir);
    void loadPages(const std::string& rootDir, PartialProgressData* partialProgressData);
    void loadObjs(const std::string& rootDir, PartialProgressData* partialProgressData);
    void loadVariables(const std::string& rootDir, PartialProgressData* partialProgressData);
    void loadMediaFiles(const std::string& rootDir, const std::string& mediaDirName, std::vector<MediaFile*>& mediaFiles);
    void writeMediaFiles(FileManipulator* fileManipulator, const std::string& rootDir, const std::vector<MediaFile*>& mediaFiles, const std::string& mediaDirName);
    void writePages(FileManipulator* fileManipulator, const std::string& rootDir, PartialProgressData* partialProgressData);
    void writeObjs(FileManipulator* fileManipulator, const std::string& rootDir, PartialProgressData* partialProgressData);
    void writeVariables(FileManipulator* fileManipulator, const std::string& rootDir, PartialProgressData* partialProgressData);

    void writePageOrderFile(FileManipulator* fileManipulator, const std::string& rootDir);
    void writeObjOrderFile(FileManipulator* fileManipulator, const std::string& rootDir);
    void writeVarOrderFile(FileManipulator* fileManipulator, const std::string& rootDir);
    void writeAutowiredPagesFile(FileManipulator* fileManipulator, const std::string& rootDir);

    std::vector<std::string> createSortedDirList(const std::vector<std::string>& dirs, const std::vector<std::string>& orderList, const std::string& entityName);
    void validateVariableReferences();
    void resetVariableDataTypes();
    void processAutowiredPages();
    std::set<std::string> getUsedMediaFiles(MediaFile::Type mediaType) const;
    void addUsedImages(std::set<std::string>& usedImages, const std::string& imageFileName) const;
    void addUsedSounds(std::set<std::string>& usedSounds, const std::string& soundFileName) const;

    MediaFileImpl* copyMediaFile(
		FileManipulator& fileManipulator,
		const std::string& sourceFile,
		const std::string& fileName,
		const std::string& mediaDirName
	);
    
	std::string createUniqueMediaFile(
		FileManipulator& fileManipulator,
		const std::string& sourceFile,
		const std::string& fileName,
		const std::string& mediaDirName
	);

    // Переменные-члены
    NonLinearBook* m_parentNLB;
    Page* m_parentPage;
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
    std::string m_rootDir;
    
    std::map<std::string, PageImpl*> m_pages;
    std::vector<std::string> m_autowiredPages;
    std::map<std::string, ObjImpl*> m_objs;
    std::map<std::string, VariableImpl*> m_variables;
    std::vector<MediaFile*> m_imageFiles;
    std::vector<MediaFile*> m_soundFiles;

    // Карты для медиафайлов
    std::map<std::string, std::string> m_mediaToConstraintMap;
    std::map<std::string, std::string> m_mediaRedirectsMap;
    std::map<std::string, MediaExportParameters> m_mediaExportParametersMap;
    std::map<std::string, bool> m_mediaFlagsMap;
    std::map<std::string, Variable::DataType> m_variableDataTypes;

	class AddPageCommand : public NLBCommand {
	private:
	    NonLinearBookImpl* m_nlb;
        PageImpl* m_page;
	    
	public:
        AddPageCommand(NonLinearBookImpl* nlb, PageImpl* page);
	    void execute() override;
	    void revert() override;
	};

	class AddObjCommand : public NLBCommand {
	private:
	    NonLinearBookImpl* m_nlb;
        ObjImpl* m_obj;
	    
	public:
        AddObjCommand(NonLinearBookImpl* nlb, ObjImpl* obj);
	    void execute() override;
	    void revert() override;
	};

	class DeletePageCommand : public NLBCommand {
	private:
	    NonLinearBookImpl* m_nlb;
        PageImpl* m_page;
        std::vector<Link*> m_adjacentLinks;
	    bool m_wasDeleted;
	    
	public:
        DeletePageCommand(NonLinearBookImpl* nlb, PageImpl* page,
                         const std::vector<Link*>& adjacentLinks);
	    void execute() override;
	    void revert() override;
	};

	class DeleteObjCommand : public NLBCommand {
	private:
	    NonLinearBookImpl* m_nlb;
        ObjImpl* m_obj;
        std::vector<Link*> m_adjacentLinks;
	    bool m_wasDeleted;
	    
	public:
        DeleteObjCommand(NonLinearBookImpl* nlb, ObjImpl* obj,
                        const std::vector<Link*>& adjacentLinks);
	    void execute() override;
	    void revert() override;
	};

	class CopyCommand : public NLBCommand {
	private:
	    std::vector<std::string> m_pageIds;
	    std::vector<std::string> m_objIds;
	    
	public:
	    CopyCommand(const std::vector<std::string>& pageIds, const std::vector<std::string>& objIds);
	    void execute() override;
	    void revert() override;
	};

	class PasteCommand : public NLBCommand {
	private:
	    NonLinearBookImpl* m_nlb;
        NonLinearBookImpl* m_nlbToPaste;
	    std::vector<std::string> m_addedPageIds;
	    std::vector<std::string> m_addedObjIds;
	    
	public:
        PasteCommand(NonLinearBookImpl* nlb, NonLinearBookImpl* nlbToPaste);
	    void execute() override;
	    void revert() override;
	};

	class ChangeStartPointCommand : public NLBCommand {
	private:
	    NonLinearBookImpl* m_nlb;
	    std::string m_newStartPoint;
	    std::string m_oldStartPoint;
	    
	public:
	    ChangeStartPointCommand(NonLinearBookImpl* nlb, const std::string& startPoint);
	    void execute() override;
	    void revert() override;
	};

	class UpdateBookPropertiesCommand : public NLBCommand {
	private:
	    NonLinearBookImpl* m_nlb;
	    // Store old and new values
	    std::string m_oldLicense, m_newLicense;
	    Theme m_oldTheme, m_newTheme;
	    std::string m_oldLanguage, m_newLanguage;
	    std::string m_oldTitle, m_newTitle;
	    std::string m_oldAuthor, m_newAuthor;
	    std::string m_oldVersion, m_newVersion;
	    std::string m_oldPerfectGameAchievementName, m_newPerfectGameAchievementName;
	    bool m_oldFullAutowire, m_newFullAutowire;
	    bool m_oldSuppressMedia, m_newSuppressMedia;
	    bool m_oldSuppressSound, m_newSuppressSound;
	    bool m_propagateToSubmodules;
	    
	public:
	    UpdateBookPropertiesCommand(NonLinearBookImpl* nlb,
	                               const std::string& license, Theme theme,
	                               const std::string& language, const std::string& title,
	                               const std::string& author, const std::string& version,
	                               const std::string& perfectGameAchievementName,
	                               bool fullAutowire, bool suppressMedia, bool suppressSound,
	                               bool propagateToSubmodules);
	    void execute() override;
	    void revert() override;
	};

	class UpdateModificationsCommand : public NLBCommand {
	private:
        ModifyingItem* m_modifyingItem;
        ModificationsTableModel* m_modificationsTableModel;
        std::vector<ModificationImpl*> m_oldModifications;
	    
	public:
        UpdateModificationsCommand(ModifyingItem* modifyingItem,
                                  ModificationsTableModel* modificationsTableModel);
	    void execute() override;
	    void revert() override;
	};
};
