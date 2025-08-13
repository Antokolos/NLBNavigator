#include "nlb/domain/NonLinearBookImpl.h"
#include "nlb/domain/MediaExportParameters.h"
#include "nlb/domain/PageImpl.h"
#include "nlb/domain/ObjImpl.h"
#include "nlb/domain/LinkImpl.h"
#include "nlb/domain/VariableImpl.h"
#include "nlb/util/FileManipulator.h"
#include "nlb/api/Constants.h"
#include "nlb/api/SearchResultTableModel.h"
#include "nlb/api/ProgressData.h"
#include "nlb/api/PartialProgressData.h"
#include "nlb/api/SearchContract.h"
#include "nlb/api/Link.h"
#include "nlb/util/FileUtils.h"
#include "nlb/util/StringHelper.h"
#include "nlb/exception/NLBExceptions.h"

#include "nlb/domain/MediaFileImpl.h"

// Константы
const std::string NonLinearBookImpl::PAGES_DIR_NAME = "pages";
const std::string NonLinearBookImpl::OBJECTS_DIR_NAME = "objs";
const std::string NonLinearBookImpl::VARIABLES_DIR_NAME = "vars";
const std::string NonLinearBookImpl::STARTPOINT_FILE_NAME = "startpoint";
const std::string NonLinearBookImpl::LANGUAGE_FILE_NAME = "language";
const std::string NonLinearBookImpl::LICENSE_FILE_NAME = "license";
const std::string NonLinearBookImpl::THEME_FILE_NAME = "theme";
const std::string NonLinearBookImpl::TITLE_FILE_NAME = "title";
const std::string NonLinearBookImpl::AUTHOR_FILE_NAME = "author";
const std::string NonLinearBookImpl::VERSION_FILE_NAME = "version";
const std::string NonLinearBookImpl::PERFECTGAMEACHIEVEMENTNAME_FILE_NAME = "perfectgameachievementname";
const std::string NonLinearBookImpl::FULL_AUTOWIRE_FILE_NAME = "fullautowire";
const std::string NonLinearBookImpl::SUPPRESS_MEDIA_FILE_NAME = "suppressmedia";
const std::string NonLinearBookImpl::SUPPRESS_SOUND_FILE_NAME = "suppresssound";
const std::string NonLinearBookImpl::AUTOWIRED_PAGES_FILE_NAME = "autowiredpages";
const std::string NonLinearBookImpl::PAGE_ORDER_FILE_NAME = "pageorder";
const std::string NonLinearBookImpl::OBJ_ORDER_FILE_NAME = "objorder";
const std::string NonLinearBookImpl::VAR_ORDER_FILE_NAME = "varorder";
const std::string NonLinearBookImpl::MEDIA_EXPORT_PARAMETERS_DIR_NAME = "mediaexportparameters";
const std::string NonLinearBookImpl::MEDIA_CONSTRAINTS_DIR_NAME = "mediaconstraints";
const std::string NonLinearBookImpl::MEDIA_REDIRECTS_DIR_NAME = "mediaredirects";
const std::string NonLinearBookImpl::MEDIA_FLAGS_DIR_NAME = "mediaflags";
const std::string NonLinearBookImpl::MEDIA_EXPORT_PARAMETERS_SUFFIX = ".mep";
const std::string NonLinearBookImpl::MEDIA_CONSTRAINT_SUFFIX = ".mc";
const std::string NonLinearBookImpl::MEDIA_REDIRECT_SUFFIX = ".mr";
const std::string NonLinearBookImpl::MEDIA_FLAG_SUFFIX = ".mf";
const std::string NonLinearBookImpl::PAGEORDER_SEPARATOR = "\n";
const std::string NonLinearBookImpl::OBJORDER_SEPARATOR = "\n";
const std::string NonLinearBookImpl::VARORDER_SEPARATOR = "\n";
const std::string NonLinearBookImpl::AUTOWIRED_PAGES_SEPARATOR = "\n";
const std::string NonLinearBookImpl::NLB_EXTENSION = ".nlb";
const std::string NonLinearBookImpl::CONSTRID_EXT = ".constrid";
const std::string NonLinearBookImpl::REDIRECT_EXT = ".redirect";
const std::string NonLinearBookImpl::FLAG_EXT = ".flag";
const std::string NonLinearBookImpl::PRESET_EXT = ".preset";
const std::string NonLinearBookImpl::MEDIA_FILE_NAME_TEMPLATE = "%s_%d%s";

// Конструкторы
NonLinearBookImpl::NonLinearBookImpl()
    : m_parentNLB(nullptr), m_parentPage(nullptr),
      m_startPoint(DEFAULT_STARTPOINT),
      m_theme(DEFAULT_THEME),
      m_language(DEFAULT_LANGUAGE),
      m_license(DEFAULT_LICENSE),
      m_title(DEFAULT_TITLE),
      m_author(DEFAULT_AUTHOR),
      m_version(DEFAULT_VERSION),
      m_perfectGameAchievementName(DEFAULT_PERFECT_GAME_ACHIEVEMENT_NAME),
      m_fullAutowire(DEFAULT_FULL_AUTOWIRE),
      m_suppressMedia(DEFAULT_SUPPRESS_MEDIA),
      m_suppressSound(DEFAULT_SUPPRESS_SOUND) {
}

NonLinearBookImpl::NonLinearBookImpl(NonLinearBook* parentNLB, Page* parentPage)
    : m_parentNLB(parentNLB), m_parentPage(parentPage),
      m_startPoint(DEFAULT_STARTPOINT),
      m_theme(parentNLB ? parentNLB->getTheme() : DEFAULT_THEME),
      m_language(parentNLB ? parentNLB->getLanguage() : DEFAULT_LANGUAGE),
      m_license(parentNLB ? parentNLB->getLicense() : DEFAULT_LICENSE),
      m_title(DEFAULT_TITLE),
      m_author(parentNLB ? parentNLB->getAuthor() : DEFAULT_AUTHOR),
      m_version(parentNLB ? parentNLB->getVersion() : DEFAULT_VERSION),
      m_perfectGameAchievementName(DEFAULT_PERFECT_GAME_ACHIEVEMENT_NAME),
      m_fullAutowire(parentNLB ? parentNLB->isFullAutowire() : DEFAULT_FULL_AUTOWIRE),
      m_suppressMedia(parentNLB ? parentNLB->isSuppressMedia() : DEFAULT_SUPPRESS_MEDIA),
      m_suppressSound(parentNLB ? parentNLB->isSuppressSound() : DEFAULT_SUPPRESS_SOUND) {
}

NonLinearBookImpl::NonLinearBookImpl(const NonLinearBook* source,
                                   NonLinearBook* parentNLB,
                                   Page* parentPage)
    : NonLinearBookImpl(parentNLB, parentPage) {
    append(source, false, false);
}

// Реализация методов из NonLinearBook
std::set<std::string> NonLinearBookImpl::getAllAchievementNames(bool recursive) const {
    std::set<std::string> result;
    
    // Собираем достижения из всех страниц
    for (const auto& [pageId, page] : m_pages) {
        // Логика сбора достижений из модификаций страниц
        auto modifications = page->getModifications();
        for (const auto& mod : modifications) {
            // Проверяем тип модификации на достижения
            // Это упрощенная логика, в реальности нужно проверять конкретные типы
        }
    }
    
    // Рекурсивно собираем из подмодулей если нужно
    if (recursive) {
        for (const auto& [pageId, page] : m_pages) {
            if (page->isModuleExternal()) {
                auto module = page->getModule();
                if (module && !module->isDummy()) {
                    auto moduleAchievements = module->getAllAchievementNames(true);
                    result.insert(moduleAchievements.begin(), moduleAchievements.end());
                }
            }
        }
    }
    
    return result;
}

std::string NonLinearBookImpl::getPerfectGameAchievementName() const {
    return m_perfectGameAchievementName;
}

bool NonLinearBookImpl::isEmpty() const {
    return m_pages.empty() && m_objs.empty() && m_variables.empty();
}

std::string NonLinearBookImpl::getStartPoint() const {
    return m_startPoint;
}

std::string NonLinearBookImpl::getLanguage() const {
    return m_language;
}

std::string NonLinearBookImpl::getLicense() const {
    return m_license;
}

Theme NonLinearBookImpl::getTheme() const {
    return m_theme;
}

bool NonLinearBookImpl::isFullAutowire() const {
    return m_fullAutowire;
}

bool NonLinearBookImpl::isSuppressMedia() const {
    return m_suppressMedia;
}

bool NonLinearBookImpl::isSuppressSound() const {
    return m_suppressSound;
}

std::string NonLinearBookImpl::getTitle() const {
    return m_title;
}

std::string NonLinearBookImpl::getAuthor() const {
    return m_author;
}

std::string NonLinearBookImpl::getVersion() const {
    return m_version;
}

std::string NonLinearBookImpl::getRootDir() const {
    return m_rootDir;
}

std::string NonLinearBookImpl::getImagesDir() const {
    return FileUtils::combinePath(m_rootDir, IMAGES_DIR_NAME);
}

std::set<std::string> NonLinearBookImpl::getUsedImages() const {
    return getUsedMediaFiles(MediaFile::Type::Image);
}

std::set<std::string> NonLinearBookImpl::getUsedSounds() const {
    return getUsedMediaFiles(MediaFile::Type::Sound);
}

const std::vector<MediaFile*>& NonLinearBookImpl::getImageFiles() const {
    return m_imageFiles;
}

