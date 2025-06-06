#pragma once

#include <string>
#include <set>
#include <map>
#include <vector>
#include <memory>
#include "nlb/api/Constants.h"
#include "nlb/api/Theme.h"
#include "nlb/api/MediaFile.h"
#include "nlb/api/Variable.h"

class Page;
class Obj;
class History;
class SearchResultTableModel;
class SearchContract;
class ProgressData;
class MediaExportParameters;

/*!
 * NonLinearBook interface defines core functionality for non-linear book handling
 */
class NonLinearBook {
public:
    // Constants
    inline static const std::string TRUE_VARID = "TRUE";
    inline static const std::string FALSE_VARID = "FALSE";
    inline static const std::string LC_VARID_PREFIX = "LC_";
    inline static const std::string LC_VARID_SEPARATOR_OUT = "_OUT_";

    inline static const std::string SOUND_DIR_NAME = "sound";
    inline static const std::string IMAGES_DIR_NAME = "images";
    inline static const std::string DEFAULT_STARTPOINT = "";
    inline static const Theme DEFAULT_THEME = Theme::DEFAULT;
    inline static const std::string DEFAULT_LANGUAGE = "ru";
    inline static const std::string DEFAULT_LICENSE = "";
    inline static constexpr bool DEFAULT_FULL_AUTOWIRE = false;
    inline static constexpr bool DEFAULT_SUPPRESS_MEDIA = false;
    inline static constexpr bool DEFAULT_SUPPRESS_SOUND = false;
    inline static const std::string DEFAULT_TITLE = "";
    inline static const std::string DEFAULT_AUTHOR = "";
    inline static const std::string DEFAULT_VERSION = "";
    inline static const std::string DEFAULT_PERFECT_GAME_ACHIEVEMENT_NAME = "";

    /*!
     * Module information class
     */
    class ModuleInfo {
    public:
        ModuleInfo(const std::string& modulePageId, const std::string& moduleName, int depth)
            : m_modulePageId(modulePageId), m_moduleName(moduleName), m_depth(depth) {}

        std::string getModulePageId() const { return m_modulePageId; }
        std::string getModuleName() const { return m_moduleName; }
        std::string toString() const;

    private:
        std::string m_modulePageId;
        std::string m_moduleName;
        int m_depth;
    };

    /*!
     * Book statistics class
     */
    class BookStatistics {
    public:
        BookStatistics() : m_pagesCount(0), m_objsCount(0), m_uniqueEndings(0), m_charactersCount(0) {}

        const std::vector<ModuleInfo>& getModuleInfos() const { return m_moduleInfos; }
        void addModuleInfo(const ModuleInfo& moduleInfo) { m_moduleInfos.push_back(moduleInfo); }
        const std::vector<ModuleInfo>& getModulesToBeDeletedInfos() const { return m_modulesToBeDeletedInfos; }
        void addModuleToBeDeletedInfo(const ModuleInfo& moduleInfo) { m_modulesToBeDeletedInfos.push_back(moduleInfo); }

        int getPagesCount() const { return m_pagesCount; }
        void incPagesCount(int pagesCount) { m_pagesCount += pagesCount; }
        int getObjsCount() const { return m_objsCount; }
        void incObjsCount(int objsCount) { m_objsCount += objsCount; }
        int getUniqueEndings() const { return m_uniqueEndings; }
        void incUniqueEndings(int uniqueEndings) { m_uniqueEndings += uniqueEndings; }
        int getCharactersCount() const { return m_charactersCount; }
        void incCharactersCount(int charactersCount) { m_charactersCount += charactersCount; }
        void addBookStatistics(const BookStatistics& bookStatistics);
        void addDeletedModulesFromBookStatistics(const BookStatistics& bookStatistics);
        int getModulesCount() const { return static_cast<int>(m_moduleInfos.size()); }

    private:
        int m_pagesCount;
        int m_objsCount;
        int m_uniqueEndings;
        int m_charactersCount;
        std::vector<ModuleInfo> m_moduleInfos;
        std::vector<ModuleInfo> m_modulesToBeDeletedInfos;
    };

    /*!
     * Variable statistics class
     */
    class VariableStatistics {
    public:
        VariableStatistics() : m_pageVariablesCount(0), m_pageTimerVariablesCount(0),
            m_objVariablesCount(0), m_objConstraintsCount(0), m_objRefsCount(0),
            m_linkVariablesCount(0), m_linkConstraintVariablesCount(0),
            m_plainVariablesCount(0), m_expressionsCount(0),
            m_moduleConstraintCount(0), m_autowireConstraintCount(0) {}

        int getPageVariablesCount() const { return m_pageVariablesCount; }
        void incPageVariablesCount() { ++m_pageVariablesCount; }
        int getPageTimerVariablesCount() const { return m_pageTimerVariablesCount; }
        void incPageTimerVariablesCount() { ++m_pageTimerVariablesCount; }
        int getObjVariablesCount() const { return m_objVariablesCount; }
        void incObjVariablesCount() { ++m_objVariablesCount; }
        int getObjConstraintsCount() const { return m_objConstraintsCount; }
        void incObjConstraintsCount() { ++m_objConstraintsCount; }
        int getObjRefsCount() const { return m_objRefsCount; }
        void incObjRefsCount() { ++m_objRefsCount; }
        int getLinkVariablesCount() const { return m_linkVariablesCount; }
        void incLinkVariablesCount() { ++m_linkVariablesCount; }
        int getLinkConstraintVariablesCount() const { return m_linkConstraintVariablesCount; }
        void incLinkConstraintVariablesCount() { ++m_linkConstraintVariablesCount; }
        int getPlainVariablesCount() const { return m_plainVariablesCount; }
        void incPlainVariablesCount() { ++m_plainVariablesCount; }
        int getExpressionsCount() const { return m_expressionsCount; }
        void incExpressionsCount() { ++m_expressionsCount; }
        int getModuleConstraintCount() const { return m_moduleConstraintCount; }
        void incModuleConstraintCount() { ++m_moduleConstraintCount; }
        int getAutowireConstraintCount() const { return m_autowireConstraintCount; }
        void incAutowireConstraintCount() { ++m_autowireConstraintCount; }

