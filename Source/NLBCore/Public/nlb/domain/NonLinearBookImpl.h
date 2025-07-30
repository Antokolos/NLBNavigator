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

class NonLinearBookImpl : public NonLinearBook, public std::enable_shared_from_this<NonLinearBookImpl> {
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
    NonLinearBookImpl(std::shared_ptr<NonLinearBook> parentNLB, std::shared_ptr<Page> parentPage);
    NonLinearBookImpl(const std::shared_ptr<NonLinearBook>& source, 
                     std::shared_ptr<NonLinearBook> parentNLB, 
                     std::shared_ptr<Page> parentPage);

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
	const std::vector<std::shared_ptr<MediaFile>>& getImageFiles() const override;
	const std::vector<std::shared_ptr<MediaFile>>& getSoundFiles() const override;

    std::map<std::string, std::shared_ptr<Page>> getPages() const override;
    std::map<std::string, std::shared_ptr<Page>> getDownwardPagesHeirarchy() const override;
    std::map<std::string, std::shared_ptr<Page>> getUpwardPagesHeirarchy() const override;
    std::vector<std::string> getAutowiredPagesIds() const override;
    std::vector<std::string> getParentGlobalAutowiredPagesIds() const override;
    bool isAutowired(const std::string& pageId) const override;
    std::shared_ptr<Page> getPageById(const std::string& id) const override;

    std::map<std::string, std::shared_ptr<Obj>> getObjs() const override;
    std::shared_ptr<Obj> getObjById(const std::string& objId) const override;