const std::vector<MediaFile*>& NonLinearBookImpl::getSoundFiles() const {
    return m_soundFiles;
}

std::map<std::string, Page*> NonLinearBookImpl::getPages() const {
    std::map<std::string, Page*> result;
    for (const auto& [id, pageImpl] : m_pages) {
        result[id] = pageImpl;
    }
    return result;
}

std::map<std::string, Page*> NonLinearBookImpl::getDownwardPagesHeirarchy() const {
    std::map<std::string, Page*> result = getPages();
    
    // Рекурсивно добавляем страницы из подмодулей
    for (const auto& [pageId, page] : m_pages) {
        if (page->isModuleExternal()) {
            auto module = page->getModule();
            if (module && !module->isDummy()) {
                auto childPages = module->getDownwardPagesHeirarchy();
                result.insert(childPages.begin(), childPages.end());
            }
        }
    }
    
    return result;
}

std::map<std::string, Page*> NonLinearBookImpl::getUpwardPagesHeirarchy() const {
    std::map<std::string, Page*> result = getPages();
    
    // Добавляем страницы из родительских модулей
    if (m_parentNLB && !m_parentNLB->isDummy()) {
        auto parentPages = m_parentNLB->getUpwardPagesHeirarchy();
        result.insert(parentPages.begin(), parentPages.end());
    }
    
    return result;
}

std::vector<std::string> NonLinearBookImpl::getAutowiredPagesIds() const {
    return m_autowiredPages;
}

std::vector<std::string> NonLinearBookImpl::getParentGlobalAutowiredPagesIds() const {
    std::vector<std::string> result;
    
    if (m_parentNLB && !m_parentNLB->isDummy()) {
        // Собираем глобальные автопроводные страницы из родительского модуля
        auto parentAutowired = m_parentNLB->getAutowiredPagesIds();
        auto parentPages = m_parentNLB->getPages();
        
        for (const auto& pageId : parentAutowired) {
            auto it = parentPages.find(pageId);
            if (it != parentPages.end() && it->second->isGlobalAutowire()) {
                result.push_back(pageId);
            }
        }
        
        // Рекурсивно получаем из вышестоящих модулей
        auto grandParentAutowired = m_parentNLB->getParentGlobalAutowiredPagesIds();
        result.insert(result.end(), grandParentAutowired.begin(), grandParentAutowired.end());
    }
    
    return result;
}

bool NonLinearBookImpl::isAutowired(const std::string& pageId) const {
    return std::find(m_autowiredPages.begin(), m_autowiredPages.end(), pageId) != m_autowiredPages.end();
}

Page* NonLinearBookImpl::getPageById(const std::string& id) const {
    if (id.empty()) return nullptr;
    auto it = m_pages.find(id);
    return (it != m_pages.end()) ? it->second : nullptr;
}

std::map<std::string, Obj*> NonLinearBookImpl::getObjs() const {
    std::map<std::string, Obj*> result;
    for (const auto& [id, objImpl] : m_objs) {
        result[id] = objImpl;
    }
    return result;
}

Obj* NonLinearBookImpl::getObjById(const std::string& objId) const {
    auto it = m_objs.find(objId);
    return (it != m_objs.end()) ? it->second : nullptr;
}

void NonLinearBookImpl::exportMedia(bool recursively, const std::string& mediaDir,
                                  const std::string& exportDir,
                                  const std::vector<MediaFile*>& mediaFiles,
                                  MediaFile::Type type) const {
    // Создаем директорию экспорта если не существует
    if (!FileUtils::exists(exportDir)) {
        FileUtils::createDirectory(exportDir);
    }
    
    // Экспортируем медиафайлы
    for (const auto& mediaFile : mediaFiles) {
        std::string sourceFile = FileUtils::combinePath(mediaDir, mediaFile->getFileName());
        std::string targetFile = FileUtils::combinePath(exportDir, mediaFile->getFileName());
        
        if (FileUtils::exists(sourceFile)) {
            // Копируем файл
            std::ifstream source(sourceFile, std::ios::binary);
            std::ofstream target(targetFile, std::ios::binary);
            target << source.rdbuf();
        }
    }
    
    // Рекурсивный экспорт из подмодулей
    if (recursively) {
        for (const auto& [pageId, page] : m_pages) {
            if (page->isModuleExternal()) {
                auto module = page->getModule();
                if (module && !module->isDummy()) {
                    module->exportMedia(true, mediaDir, exportDir, 
                                      (type == MediaFile::Type::Image) ? module->getImageFiles() : module->getSoundFiles(),
                                      type);
                }
            }
        }
    }
}

Page* NonLinearBookImpl::createFilteredPage(const std::string& pageId,
                                                          const History& history) const {
    auto page = getPageById(pageId);
    if (!page) {
        return nullptr;
    }
    
    // Создаем фильтрованную копию страницы
    auto pageImpl = (PageImpl*) (page);
    
    // Определяем исключаемые объекты и ссылки на основе истории
    std::vector<std::string> objIdsToBeExcluded;
    std::vector<std::string> linkIdsToBeExcluded;
    std::vector<Link*> linksToBeAdded;
    std::map<std::string, void*> visitedVars;
    
    // Логика фильтрации на основе истории решений
    // Это упрощенная реализация - в оригинале более сложная логика
    
    return pageImpl->createFilteredCloneWithSubstitutions(
        objIdsToBeExcluded, linkIdsToBeExcluded, linksToBeAdded, visitedVars);
}

SearchResultTableModel NonLinearBookImpl::getVariables(const std::string& filter) const {
    SearchResultTableModel model({"Variable Name", "Type", "Data Type", "Value"});
    
    for (const auto& [varId, variable] : m_variables) {
        if (!variable->isDeleted()) {
            if (filter.empty() || variable->getName().find(filter) != std::string::npos) {
                SearchResult result(varId, m_parentPage ? m_parentPage->getId() : "", variable->getName());
                result.addInformation(Variable::typeToString(variable->getType()));
                result.addInformation(Variable::dataTypeToString(variable->getDataType()));
                result.addInformation(variable->getValue());
                model.addSearchResult(result);
            }
        }
    }
    
    return model;
}

SearchResultTableModel NonLinearBookImpl::getLeafs(const std::string& filter) const {
    SearchResultTableModel model({"Page Name"});
    
    for (const auto& [pageId, page] : m_pages) {
        if (!page->isDeleted() && page->isLeaf()) {
            if (filter.empty() || pageId.find(filter) != std::string::npos) {
                SearchResult result(pageId, m_parentPage ? m_parentPage->getId() : "", pageId);
                model.addSearchResult(result);
            }
        }
    }
    
    return model;
}

SearchResultTableModel NonLinearBookImpl::searchText(const SearchContract& contract,
                                                   const std::string& filter) const {
    SearchResultTableModel model({"Found In", "Context"});
    
    // Поиск в страницах
    if (contract.isSearchInPages()) {
        for (const auto& [pageId, page] : m_pages) {
            if (!page->isDeleted()) {
                auto searchResult = page->searchText(contract);
                if (searchResult) {
                    if (filter.empty() || pageId.find(filter) != std::string::npos) {
                        model.addSearchResult(*searchResult);
                    }
                }
            }
        }
    }
    
    // Поиск в объектах
    if (contract.isSearchInObjects()) {
        for (const auto& [objId, obj] : m_objs) {
            if (!obj->isDeleted()) {
                auto searchResult = obj->searchText(contract);
                if (searchResult) {
                    if (filter.empty() || objId.find(filter) != std::string::npos) {
                        model.addSearchResult(*searchResult);
                    }
                }
            }
        }
    }
    
    // Поиск в переменных
    if (contract.isSearchInVars()) {
        for (const auto& [varId, variable] : m_variables) {
            if (!variable->isDeleted()) {
                auto searchResult = variable->searchText(contract);
                if (searchResult) {
                    if (filter.empty() || varId.find(filter) != std::string::npos) {
                        model.addSearchResult(*searchResult);
                    }
                }
            }
        }
    }
    
    return model;
}

bool NonLinearBookImpl::findVariable(const std::string& variableName) const {
    for (const auto& [varId, variable] : m_variables) {
        if (!variable->isDeleted() && variable->getName() == variableName) {
            return true;
        }
    }
    return false;
}

SearchResultTableModel NonLinearBookImpl::checkBook(const std::string& filter) const {
    SearchResultTableModel model({"Issue Type", "Description"});
    
    // Проверка целостности ссылок
    for (const auto& [pageId, page] : m_pages) {
        if (!page->isDeleted()) {
            auto links = page->getLinks();
            for (const auto& link : links) {
                if (!link->isDeleted()) {
                    std::string target = link->getTarget();
                    if (!target.empty() && !getPageById(target) && !getObjById(target)) {
                        SearchResult result(pageId, m_parentPage ? m_parentPage->getId() : "", "Broken Link");
                        result.addInformation("Link target '" + target + "' not found");
                        model.addSearchResult(result);
                    }
                }
            }
        }
    }
    
    // Проверка переменных
    for (const auto& [varId, variable] : m_variables) {
        if (!variable->isDeleted()) {
            std::string target = variable->getTarget();
            if (!target.empty()) {
                bool targetExists = false;
                switch (variable->getType()) {
                    case Variable::Type::PAGE:
                        targetExists = (getPageById(target) != nullptr);
                        break;
                    case Variable::Type::OBJ:
                        targetExists = (getObjById(target) != nullptr);
                        break;
                    default:
                        targetExists = true; // Для других типов не проверяем
                        break;
                }
                
                if (!targetExists) {
                    SearchResult result(varId, m_parentPage ? m_parentPage->getId() : "", "Invalid Variable Reference");
                    result.addInformation("Variable target '" + target + "' not found");
                    model.addSearchResult(result);
                }
            }
        }
    }
    
    return model;
}