    private:
        int m_pageVariablesCount;
        int m_pageTimerVariablesCount;
        int m_objVariablesCount;
        int m_objConstraintsCount;
        int m_objRefsCount;
        int m_linkVariablesCount;
        int m_linkConstraintVariablesCount;
        int m_plainVariablesCount;
        int m_expressionsCount;
        int m_moduleConstraintCount;
        int m_autowireConstraintCount;
    };

    virtual ~NonLinearBook() = default;

    virtual std::set<std::string> getAllAchievementNames(bool recursive) const = 0;
    virtual std::string getPerfectGameAchievementName() const = 0;
    virtual bool isEmpty() const = 0;
    virtual std::string getStartPoint() const = 0;
    virtual std::string getLanguage() const = 0;
    virtual std::string getLicense() const = 0;
    virtual Theme getTheme() const = 0;
    virtual bool isFullAutowire() const = 0;
    virtual bool isSuppressMedia() const = 0;
    virtual bool isSuppressSound() const = 0;
    virtual std::string getTitle() const = 0;
    virtual std::string getAuthor() const = 0;
    virtual std::string getVersion() const = 0;
    virtual std::string getRootDir() const = 0;
    virtual std::string getImagesDir() const = 0;

    /*!
     * Return set of image file names that has usages in the book
     */
    virtual std::set<std::string> getUsedImages() const = 0;

    /*!
     * Return set of sound file names that has usages in the book
     */
    virtual std::set<std::string> getUsedSounds() const = 0;

    virtual const std::vector<std::shared_ptr<MediaFile>>& getImageFiles() const = 0;
    virtual const std::vector<std::shared_ptr<MediaFile>>& getSoundFiles() const = 0;

    virtual void exportMedia(
        bool isRoot,
        const std::string& mainExportDir,
        const std::string& mediaDirName,
        const std::vector<std::shared_ptr<MediaFile>>& mediaFiles,
        MediaFile::Type mediaType
    ) const = 0;

    virtual std::map<std::string, std::shared_ptr<Page>> getPages() const = 0;
    virtual std::map<std::string, std::shared_ptr<Page>> getDownwardPagesHeirarchy() const = 0;
    virtual std::map<std::string, std::shared_ptr<Page>> getUpwardPagesHeirarchy() const = 0;
    virtual std::vector<std::string> getAutowiredPagesIds() const = 0;
    virtual std::vector<std::string> getParentGlobalAutowiredPagesIds() const = 0;
    virtual bool isAutowired(const std::string& pageId) const = 0;
    virtual std::shared_ptr<Page> getPageById(const std::string& id) const = 0;
    virtual std::map<std::string, std::shared_ptr<Obj>> getObjs() const = 0;
    virtual std::shared_ptr<Obj> getObjById(const std::string& objId) const = 0;
    virtual std::shared_ptr<Page> createFilteredPage(const std::string& sourceId, const History& history) const = 0;
    virtual bool load(const std::string& path, const ProgressData& progressData) = 0;
    virtual std::shared_ptr<Variable> getVariableById(const std::string& varId) const = 0;
    virtual std::vector<std::shared_ptr<Variable>> getVariables() const = 0;
    virtual SearchResultTableModel getLeafs(const std::string& modulePageId) const = 0;
    virtual SearchResultTableModel searchText(const SearchContract& searchContract, const std::string& modulePageId) const = 0;
    virtual SearchResultTableModel getVariables(const std::string& modulePageId) const = 0;
    virtual bool findVariable(const std::string& variableNameToFind) const = 0;
    virtual SearchResultTableModel checkBook(const std::string& modulePageId) const = 0;
    virtual BookStatistics getBookStatistics() const = 0;
    virtual VariableStatistics getVariableStatistics() const = 0;
    virtual std::shared_ptr<NonLinearBook> getParentNLB() const = 0;
    virtual bool isDummy() const = 0;
    virtual std::shared_ptr<Page> getParentPage() const = 0;
    virtual std::map<std::string, std::shared_ptr<NonLinearBook>> getExternalModules() const = 0;

    /*!
     * Find external module recursively by name (if not found in the current book, will find in the parent book and so on)
     */
    virtual std::shared_ptr<NonLinearBook> findExternalModule(const std::string& name) const = 0;

    virtual std::map<std::string, Variable::DataType> getVariableDataTypes() const = 0;
    virtual std::map<std::string, std::string> getMediaToConstraintMap() const = 0;
    virtual std::map<std::string, std::string> getMediaRedirectsMap() const = 0;
    virtual std::map<std::string, MediaExportParameters> getMediaExportParametersMap() const = 0;
    virtual std::map<std::string, bool> getMediaFlagsMap() const = 0;
};