	void exportMedia(
		bool recursively,
		const std::string& mediaDir,
		const std::string& exportDir,
		const std::vector<std::shared_ptr<MediaFile>>& mediaFiles,
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

    std::vector<std::shared_ptr<Variable>> getVariables() const override;
    std::shared_ptr<Variable> getVariableById(const std::string& id) const override;
    void save(std::shared_ptr<FileManipulator> fileManipulator, std::shared_ptr<ProgressData> progressData,
              std::shared_ptr<PartialProgressData> partialProgressData);
    bool load(const std::string& path, const ProgressData& progressData) override;

    // Дополнительные методы
    void clear();
    bool loadAndSetParent(const std::string& path, std::shared_ptr<NonLinearBook> parentNLB, std::shared_ptr<Page> parentPage);
    void append(const std::shared_ptr<NonLinearBook>& source, bool generateNewIds, bool overwriteTheme);

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
	void addImageFile(std::shared_ptr<MediaFileImpl> imageFile);
    
	/*!
	 * @brief Add sound file to collection  
	 * @param soundFile MediaFileImpl to add
	 */
	void addSoundFile(std::shared_ptr<MediaFileImpl> soundFile);
	
    void copyAndAddImageFile(const FileManipulator& fileManipulator, const std::string& imageFile, const std::string& imageFileName);
    void copyAndAddSoundFile(const FileManipulator& fileManipulator, const std::string& soundFile, const std::string& soundFileName);
    void removeImageFile(const FileManipulator& fileManipulator, const std::string& imageFileName);
    void removeSoundFile(const FileManipulator& fileManipulator, const std::string& soundFileName);

    // Методы получения реализаций
    std::shared_ptr<PageImpl> getPageImplById(const std::string& id) const;
    std::shared_ptr<ObjImpl> getObjImplById(const std::string& id) const;
    std::shared_ptr<VariableImpl> getVariableImplById(const std::string& id) const;

    // Вспомогательные методы для получения связанных элементов
    std::vector<std::shared_ptr<Link>> getAssociatedLinks(std::shared_ptr<NodeItem> nodeItem);

    // Методы для работы с командами (заглушки)
    std::shared_ptr<NLBCommand> createUpdateModificationsCommand(std::shared_ptr<ModifyingItem> modifyingItem, std::shared_ptr<ModificationsTableModel> modificationsTableModel);
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
    std::shared_ptr<NLBCommand> createUpdateBookPropertiesCommand(const std::string& license, Theme theme, const std::string& language, const std::string& title, const std::string& author, const std::string& version, const std::string& perfectGameAchievementName, bool fullAutowire, bool suppressMedia, bool suppressSound, bool propagateToSubmodules);
    std::shared_ptr<NLBCommand> createUpdatePageCommand(std::shared_ptr<Page> page, const std::string& imageFileName, bool imageBackground, bool imageAnimated, const std::string& soundFileName, bool soundSFX, const std::string& pageVariableName, const std::string& pageTimerVariableName, const std::string& pageDefTagVariableValue, std::shared_ptr<MultiLangString> pageText, std::shared_ptr<MultiLangString> pageCaptionText, Theme theme, bool useCaption, bool useMPL, const std::string& moduleName, bool moduleExternal, std::shared_ptr<MultiLangString> traverseText, bool autoTraverse, bool autoReturn, std::shared_ptr<MultiLangString> returnText, const std::string& returnPageId, const std::string& moduleConsraintVariableName, bool autowire, std::shared_ptr<MultiLangString> autowireInText, std::shared_ptr<MultiLangString> autowireOutText, bool autoIn, bool needsAction, bool autoOut, const std::string& autowireInConstraint, const std::string& autowireOutConstraint, bool globalAutowire, bool noSave, bool autosFirst, std::shared_ptr<LinksTableModel> linksTableModel);
    std::shared_ptr<NLBCommand> createChangeStartPointCommand(const std::string& startPoint);
    std::shared_ptr<NLBCommand> createCopyCommand(const std::vector<std::string>& pageIds, const std::vector<std::string>& objIds);
    std::shared_ptr<NLBCommand> createDeleteCommand(const std::vector<std::string>& pageIds, const std::vector<std::string>& objIds);
    std::shared_ptr<NLBCommand> createPasteCommand(std::shared_ptr<NonLinearBookImpl> nlbToPaste);

    // Методы загрузки и сохранения
    void loadBook(const std::string& path, std::shared_ptr<ProgressData> progressData);
    void saveBook(const std::string& path, std::shared_ptr<ProgressData> progressData);
    void writeNLB(std::shared_ptr<FileManipulator> fileManipulator, const std::string& nlbDir, std::shared_ptr<PartialProgressData> partialProgressData);
    void readNLB(const std::string& nlbDir);
    std::shared_ptr<NLBCommand> createAddPageCommand(const std::shared_ptr<PageImpl>& pageImpl);
    std::shared_ptr<NLBCommand> createAddObjCommand(const std::shared_ptr<ObjImpl>& objImpl);
    std::shared_ptr<NLBCommand> createDeletePageCommand(std::shared_ptr<PageImpl> page,
                                                        const std::vector<std::shared_ptr<Link>>& adjacentLinks);
    std::shared_ptr<NLBCommand> createDeleteObjCommand(std::shared_ptr<ObjImpl> obj,
                                                       const std::vector<std::shared_ptr<Link>>& adjacentLinks);

private:
    // Приватные методы
    void readBookProperties(const std::string& rootDir);
    void writeBookProperties(std::shared_ptr<FileManipulator> fileManipulator, const std::string& rootDir);
    void loadPages(const std::string& rootDir, std::shared_ptr<PartialProgressData> partialProgressData);
    void loadObjs(const std::string& rootDir, std::shared_ptr<PartialProgressData> partialProgressData);
    void loadVariables(const std::string& rootDir, std::shared_ptr<PartialProgressData> partialProgressData);
    void loadMediaFiles(const std::string& rootDir, const std::string& mediaDirName, std::vector<std::shared_ptr<MediaFile>>& mediaFiles);
    void writeMediaFiles(std::shared_ptr<FileManipulator> fileManipulator, const std::string& rootDir, const std::vector<std::shared_ptr<MediaFile>>& mediaFiles, const std::string& mediaDirName);
    void writePages(std::shared_ptr<FileManipulator> fileManipulator, const std::string& rootDir, std::shared_ptr<PartialProgressData> partialProgressData);
    void writeObjs(std::shared_ptr<FileManipulator> fileManipulator, const std::string& rootDir, std::shared_ptr<PartialProgressData> partialProgressData);
    void writeVariables(std::shared_ptr<FileManipulator> fileManipulator, const std::string& rootDir, std::shared_ptr<PartialProgressData> partialProgressData);

    void writePageOrderFile(std::shared_ptr<FileManipulator> fileManipulator, const std::string& rootDir);
    void writeObjOrderFile(std::shared_ptr<FileManipulator> fileManipulator, const std::string& rootDir);
    void writeVarOrderFile(std::shared_ptr<FileManipulator> fileManipulator, const std::string& rootDir);
    void writeAutowiredPagesFile(std::shared_ptr<FileManipulator> fileManipulator, const std::string& rootDir);

    std::vector<std::string> createSortedDirList(const std::vector<std::string>& dirs, const std::vector<std::string>& orderList, const std::string& entityName);
    void validateVariableReferences();
    void resetVariableDataTypes();
    void processAutowiredPages();
    std::set<std::string> getUsedMediaFiles(MediaFile::Type mediaType) const;
    void addUsedImages(std::set<std::string>& usedImages, const std::string& imageFileName) const;
    void addUsedSounds(std::set<std::string>& usedSounds, const std::string& soundFileName) const;

	std::shared_ptr<MediaFileImpl> copyMediaFile(
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
    std::string m_rootDir;
    
    std::map<std::string, std::shared_ptr<PageImpl>> m_pages;
    std::vector<std::string> m_autowiredPages;
    std::map<std::string, std::shared_ptr<ObjImpl>> m_objs;
    std::map<std::string, std::shared_ptr<VariableImpl>> m_variables;
	std::vector<std::shared_ptr<MediaFile>> m_imageFiles;
	std::vector<std::shared_ptr<MediaFile>> m_soundFiles;

    // Карты для медиафайлов
    std::map<std::string, std::string> m_mediaToConstraintMap;
    std::map<std::string, std::string> m_mediaRedirectsMap;
    std::map<std::string, MediaExportParameters> m_mediaExportParametersMap;
    std::map<std::string, bool> m_mediaFlagsMap;
    std::map<std::string, Variable::DataType> m_variableDataTypes;

	class AddPageCommand : public NLBCommand {
	private:
	    NonLinearBookImpl* m_nlb;
	    std::shared_ptr<PageImpl> m_page;
	    
	public:
	    AddPageCommand(NonLinearBookImpl* nlb, std::shared_ptr<PageImpl> page);
	    void execute() override;
	    void revert() override;
	};

	class AddObjCommand : public NLBCommand {
	private:
	    NonLinearBookImpl* m_nlb;
	    std::shared_ptr<ObjImpl> m_obj;
	    
	public:
	    AddObjCommand(NonLinearBookImpl* nlb, std::shared_ptr<ObjImpl> obj);
	    void execute() override;
	    void revert() override;
	};

	class DeletePageCommand : public NLBCommand {
	private:
	    NonLinearBookImpl* m_nlb;
	    std::shared_ptr<PageImpl> m_page;
	    std::vector<std::shared_ptr<Link>> m_adjacentLinks;
	    bool m_wasDeleted;
	    
	public:
	    DeletePageCommand(NonLinearBookImpl* nlb, std::shared_ptr<PageImpl> page, 
	                     const std::vector<std::shared_ptr<Link>>& adjacentLinks);
	    void execute() override;
	    void revert() override;
	};

	class DeleteObjCommand : public NLBCommand {
	private:
	    NonLinearBookImpl* m_nlb;
	    std::shared_ptr<ObjImpl> m_obj;
	    std::vector<std::shared_ptr<Link>> m_adjacentLinks;
	    bool m_wasDeleted;
	    
	public:
	    DeleteObjCommand(NonLinearBookImpl* nlb, std::shared_ptr<ObjImpl> obj,
	                    const std::vector<std::shared_ptr<Link>>& adjacentLinks);
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
	    std::shared_ptr<NonLinearBookImpl> m_nlbToPaste;
	    std::vector<std::string> m_addedPageIds;
	    std::vector<std::string> m_addedObjIds;
	    
	public:
	    PasteCommand(NonLinearBookImpl* nlb, std::shared_ptr<NonLinearBookImpl> nlbToPaste);
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
	    std::shared_ptr<ModifyingItem> m_modifyingItem;
	    std::shared_ptr<ModificationsTableModel> m_modificationsTableModel;
	    std::vector<std::shared_ptr<ModificationImpl>> m_oldModifications;
	    
	public:
	    UpdateModificationsCommand(std::shared_ptr<ModifyingItem> modifyingItem,
	                              std::shared_ptr<ModificationsTableModel> modificationsTableModel);
	    void execute() override;
	    void revert() override;
	};
};