NonLinearBook::BookStatistics NonLinearBookImpl::getBookStatistics() const {
    BookStatistics stats;
    
    // Подсчитываем страницы
    for (const auto& [pageId, page] : m_pages) {
        if (!page->isDeleted()) {
            stats.incPagesCount(1);
            
            // Подсчитываем символы в тексте страницы
            std::string pageText = page->getText();
            stats.incCharactersCount(static_cast<int>(pageText.length()));
            
            // Проверяем, является ли страница концовкой
            if (page->isLeaf() && page->isFinish()) {
                stats.incUniqueEndings(1);
            }
        }
    }
    
    // Подсчитываем объекты
    for (const auto& [objId, obj] : m_objs) {
        if (!obj->isDeleted()) {
            stats.incObjsCount(1);
            
            // Подсчитываем символы в тексте объекта
            std::string objText = obj->getText();
            stats.incCharactersCount(static_cast<int>(objText.length()));
        }
    }
    
    // Добавляем информацию о модулях
    for (const auto& [pageId, page] : m_pages) {
        if (!page->isDeleted() && page->isModuleExternal()) {
            ModuleInfo moduleInfo(pageId, page->getModuleName(), 1);
            stats.addModuleInfo(moduleInfo);
            
            // Рекурсивно добавляем статистику подмодулей
            auto module = page->getModule();
            if (module && !module->isDummy()) {
                auto childStats = module->getBookStatistics();
                stats.addBookStatistics(childStats);
            }
        }
    }
    
    return stats;
}

NonLinearBook::VariableStatistics NonLinearBookImpl::getVariableStatistics() const {
    VariableStatistics stats;
    
    for (const auto& [varId, variable] : m_variables) {
        if (!variable->isDeleted()) {
            switch (variable->getType()) {
                case Variable::Type::PAGE:
                    stats.incPageVariablesCount();
                    break;
                case Variable::Type::TIMER:
                    stats.incPageTimerVariablesCount();
                    break;
                case Variable::Type::OBJ:
                    stats.incObjVariablesCount();
                    break;
                case Variable::Type::OBJCONSTRAINT:
                    stats.incObjConstraintsCount();
                    break;
                case Variable::Type::OBJREF:
                    stats.incObjRefsCount();
                    break;
                case Variable::Type::LINK:
                    stats.incLinkVariablesCount();
                    break;
                case Variable::Type::LINKCONSTRAINT:
                    stats.incLinkConstraintVariablesCount();
                    break;
                case Variable::Type::VAR:
                    stats.incPlainVariablesCount();
                    break;
                case Variable::Type::EXPRESSION:
                    stats.incExpressionsCount();
                    break;
                case Variable::Type::MODCONSTRAINT:
                    stats.incModuleConstraintCount();
                    break;
                case Variable::Type::AUTOWIRECONSTRAINT:
                    stats.incAutowireConstraintCount();
                    break;
            }
        }
    }
    
    return stats;
}

NonLinearBook* NonLinearBookImpl::getParentNLB() const {
    return m_parentNLB;
}

bool NonLinearBookImpl::isDummy() const {
    return false;
}

Page* NonLinearBookImpl::getParentPage() const {
    return m_parentPage;
}

std::map<std::string, NonLinearBook*> NonLinearBookImpl::getExternalModules() const {
    std::map<std::string, NonLinearBook*> result;
    
    for (const auto& [pageId, page] : m_pages) {
        if (!page->isDeleted() && page->isModuleExternal()) {
            auto module = page->getModule();
            if (module && !module->isDummy()) {
                result[pageId] = module;
            }
        }
    }
    
    return result;
}

NonLinearBook* NonLinearBookImpl::findExternalModule(const std::string& moduleId) const {
    // Ищем в текущем модуле
    auto modules = getExternalModules();
    auto it = modules.find(moduleId);
    if (it != modules.end()) {
        return it->second;
    }
    
    // Рекурсивно ищем в родительском модуле
    if (m_parentNLB && !m_parentNLB->isDummy()) {
        return m_parentNLB->findExternalModule(moduleId);
    }
    
    return nullptr;
}

std::map<std::string, Variable::DataType> NonLinearBookImpl::getVariableDataTypes() const {
    return m_variableDataTypes;
}

std::map<std::string, std::string> NonLinearBookImpl::getMediaToConstraintMap() const {
    return m_mediaToConstraintMap;
}

std::map<std::string, std::string> NonLinearBookImpl::getMediaRedirectsMap() const {
    return m_mediaRedirectsMap;
}

std::map<std::string, MediaExportParameters> NonLinearBookImpl::getMediaExportParametersMap() const {
    return m_mediaExportParametersMap;
}

std::map<std::string, bool> NonLinearBookImpl::getMediaFlagsMap() const {
    return m_mediaFlagsMap;
}

std::vector<Variable*> NonLinearBookImpl::getVariables() const {
    std::vector<Variable*> result;
    for (const auto& [varId, variable] : m_variables) {
        result.push_back(variable);
    }
    return result;
}

Variable* NonLinearBookImpl::getVariableById(const std::string& id) const {
    auto it = m_variables.find(id);
    return (it != m_variables.end()) ? it->second : nullptr;
}

void NonLinearBookImpl::save(FileManipulator* fileManipulator,
                           ProgressData* progressData,
                           PartialProgressData* partialProgressData) {
    // Если этот метод отсутствует, реализуйте его:
    writeNLB(fileManipulator, m_rootDir, partialProgressData);
    progressData->setProgressValue(90);
    progressData->setNoteText("Save completed");
}

bool NonLinearBookImpl::load(const std::string& path, ProgressData& progressData) {
    try {
        m_rootDir = path;
        readNLB(path, progressData);
        validateVariableReferences();
        resetVariableDataTypes();
        processAutowiredPages();
        return true;
    } catch (const std::exception& e) {
        clear();
        return false;
    }
}

void NonLinearBookImpl::clear() {
    m_pages.clear();
    m_objs.clear();
    m_variables.clear();
    m_imageFiles.clear();
    m_soundFiles.clear();
    m_autowiredPages.clear();
    m_mediaToConstraintMap.clear();
    m_mediaRedirectsMap.clear();
    m_mediaExportParametersMap.clear();
    m_mediaFlagsMap.clear();
    m_variableDataTypes.clear();
    m_rootDir.clear();
}

bool NonLinearBookImpl::loadAndSetParent(const std::string& path, 
                                        NonLinearBook* parentNLB,
                                        Page* parentPage) {
    m_parentNLB = parentNLB;
    m_parentPage = parentPage;
    DummyProgressData progressData;
    return load(path, progressData);
}

void NonLinearBookImpl::append(const NonLinearBook* source,
                              bool generateNewIds, bool overwriteTheme) {
    if (!source || source->isDummy()) {
        return;
    }
    
    // Копируем страницы
    auto sourcePages = source->getPages();
    for (const auto& [pageId, page] : sourcePages) {
        if (!page->isDeleted()) {
            // Создаем копию страницы
            auto newPage = new PageImpl(page, this, overwriteTheme);
            if (generateNewIds) {
                // Генерируем новый ID если нужно
                // newPage->setId(UUID::randomUUID());
            }
            m_pages[newPage->getId()] = newPage;
        }
    }
    
    // Копируем объекты
    auto sourceObjs = source->getObjs();
    for (const auto& [objId, obj] : sourceObjs) {
        if (!obj->isDeleted()) {
            auto newObj = new ObjImpl(obj, this);
            if (generateNewIds) {
                // newObj->setId(UUID::randomUUID());
            }
            m_objs[newObj->getId()] = newObj;
        }
    }
    
    // Копируем переменные
    auto sourceVars = source->getVariables();
    for (const auto& variable : sourceVars) {
        if (!variable->isDeleted()) {
            auto newVar = new VariableImpl(variable, this);
            if (generateNewIds) {
                // newVar->setId(UUID::randomUUID());
            }
            m_variables[newVar->getId()] = newVar;
        }
    }
}

// Методы экспорта (заглушки)
void NonLinearBookImpl::exportImages(bool isRoot, const std::string& exportDir) const {
    exportMedia(isRoot, getImagesDir(), exportDir, m_imageFiles, MediaFile::Type::Image);
}

void NonLinearBookImpl::exportSound(bool isRoot, const std::string& exportDir) const {
    exportMedia(isRoot, FileUtils::combinePath(m_rootDir, SOUND_DIR_NAME), exportDir, m_soundFiles, MediaFile::Type::Sound);
}

void NonLinearBookImpl::exportToChoiceScript(const std::string& exportDir) const {
    // Заглушка - экспорт в ChoiceScript формат
}

void NonLinearBookImpl::exportToQSPTextFile(const std::string& exportDir) const {
    // Заглушка - экспорт в QSP формат
}

void NonLinearBookImpl::exportToURQTextFile(const std::string& exportDir) const {
    // Заглушка - экспорт в URQ формат
}

void NonLinearBookImpl::exportToPDFFile(const std::string& exportFile) const {
    // Заглушка - экспорт в PDF
}

void NonLinearBookImpl::exportToTXTFile(const std::string& exportDir) const {
    // Заглушка - экспорт в TXT
}

void NonLinearBookImpl::exportToHTMLFile(const std::string& exportDir) const {
    // Заглушка - экспорт в HTML
}

void NonLinearBookImpl::exportToJSIQFile(const std::string& exportDir) const {
    // Заглушка - экспорт в JSIQ
}

void NonLinearBookImpl::exportToSTEADFile(const std::string& exportDir) const {
    // Заглушка - экспорт в STEAD
}

void NonLinearBookImpl::exportToVNSTEADFile(const std::string& exportDir) const {
    // Заглушка - экспорт в VNSTEAD  
}

void NonLinearBookImpl::exportToASMFile(const std::string& exportDir) const {
    // Заглушка - экспорт в ASM
}

// Методы работы с медиафайлами
void NonLinearBookImpl::setMediaFileConstrId(MediaFile::Type mediaType, const std::string& fileName, const std::string& constrId) {
    std::string key = (mediaType == MediaFile::Type::Image ? "img_" : "snd_") + fileName;
    m_mediaToConstraintMap[key] = constrId;
}

void NonLinearBookImpl::setMediaFileRedirect(MediaFile::Type mediaType, const std::string& fileName, const std::string& redirect) {
    std::string key = (mediaType == MediaFile::Type::Image ? "img_" : "snd_") + fileName;
    m_mediaRedirectsMap[key] = redirect;
}

void NonLinearBookImpl::setMediaFileFlag(MediaFile::Type mediaType, const std::string& fileName, bool flag) {
    std::string key = (mediaType == MediaFile::Type::Image ? "img_" : "snd_") + fileName;
    m_mediaFlagsMap[key] = flag;
}

void NonLinearBookImpl::setMediaFileExportParametersPreset(MediaFile::Type mediaType, const std::string& fileName, MediaExportParameters::Preset preset) {
    std::string key = (mediaType == MediaFile::Type::Image ? "img_" : "snd_") + fileName;
    m_mediaExportParametersMap[key] = MediaExportParameters::fromPreset(preset);
}

void NonLinearBookImpl::addImageFile(MediaFileImpl* imageFile) {
    if (imageFile) {
        m_imageFiles.push_back(imageFile);
    }
}

void NonLinearBookImpl::addSoundFile(MediaFileImpl* soundFile) {
    if (soundFile) {
        m_soundFiles.push_back(soundFile);
    }
}

MediaFileImpl* NonLinearBookImpl::copyMediaFile(
    FileManipulator& fileManipulator,
    const std::string& sourceFile,
    const std::string& fileName,
    const std::string& mediaDirName)
{
    // Создаем уникальное имя файла в медиа-директории
    std::string localFilePath = createUniqueMediaFile(fileManipulator, sourceFile, fileName, mediaDirName);
    
    // Копируем файл
    try {
        fileManipulator.copyFile(localFilePath, sourceFile, "Cannot copy media file " + FileUtils::getFileName(localFilePath));
    } catch (const std::exception& e) {
        throw NLBFileManipulationException("Failed to copy media file: " + std::string(e.what()));
    }
    
    // Создаем объект MediaFileImpl с именем скопированного файла
    auto mediaFile = new MediaFileImpl(FileUtils::getFileName(localFilePath));
    return mediaFile;
}

std::string NonLinearBookImpl::createUniqueMediaFile(
    FileManipulator& fileManipulator,
    const std::string& sourceFile,
    const std::string& fileName,
    const std::string& mediaDirName)
{
    // Определяем имя файла (переданное или из исходного файла)
    std::string uniqueFileName = fileName.empty() ? 
        StringHelper::toLowerCase(FileUtils::getFileName(sourceFile)) : 
        StringHelper::toLowerCase(fileName);
    
    // Создаем медиа-директорию
    std::string mediaDir = FileUtils::combinePath(m_rootDir, mediaDirName);
    fileManipulator.createDir(mediaDir, "Cannot create NLB media directory");
    
    // Формируем полный путь к файлу
    std::string localFilePath = FileUtils::combinePath(mediaDir, uniqueFileName);
    
    // Если файл уже существует, создаем уникальное имя
    if (FileUtils::exists(localFilePath)) {
        // Разделяем имя на части: имя + расширение
        size_t extIndex = uniqueFileName.find_last_of('.');
        std::string namePart = (extIndex != std::string::npos) ? 
            uniqueFileName.substr(0, extIndex) : uniqueFileName;
        std::string extPart = (extIndex != std::string::npos) ? 
            uniqueFileName.substr(extIndex) : "";
        
        // Ищем уникальное имя
        int counter = 1;
        do {
            uniqueFileName = namePart + "_" + std::to_string(counter++) + extPart;
            localFilePath = FileUtils::combinePath(mediaDir, uniqueFileName);
        } while (FileUtils::exists(localFilePath));
    }
    
    return localFilePath;
}

void NonLinearBookImpl::copyAndAddImageFile(
    const FileManipulator& fileManipulator,
    const std::string& imageFile,
    const std::string& imageFileName)
{
    // Создаем неконстантную копию fileManipulator для работы
    FileManipulator& fm = const_cast<FileManipulator&>(fileManipulator);
    
    try {
        // Копируем медиафайл
        auto mediaFile = copyMediaFile(fm, imageFile, imageFileName, IMAGES_DIR_NAME);
        
        // Добавляем в коллекцию изображений
        addImageFile((MediaFileImpl*) (mediaFile));
        
    } catch (const std::exception& e) {
        throw NLBIOException("Failed to copy and add image file: " + std::string(e.what()));
    }
}

void NonLinearBookImpl::copyAndAddSoundFile(
    const FileManipulator& fileManipulator,
    const std::string& soundFile,
    const std::string& soundFileName)
{
    // Создаем неконстантную копию fileManipulator для работы
    FileManipulator& fm = const_cast<FileManipulator&>(fileManipulator);
    
    try {
        // Копируем медиафайл
        auto mediaFile = copyMediaFile(fm, soundFile, soundFileName, SOUND_DIR_NAME);
        
        // Добавляем в коллекцию звуков
        addSoundFile((MediaFileImpl*) (mediaFile));
        
    } catch (const std::exception& e) {
        throw NLBIOException("Failed to copy and add sound file: " + std::string(e.what()));
    }
}

void NonLinearBookImpl::removeImageFile(
    const FileManipulator& fileManipulator,
    const std::string& imageFileName)
{
    // Создаем неконстантную копию fileManipulator для работы
    FileManipulator& fm = const_cast<FileManipulator&>(fileManipulator);
    
    // Ищем файл в коллекции изображений
    auto it = std::find_if(m_imageFiles.begin(), m_imageFiles.end(),
        [&imageFileName](const MediaFile* mediaFile) {
            return mediaFile->getFileName() == imageFileName;
        });
    
    if (it != m_imageFiles.end()) {
        // Проверяем существование директории изображений
        std::string imagesDir = FileUtils::combinePath(m_rootDir, IMAGES_DIR_NAME);
        if (!FileUtils::exists(imagesDir)) {
            throw NLBConsistencyException("NLB images dir does not exist");
        }
        
        try {
            // Удаляем основной файл
            std::string imageFilePath = FileUtils::combinePath(imagesDir, imageFileName);
            fm.deleteFileOrDir(imageFilePath);
            
            // Удаляем файлы метаданных (игнорируем ошибки, если они не существуют)
            try {
                std::string redirectFilePath = FileUtils::combinePath(imagesDir, imageFileName + REDIRECT_EXT);
                fm.deleteFileOrDir(redirectFilePath);
            } catch (...) {
                // Игнорируем ошибку - файл может не существовать
            }
            
            try {
                std::string constridFilePath = FileUtils::combinePath(imagesDir, imageFileName + CONSTRID_EXT);
                fm.deleteFileOrDir(constridFilePath);
            } catch (...) {
                // Игнорируем ошибку - файл может не существовать
            }
            
            try {
                std::string flagFilePath = FileUtils::combinePath(imagesDir, imageFileName + FLAG_EXT);
                fm.deleteFileOrDir(flagFilePath);
            } catch (...) {
                // Игнорируем ошибку - файл может не существовать
            }
            
            try {
                std::string presetFilePath = FileUtils::combinePath(imagesDir, imageFileName + PRESET_EXT);
                fm.deleteFileOrDir(presetFilePath);
            } catch (...) {
                // Игнорируем ошибку - файл может не существовать
            }
            
            // Удаляем из коллекции
            m_imageFiles.erase(it);
            
        } catch (const std::exception& e) {
            throw NLBFileManipulationException("Failed to delete image file: " + std::string(e.what()));
        }
    } else {
        throw NLBConsistencyException("Specified image file does not exist in images collection");
    }
}

void NonLinearBookImpl::removeSoundFile(
    const FileManipulator& fileManipulator,
    const std::string& soundFileName)
{
    // Создаем неконстантную копию fileManipulator для работы
    FileManipulator& fm = const_cast<FileManipulator&>(fileManipulator);
    
    // Ищем файл в коллекции звуков
    auto it = std::find_if(m_soundFiles.begin(), m_soundFiles.end(),
        [&soundFileName](const MediaFile* mediaFile) {
            return mediaFile->getFileName() == soundFileName;
        });
    
    if (it != m_soundFiles.end()) {
        // Проверяем существование директории звуков
        std::string soundDir = FileUtils::combinePath(m_rootDir, SOUND_DIR_NAME);
        if (!FileUtils::exists(soundDir)) {
            throw NLBConsistencyException("NLB sound dir does not exist");
        }
        
        try {
            // Удаляем основной файл
            std::string soundFilePath = FileUtils::combinePath(soundDir, soundFileName);
            fm.deleteFileOrDir(soundFilePath);
            
            // Удаляем файлы метаданных (игнорируем ошибки, если они не существуют)
            try {
                std::string redirectFilePath = FileUtils::combinePath(soundDir, soundFileName + REDIRECT_EXT);
                fm.deleteFileOrDir(redirectFilePath);
            } catch (...) {
                // Игнорируем ошибку - файл может не существовать
            }
            
            try {
                std::string constridFilePath = FileUtils::combinePath(soundDir, soundFileName + CONSTRID_EXT);
                fm.deleteFileOrDir(constridFilePath);
            } catch (...) {
                // Игнорируем ошибку - файл может не существовать
            }
            
            try {
                std::string flagFilePath = FileUtils::combinePath(soundDir, soundFileName + FLAG_EXT);
                fm.deleteFileOrDir(flagFilePath);
            } catch (...) {
                // Игнорируем ошибку - файл может не существовать
            }
            
            try {
                std::string presetFilePath = FileUtils::combinePath(soundDir, soundFileName + PRESET_EXT);
                fm.deleteFileOrDir(presetFilePath);
            } catch (...) {
                // Игнорируем ошибку - файл может не существовать
            }
            
            // Удаляем из коллекции
            m_soundFiles.erase(it);
            
        } catch (const std::exception& e) {
            throw NLBFileManipulationException("Failed to delete sound file: " + std::string(e.what()));
        }
    } else {
        throw NLBConsistencyException("Specified sound file does not exist in sound collection");
    }
}

// Методы получения реализаций
PageImpl* NonLinearBookImpl::getPageImplById(const std::string& id) const {
    auto it = m_pages.find(id);
    return (it != m_pages.end()) ? it->second : nullptr;
}

ObjImpl* NonLinearBookImpl::getObjImplById(const std::string& id) const {
    auto it = m_objs.find(id);
    return (it != m_objs.end()) ? it->second : nullptr;
}

VariableImpl* NonLinearBookImpl::getVariableImplById(const std::string& id) const {
    auto it = m_variables.find(id);
    return (it != m_variables.end()) ? it->second : nullptr;
}

std::vector<Link*> NonLinearBookImpl::getAssociatedLinks(NodeItem* nodeItem) {
    std::vector<Link*> result;
    
    // Ищем ссылки, которые ссылаются на данный узел
    for (const auto& [pageId, page] : m_pages) {
        if (!page->isDeleted()) {
            auto links = page->getLinks();
            for (const auto& link : links) {
                if (!link->isDeleted() && link->getTarget() == nodeItem->getId()) {
                    result.push_back(link);
                }
            }
        }
    }
    
    return result;
}

// Приватные методы
void NonLinearBookImpl::readBookProperties(const std::string& rootDir) {
    // Читаем основные свойства книги
    m_startPoint = FileManipulator::getOptionalFileAsString(rootDir, STARTPOINT_FILE_NAME, DEFAULT_STARTPOINT);
    m_language = FileManipulator::getOptionalFileAsString(rootDir, LANGUAGE_FILE_NAME, DEFAULT_LANGUAGE);
    m_license = FileManipulator::getOptionalFileAsString(rootDir, LICENSE_FILE_NAME, DEFAULT_LICENSE);
    m_title = FileManipulator::getOptionalFileAsString(rootDir, TITLE_FILE_NAME, DEFAULT_TITLE);
    m_author = FileManipulator::getOptionalFileAsString(rootDir, AUTHOR_FILE_NAME, DEFAULT_AUTHOR);
    m_version = FileManipulator::getOptionalFileAsString(rootDir, VERSION_FILE_NAME, DEFAULT_VERSION);
    m_perfectGameAchievementName = FileManipulator::getOptionalFileAsString(rootDir, PERFECTGAMEACHIEVEMENTNAME_FILE_NAME, DEFAULT_PERFECT_GAME_ACHIEVEMENT_NAME);
    
    // Читаем булевы свойства
    std::string fullAutowireStr = FileManipulator::getOptionalFileAsString(rootDir, FULL_AUTOWIRE_FILE_NAME, DEFAULT_FULL_AUTOWIRE ? "true" : "false");
    m_fullAutowire = (fullAutowireStr == "true");
    
    std::string suppressMediaStr = FileManipulator::getOptionalFileAsString(rootDir, SUPPRESS_MEDIA_FILE_NAME, DEFAULT_SUPPRESS_MEDIA ? "true" : "false");
    m_suppressMedia = (suppressMediaStr == "true");
    
    std::string suppressSoundStr = FileManipulator::getOptionalFileAsString(rootDir, SUPPRESS_SOUND_FILE_NAME, DEFAULT_SUPPRESS_SOUND ? "true" : "false");
    m_suppressSound = (suppressSoundStr == "true");
    
    // Читаем тему
    std::string themeStr = FileManipulator::getOptionalFileAsString(rootDir, THEME_FILE_NAME, ThemeUtils::toString(DEFAULT_THEME));
    m_theme = ThemeUtils::fromString(themeStr);
}

void NonLinearBookImpl::writeBookProperties(FileManipulator* fileManipulator, const std::string& rootDir) {
    // Записываем основные свойства книги
    fileManipulator->writeOptionalString(rootDir, STARTPOINT_FILE_NAME, m_startPoint, DEFAULT_STARTPOINT);
    fileManipulator->writeOptionalString(rootDir, LANGUAGE_FILE_NAME, m_language, DEFAULT_LANGUAGE);
    fileManipulator->writeOptionalString(rootDir, LICENSE_FILE_NAME, m_license, DEFAULT_LICENSE);
    fileManipulator->writeOptionalString(rootDir, TITLE_FILE_NAME, m_title, DEFAULT_TITLE);
    fileManipulator->writeOptionalString(rootDir, AUTHOR_FILE_NAME, m_author, DEFAULT_AUTHOR);
    fileManipulator->writeOptionalString(rootDir, VERSION_FILE_NAME, m_version, DEFAULT_VERSION);
    fileManipulator->writeOptionalString(rootDir, PERFECTGAMEACHIEVEMENTNAME_FILE_NAME, m_perfectGameAchievementName, DEFAULT_PERFECT_GAME_ACHIEVEMENT_NAME);
    
    // Записываем булевы свойства
    fileManipulator->writeOptionalString(rootDir, FULL_AUTOWIRE_FILE_NAME, 
        m_fullAutowire ? "true" : "false", DEFAULT_FULL_AUTOWIRE ? "true" : "false");
    fileManipulator->writeOptionalString(rootDir, SUPPRESS_MEDIA_FILE_NAME,
        m_suppressMedia ? "true" : "false", DEFAULT_SUPPRESS_MEDIA ? "true" : "false");
    fileManipulator->writeOptionalString(rootDir, SUPPRESS_SOUND_FILE_NAME,
        m_suppressSound ? "true" : "false", DEFAULT_SUPPRESS_SOUND ? "true" : "false");
    
    // Записываем тему
    fileManipulator->writeOptionalString(rootDir, THEME_FILE_NAME, 
        ThemeUtils::toString(m_theme), ThemeUtils::toString(DEFAULT_THEME));
}

void NonLinearBookImpl::loadPages(const std::string& rootDir, PartialProgressData* partialProgressData) {
    std::string pagesDir = FileUtils::combinePath(rootDir, PAGES_DIR_NAME);
    if (!FileUtils::exists(pagesDir)) {
        return;
    }
    
    // Читаем порядок страниц
    std::string pageOrderString = FileManipulator::getOptionalFileAsString(rootDir, PAGE_ORDER_FILE_NAME, "");
    std::vector<std::string> pageDirs = FileUtils::getDirectoryFiles(pagesDir);
    
    if (!pageOrderString.empty()) {
        std::vector<std::string> pageOrderList = StringHelper::tokenize(pageOrderString, PAGEORDER_SEPARATOR);
        pageDirs = createSortedDirList(pageDirs, pageOrderList, "pages");
    }
    
    // Загружаем страницы
    for (const std::string& pageDir : pageDirs) {
        std::string fullPageDir = FileUtils::combinePath(pagesDir, pageDir);
        auto page = new PageImpl(this);
        page->readPage(fullPageDir);
        m_pages[page->getId()] = page;
        
        if (partialProgressData) {
            partialProgressData->setRealProgressValue();
        }
    }
}

void NonLinearBookImpl::loadObjs(const std::string& rootDir, PartialProgressData* partialProgressData) {
    std::string objsDir = FileUtils::combinePath(rootDir, OBJECTS_DIR_NAME);
    if (!FileUtils::exists(objsDir)) {
        return;
    }
    
    // Читаем порядок объектов
    std::string objOrderString = FileManipulator::getOptionalFileAsString(rootDir, OBJ_ORDER_FILE_NAME, "");
    std::vector<std::string> objDirs = FileUtils::getDirectoryFiles(objsDir);
    
    if (!objOrderString.empty()) {
        std::vector<std::string> objOrderList = StringHelper::tokenize(objOrderString, OBJORDER_SEPARATOR);
        objDirs = createSortedDirList(objDirs, objOrderList, "objects");
    }
    
    // Загружаем объекты
    for (const std::string& objDir : objDirs) {
        std::string fullObjDir = FileUtils::combinePath(objsDir, objDir);
        auto obj = new ObjImpl(this);
        obj->readObj(fullObjDir);
        m_objs[obj->getId()] = obj;
        
        if (partialProgressData) {
            partialProgressData->setRealProgressValue();
        }
    }
}

void NonLinearBookImpl::loadVariables(const std::string& rootDir, PartialProgressData* partialProgressData) {
    std::string varsDir = FileUtils::combinePath(rootDir, VARIABLES_DIR_NAME);
    if (!FileUtils::exists(varsDir)) {
        return;
    }
    
    // Читаем порядок переменных
    std::string varOrderString = FileManipulator::getOptionalFileAsString(rootDir, VAR_ORDER_FILE_NAME, "");
    std::vector<std::string> varDirs = FileUtils::getDirectoryFiles(varsDir);
    
    if (!varOrderString.empty()) {
        std::vector<std::string> varOrderList = StringHelper::tokenize(varOrderString, VARORDER_SEPARATOR);
        varDirs = createSortedDirList(varDirs, varOrderList, "variables");
    }
    
    // Загружаем переменные
    for (const std::string& varDir : varDirs) {
        std::string fullVarDir = FileUtils::combinePath(varsDir, varDir);
        auto variable = new VariableImpl(this);
        variable->readVariable(fullVarDir);
        m_variables[variable->getId()] = variable;
        
        if (partialProgressData) {
            partialProgressData->setRealProgressValue();
        }
    }
}

void NonLinearBookImpl::loadMediaFiles(const std::string& rootDir, const std::string& mediaDirName, std::vector<MediaFile*>& mediaFiles) {
    std::string mediaDir = FileUtils::combinePath(rootDir, mediaDirName);
    if (!FileUtils::exists(mediaDir)) {
        return;
    }
    
    std::vector<std::string> files = FileUtils::getDirectoryFiles(mediaDir);
    for (const std::string& fileName : files) {
        std::string fullPath = FileUtils::combinePath(mediaDir, fileName);
        if (!FileUtils::isDirectory(fullPath)) {
            auto mediaFile = new MediaFileImpl(fileName);
            mediaFiles.push_back(mediaFile);
        }
    }
}

void NonLinearBookImpl::writeMediaFiles(FileManipulator* fileManipulator, const std::string& rootDir, const std::vector<MediaFile*>& mediaFiles, const std::string& mediaDirName) {
    if (mediaFiles.empty()) {
        return;
    }
    
    std::string mediaDir = FileUtils::combinePath(rootDir, mediaDirName);
    fileManipulator->createDir(mediaDir, "Cannot create media directory: " + mediaDir);
    
    // Записываем медиафайлы (в реальности тут должно быть копирование файлов)
    for (const auto& mediaFile : mediaFiles) {
        // Логика копирования медиафайлов
    }
}

void NonLinearBookImpl::writePages(FileManipulator* fileManipulator, const std::string& rootDir, PartialProgressData* partialProgressData) {
    if (m_pages.empty()) {
        return;
    }
    
    std::string pagesDir = FileUtils::combinePath(rootDir, PAGES_DIR_NAME);
    fileManipulator->createDir(pagesDir, "Cannot create pages directory");
    
    for (const auto& [pageId, page] : m_pages) {
        page->writePage(fileManipulator, pagesDir, this, partialProgressData);
        if (partialProgressData) {
            partialProgressData->setRealProgressValue();
        }
    }
}

void NonLinearBookImpl::writeObjs(FileManipulator* fileManipulator, const std::string& rootDir, PartialProgressData* partialProgressData) {
    if (m_objs.empty()) {
        return;
    }
    
    std::string objsDir = FileUtils::combinePath(rootDir, OBJECTS_DIR_NAME);
    fileManipulator->createDir(objsDir, "Cannot create objects directory");
    
    for (const auto& [objId, obj] : m_objs) {
        obj->writeObj(fileManipulator, objsDir, this);
        if (partialProgressData) {
            partialProgressData->setRealProgressValue();
        }
    }
}

void NonLinearBookImpl::writeVariables(FileManipulator* fileManipulator, const std::string& rootDir, PartialProgressData* partialProgressData) {
    if (m_variables.empty()) {
        return;
    }
    
    std::string varsDir = FileUtils::combinePath(rootDir, VARIABLES_DIR_NAME);
    fileManipulator->createDir(varsDir, "Cannot create variables directory");
    
    for (const auto& [varId, variable] : m_variables) {
        variable->writeVariable(*fileManipulator, varsDir);
        if (partialProgressData) {
            partialProgressData->setRealProgressValue();
        }
    }
}

void NonLinearBookImpl::writePageOrderFile(FileManipulator* fileManipulator, const std::string& rootDir) {
    if (m_pages.empty()) {
        return;
    }
    
    std::string content;
    bool first = true;
    for (const auto& [pageId, page] : m_pages) {
        if (!page->isDeleted()) {
            if (first) {
                first = false;
            } else {
                content += PAGEORDER_SEPARATOR;
            }
            content += pageId;
        }
    }
    
    fileManipulator->writeOptionalString(rootDir, PAGE_ORDER_FILE_NAME, content, "");
}

void NonLinearBookImpl::writeObjOrderFile(FileManipulator* fileManipulator, const std::string& rootDir) {
    if (m_objs.empty()) {
        return;
    }
    
    std::string content;
    bool first = true;
    for (const auto& [objId, obj] : m_objs) {
        if (!obj->isDeleted()) {
            if (first) {
                first = false;
            } else {
                content += OBJORDER_SEPARATOR;
            }
            content += objId;
        }
    }
    
    fileManipulator->writeOptionalString(rootDir, OBJ_ORDER_FILE_NAME, content, "");
}

void NonLinearBookImpl::writeVarOrderFile(FileManipulator* fileManipulator, const std::string& rootDir) {
    if (m_variables.empty()) {
        return;
    }
    
    std::string content;
    bool first = true;
    for (const auto& [varId, variable] : m_variables) {
        if (!variable->isDeleted()) {
            if (first) {
                first = false;
            } else {
                content += VARORDER_SEPARATOR;
            }
            content += varId;
        }
    }
    
    fileManipulator->writeOptionalString(rootDir, VAR_ORDER_FILE_NAME, content, "");
}

void NonLinearBookImpl::writeAutowiredPagesFile(FileManipulator* fileManipulator, const std::string& rootDir) {
    std::string content;
    bool first = true;
    for (const std::string& pageId : m_autowiredPages) {
        if (first) {
            first = false;
        } else {
            content += AUTOWIRED_PAGES_SEPARATOR;
        }
        content += pageId;
    }
    
    fileManipulator->writeOptionalString(rootDir, AUTOWIRED_PAGES_FILE_NAME, content, "");
}

std::vector<std::string> NonLinearBookImpl::createSortedDirList(const std::vector<std::string>& dirs, const std::vector<std::string>& orderList, const std::string& entityName) {
    std::vector<std::string> result;
    std::vector<std::string> remainingDirs = dirs;
    
    // Сначала добавляем в порядке из orderList
    for (const std::string& orderedDir : orderList) {
        auto it = std::find(remainingDirs.begin(), remainingDirs.end(), orderedDir);
        if (it != remainingDirs.end()) {
            result.push_back(*it);
            remainingDirs.erase(it);
        } else {
            throw NLBConsistencyException("Inconsistent NLB structure: cannot locate " + entityName + " directory with name = " + orderedDir);
        }
    }
    
    // Добавляем оставшиеся директории
    if (!remainingDirs.empty()) {
        throw NLBConsistencyException("Inconsistent NLB structure: " + entityName + " directories not found in order file");
    }
    
    return result;
}

void NonLinearBookImpl::validateVariableReferences() {
    // Проверяем ссылки переменных на существующие объекты
    for (const auto& [varId, variable] : m_variables) {
        if (!variable->isDeleted()) {
            std::string target = variable->getTarget();
            if (!target.empty()) {
                switch (variable->getType()) {
                    case Variable::Type::PAGE:
                        if (!getPageById(target)) {
                            // Логируем предупреждение о несуществующей странице
                        }
                        break;
                    case Variable::Type::OBJ:
                        if (!getObjById(target)) {
                            // Логируем предупреждение о несуществующем объекте
                        }
                        break;
                    default:
                        // Для других типов проверка не требуется
                        break;
                }
            }
        }
    }
}

void NonLinearBookImpl::resetVariableDataTypes() {
    // Сбрасываем типы данных переменных
    m_variableDataTypes.clear();
    
    for (const auto& [varId, variable] : m_variables) {
        if (!variable->isDeleted()) {
            m_variableDataTypes[varId] = variable->getDataType();
        }
    }
}

void NonLinearBookImpl::processAutowiredPages() {
    // Обрабатываем автопроводные страницы
    std::string autowiredPagesString = FileManipulator::getOptionalFileAsString(m_rootDir, AUTOWIRED_PAGES_FILE_NAME, "");
    
    if (!autowiredPagesString.empty()) {
        m_autowiredPages = StringHelper::tokenize(autowiredPagesString, AUTOWIRED_PAGES_SEPARATOR);
    }
}

std::set<std::string> NonLinearBookImpl::getUsedMediaFiles(MediaFile::Type mediaType) const {
    std::set<std::string> result;
    
    // Собираем используемые медиафайлы из страниц
    for (const auto& [pageId, page] : m_pages) {
        if (!page->isDeleted()) {
            if (mediaType == MediaFile::Type::Image) {
                addUsedImages(result, page->getImageFileName());
            } else {
                addUsedSounds(result, page->getSoundFileName());
            }
        }
    }
    
    // Собираем используемые медиафайлы из объектов
    for (const auto& [objId, obj] : m_objs) {
        if (!obj->isDeleted()) {
            if (mediaType == MediaFile::Type::Image) {
                addUsedImages(result, obj->getImageFileName());
            } else {
                addUsedSounds(result, obj->getSoundFileName());
            }
        }
    }
    
    return result;
}

void NonLinearBookImpl::addUsedImages(std::set<std::string>& usedImages, const std::string& imageFileName) const {
    if (!imageFileName.empty() && imageFileName != nlb::Constants::EMPTY_STRING) {
        usedImages.insert(imageFileName);
    }
}

void NonLinearBookImpl::addUsedSounds(std::set<std::string>& usedSounds, const std::string& soundFileName) const {
    if (!soundFileName.empty() && soundFileName != nlb::Constants::EMPTY_STRING) {
        usedSounds.insert(soundFileName);
    }
}

void NonLinearBookImpl::writeNLB(FileManipulator* fileManipulator, const std::string& nlbDir, PartialProgressData* partialProgressData) {
    // Записываем свойства книги
    writeBookProperties(fileManipulator, nlbDir);
    
    // Записываем страницы
    writePages(fileManipulator, nlbDir, partialProgressData);
    writePageOrderFile(fileManipulator, nlbDir);
    
    // Записываем объекты
    writeObjs(fileManipulator, nlbDir, partialProgressData);
    writeObjOrderFile(fileManipulator, nlbDir);
    
    // Записываем переменные
    writeVariables(fileManipulator, nlbDir, partialProgressData);
    writeVarOrderFile(fileManipulator, nlbDir);
    
    // Записываем автопроводные страницы
    writeAutowiredPagesFile(fileManipulator, nlbDir);
    
    // Записываем медиафайлы
    writeMediaFiles(fileManipulator, nlbDir, m_imageFiles, IMAGES_DIR_NAME);
    writeMediaFiles(fileManipulator, nlbDir, m_soundFiles, SOUND_DIR_NAME);
}

void NonLinearBookImpl::readNLB(const std::string& nlbDir, ProgressData& progressData) {
    // Читаем свойства книги
    readBookProperties(nlbDir);
    
    // Создаем заглушку для прогресса
    auto partialProgress = new PartialProgressData(&progressData, 0, 100, 1);
    
    // Читаем страницы
    loadPages(nlbDir, partialProgress);
    
    // Читаем объекты
    loadObjs(nlbDir, partialProgress);
    
    // Читаем переменные
    loadVariables(nlbDir, partialProgress);
    
    // Читаем медиафайлы
    loadMediaFiles(nlbDir, IMAGES_DIR_NAME, m_imageFiles);
    loadMediaFiles(nlbDir, SOUND_DIR_NAME, m_soundFiles);
}

NonLinearBookImpl::AddPageCommand::AddPageCommand(NonLinearBookImpl* nlb, PageImpl* page)
    : m_nlb(nlb), m_page(page) {
}

void NonLinearBookImpl::AddPageCommand::execute() {
    // Добавляем страницу в коллекцию
    m_nlb->m_pages[m_page->getId()] = m_page;
    m_page->setDeleted(false);
    
    // Уведомляем наблюдателей
    m_page->notifyObservers();
}

void NonLinearBookImpl::AddPageCommand::revert() {
    // Удаляем страницу из коллекции
    m_nlb->m_pages.erase(m_page->getId());
    m_page->setDeleted(true);
    
    // Уведомляем наблюдателей
    m_page->notifyObservers();
}

// AddObjCommand
NonLinearBookImpl::AddObjCommand::AddObjCommand(NonLinearBookImpl* nlb, ObjImpl* obj)
    : m_nlb(nlb), m_obj(obj) {
}

void NonLinearBookImpl::AddObjCommand::execute() {
    // Добавляем объект в коллекцию
    m_nlb->m_objs[m_obj->getId()] = m_obj;
    m_obj->setDeleted(false);
    
    // Уведомляем наблюдателей
    m_obj->notifyObservers();
}

void NonLinearBookImpl::AddObjCommand::revert() {
    // Удаляем объект из коллекции
    m_nlb->m_objs.erase(m_obj->getId());
    m_obj->setDeleted(true);
    
    // Уведомляем наблюдателей
    m_obj->notifyObservers();
}

// DeletePageCommand
NonLinearBookImpl::DeletePageCommand::DeletePageCommand(NonLinearBookImpl* nlb, 
                                                       PageImpl* page,
                                                       const std::vector<Link*>& adjacentLinks)
    : m_nlb(nlb), m_page(page), m_adjacentLinks(adjacentLinks), m_wasDeleted(page->isDeleted()) {
}

void NonLinearBookImpl::DeletePageCommand::execute() {
    // Помечаем страницу как удаленную
    m_page->setDeleted(true);
    
    // Удаляем все связанные ссылки
    for (auto& link : m_adjacentLinks) {
        ((LinkImpl*) (link))->setDeleted(true);
        link->notifyObservers();
    }
    
    // Уведомляем наблюдателей
    m_page->notifyObservers();
}

void NonLinearBookImpl::DeletePageCommand::revert() {
    // Восстанавливаем страницу
    m_page->setDeleted(m_wasDeleted);
    
    // Восстанавливаем все связанные ссылки
    for (auto& link : m_adjacentLinks) {
        ((LinkImpl*) (link))->setDeleted(false);
        link->notifyObservers();
    }
    
    // Уведомляем наблюдателей
    m_page->notifyObservers();
}

// DeleteObjCommand
NonLinearBookImpl::DeleteObjCommand::DeleteObjCommand(NonLinearBookImpl* nlb,
                                                     ObjImpl* obj,
                                                     const std::vector<Link*>& adjacentLinks)
    : m_nlb(nlb), m_obj(obj), m_adjacentLinks(adjacentLinks), m_wasDeleted(obj->isDeleted()) {
}

void NonLinearBookImpl::DeleteObjCommand::execute() {
    // Помечаем объект как удаленный
    m_obj->setDeleted(true);
    
    // Удаляем все связанные ссылки
    for (auto& link : m_adjacentLinks) {
        ((LinkImpl*) (link))->setDeleted(true);
        link->notifyObservers();
    }
    
    // Уведомляем наблюдателей
    m_obj->notifyObservers();
}

void NonLinearBookImpl::DeleteObjCommand::revert() {
    // Восстанавливаем объект
    m_obj->setDeleted(m_wasDeleted);
    
    // Восстанавливаем все связанные ссылки
    for (auto& link : m_adjacentLinks) {
        ((LinkImpl*) (link))->setDeleted(false);
        link->notifyObservers();
    }
    
    // Уведомляем наблюдателей
    m_obj->notifyObservers();
}

// CopyCommand
NonLinearBookImpl::CopyCommand::CopyCommand(const std::vector<std::string>& pageIds,
                                          const std::vector<std::string>& objIds)
    : m_pageIds(pageIds), m_objIds(objIds) {
}

void NonLinearBookImpl::CopyCommand::execute() {
    // Создаем временную книгу для копирования
    auto clipboard = new NonLinearBookImpl();
    
    // TODO: Реализовать копирование страниц и объектов в clipboard
    // Это сложная операция, которая требует клонирования элементов
    
    // Сохраняем в глобальный буфер обмена
    Clipboard::singleton().setNonLinearBook(clipboard);
}

void NonLinearBookImpl::CopyCommand::revert() {
    // Копирование необратимо - ничего не делаем
    // В Java версии этот метод тоже пустой
}

// PasteCommand
NonLinearBookImpl::PasteCommand::PasteCommand(NonLinearBookImpl* nlb,
                                             NonLinearBookImpl* nlbToPaste)
    : m_nlb(nlb), m_nlbToPaste(nlbToPaste) {
}

void NonLinearBookImpl::PasteCommand::execute() {
    if (!m_nlbToPaste) {
        return;
    }
    
    // Добавляем страницы из буфера обмена
    auto pages = m_nlbToPaste->getPages();
    for (const auto& [pageId, page] : pages) {
        if (!page->isDeleted()) {
            auto newPage = new PageImpl(page, m_nlb, true);
            // Генерируем новый ID для избежания конфликтов
            std::string newId = NLBUUID::randomUUID();
            newPage->setId(newId);
            m_nlb->m_pages[newId] = newPage;
            m_addedPageIds.push_back(newId);
        }
    }
    
    // Добавляем объекты из буфера обмена
    auto objs = m_nlbToPaste->getObjs();
    for (const auto& [objId, obj] : objs) {
        if (!obj->isDeleted()) {
            auto newObj = new ObjImpl(obj, m_nlb);
            // Генерируем новый ID для избежания конфликтов
            std::string newId = NLBUUID::randomUUID();
            newObj->setId(newId);
            m_nlb->m_objs[newId] = newObj;
            m_addedObjIds.push_back(newId);
        }
    }
}

void NonLinearBookImpl::PasteCommand::revert() {
    // Удаляем добавленные страницы
    for (const auto& pageId : m_addedPageIds) {
        m_nlb->m_pages.erase(pageId);
    }
    
    // Удаляем добавленные объекты
    for (const auto& objId : m_addedObjIds) {
        m_nlb->m_objs.erase(objId);
    }
}

// ChangeStartPointCommand
NonLinearBookImpl::ChangeStartPointCommand::ChangeStartPointCommand(NonLinearBookImpl* nlb,
                                                                   const std::string& startPoint)
    : m_nlb(nlb), m_newStartPoint(startPoint), m_oldStartPoint(nlb->getStartPoint()) {
}

void NonLinearBookImpl::ChangeStartPointCommand::execute() {
    m_nlb->setStartPoint(m_newStartPoint);
}

void NonLinearBookImpl::ChangeStartPointCommand::revert() {
    m_nlb->setStartPoint(m_oldStartPoint);
}

// UpdateBookPropertiesCommand
NonLinearBookImpl::UpdateBookPropertiesCommand::UpdateBookPropertiesCommand(
    NonLinearBookImpl* nlb,
    const std::string& license, Theme theme,
    const std::string& language, const std::string& title,
    const std::string& author, const std::string& version,
    const std::string& perfectGameAchievementName,
    bool fullAutowire, bool suppressMedia, bool suppressSound,
    bool propagateToSubmodules)
    : m_nlb(nlb)
    , m_newLicense(license), m_oldLicense(nlb->getLicense())
    , m_newTheme(theme), m_oldTheme(nlb->getTheme())
    , m_newLanguage(language), m_oldLanguage(nlb->getLanguage())
    , m_newTitle(title), m_oldTitle(nlb->getTitle())
    , m_newAuthor(author), m_oldAuthor(nlb->getAuthor())
    , m_newVersion(version), m_oldVersion(nlb->getVersion())
    , m_newPerfectGameAchievementName(perfectGameAchievementName)
    , m_oldPerfectGameAchievementName(nlb->getPerfectGameAchievementName())
    , m_newFullAutowire(fullAutowire), m_oldFullAutowire(nlb->isFullAutowire())
    , m_newSuppressMedia(suppressMedia), m_oldSuppressMedia(nlb->isSuppressMedia())
    , m_newSuppressSound(suppressSound), m_oldSuppressSound(nlb->isSuppressSound())
    , m_propagateToSubmodules(propagateToSubmodules) {
}

void NonLinearBookImpl::UpdateBookPropertiesCommand::execute() {
    m_nlb->setLicense(m_newLicense);
    m_nlb->setTheme(m_newTheme);
    m_nlb->setLanguage(m_newLanguage);
    m_nlb->setTitle(m_newTitle);
    m_nlb->setAuthor(m_newAuthor);
    m_nlb->setVersion(m_newVersion);
    m_nlb->setPerfectGameAchievementName(m_newPerfectGameAchievementName);
    m_nlb->setFullAutowire(m_newFullAutowire);
    m_nlb->setSuppressMedia(m_newSuppressMedia);
    m_nlb->setSuppressSound(m_newSuppressSound);
    
    // TODO: Если propagateToSubmodules = true, применить изменения к подмодулям
}

void NonLinearBookImpl::UpdateBookPropertiesCommand::revert() {
    m_nlb->setLicense(m_oldLicense);
    m_nlb->setTheme(m_oldTheme);
    m_nlb->setLanguage(m_oldLanguage);
    m_nlb->setTitle(m_oldTitle);
    m_nlb->setAuthor(m_oldAuthor);
    m_nlb->setVersion(m_oldVersion);
    m_nlb->setPerfectGameAchievementName(m_oldPerfectGameAchievementName);
    m_nlb->setFullAutowire(m_oldFullAutowire);
    m_nlb->setSuppressMedia(m_oldSuppressMedia);
    m_nlb->setSuppressSound(m_oldSuppressSound);
}

// UpdateModificationsCommand
NonLinearBookImpl::UpdateModificationsCommand::UpdateModificationsCommand(
    ModifyingItem* modifyingItem,
    ModificationsTableModel* modificationsTableModel)
    : m_modifyingItem(modifyingItem), m_modificationsTableModel(modificationsTableModel) {
    
    // Сохраняем текущие модификации для отката
    auto currentMods = modifyingItem->getModifications();
    for (const auto& mod : currentMods) {
        m_oldModifications.push_back((ModificationImpl*) (mod));
    }
}

void NonLinearBookImpl::UpdateModificationsCommand::execute() {
    // TODO: Применить модификации из table model к modifying item
    // Это требует доступа к внутренней структуре ModificationsTableModel
}

void NonLinearBookImpl::UpdateModificationsCommand::revert() {
    // TODO: Восстановить старые модификации
}

NLBCommand* NonLinearBookImpl::createAddPageCommand(const PageImpl* pageImpl) {
    return new AddPageCommand(this, (PageImpl*) pageImpl);
}

NLBCommand* NonLinearBookImpl::createAddObjCommand(const ObjImpl* objImpl) {
    return new AddObjCommand(this, (ObjImpl*) objImpl);
}

NLBCommand* NonLinearBookImpl::createDeletePageCommand(
    PageImpl* page,
    const std::vector<Link*>& adjacentLinks) {
    return new DeletePageCommand(this, page, adjacentLinks);
}

NLBCommand* NonLinearBookImpl::createDeleteObjCommand(
    ObjImpl* obj,
    const std::vector<Link*>& adjacentLinks) {
    return new DeleteObjCommand(this, obj, adjacentLinks);
}

NLBCommand* NonLinearBookImpl::createCopyCommand(
    const std::vector<std::string>& pageIds, 
    const std::vector<std::string>& objIds) {
    return new CopyCommand(pageIds, objIds);
}

NLBCommand* NonLinearBookImpl::createDeleteCommand(const std::vector<std::string>& pageIds,
    const std::vector<std::string>& objIds)
{
    // TODO: implement
    return nullptr;
}

NLBCommand* NonLinearBookImpl::createPasteCommand(
    NonLinearBookImpl* nlbToPaste) {
    return new PasteCommand(this, nlbToPaste);
}

NLBCommand* NonLinearBookImpl::createChangeStartPointCommand(
    const std::string& startPoint) {
    return new ChangeStartPointCommand(this, startPoint);
}

NLBCommand* NonLinearBookImpl::createUpdateBookPropertiesCommand(
    const std::string& license, Theme theme,
    const std::string& language, const std::string& title,
    const std::string& author, const std::string& version,
    const std::string& perfectGameAchievementName,
    bool fullAutowire, bool suppressMedia, bool suppressSound,
    bool propagateToSubmodules) {
    return new UpdateBookPropertiesCommand(
        this, license, theme, language, title, author, version,
        perfectGameAchievementName, fullAutowire, suppressMedia, 
        suppressSound, propagateToSubmodules);
}

NLBCommand* NonLinearBookImpl::createUpdatePageCommand(Page* page,
    const std::string& imageFileName, bool imageBackground, bool imageAnimated, const std::string& soundFileName,
    bool soundSFX, const std::string& pageVariableName, const std::string& pageTimerVariableName,
    const std::string& pageDefTagVariableValue, MultiLangString* pageText,
    MultiLangString* pageCaptionText, Theme theme, bool useCaption, bool useMPL,
    const std::string& moduleName, bool moduleExternal, MultiLangString* traverseText,
    bool autoTraverse, bool autoReturn, MultiLangString* returnText, const std::string& returnPageId,
    const std::string& moduleConsraintVariableName, bool autowire, MultiLangString* autowireInText,
    MultiLangString* autowireOutText, bool autoIn, bool needsAction, bool autoOut,
    const std::string& autowireInConstraint, const std::string& autowireOutConstraint, bool globalAutowire, bool noSave,
    bool autosFirst, LinksTableModel* linksTableModel)
{
    // TODO: implement
    return nullptr;
}

NLBCommand* NonLinearBookImpl::createUpdateModificationsCommand(
    ModifyingItem* modifyingItem,
    ModificationsTableModel* modificationsTableModel) {
    return new UpdateModificationsCommand(modifyingItem, modificationsTableModel);
}

void NonLinearBookImpl::setStartPoint(const std::string& startPoint) {
    m_startPoint = startPoint;
}

void NonLinearBookImpl::setLicense(const std::string& license) {
    m_license = license;
}

void NonLinearBookImpl::setTheme(Theme theme) {
    m_theme = theme;
}

void NonLinearBookImpl::setLanguage(const std::string& language) {
    m_language = language;
}

void NonLinearBookImpl::setTitle(const std::string& title) {
    m_title = title;
}

void NonLinearBookImpl::setAuthor(const std::string& author) {
    m_author = author;
}

void NonLinearBookImpl::setVersion(const std::string& version) {
    m_version = version;
}

void NonLinearBookImpl::setPerfectGameAchievementName(const std::string& name) {
    m_perfectGameAchievementName = name;
}

void NonLinearBookImpl::setFullAutowire(bool fullAutowire) {
    m_fullAutowire = fullAutowire;
}

void NonLinearBookImpl::setSuppressMedia(bool suppressMedia) {
    m_suppressMedia = suppressMedia;
}

void NonLinearBookImpl::setSuppressSound(bool suppressSound) {
    m_suppressSound = suppressSound;
}

void NonLinearBookImpl::setRootDir(const std::string& rootDir) {
    m_rootDir = rootDir;
}

// Метод для получения эффективного количества страниц для сохранения
int NonLinearBookImpl::getEffectivePagesCountForSave() const {
    int count = 0;
    for (const auto& [pageId, page] : m_pages) {
        if (!page->isDeleted()) {
            count++;
        }
    }
    return count;
}
