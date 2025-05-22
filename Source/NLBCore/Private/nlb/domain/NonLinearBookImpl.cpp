#include "nlb/domain/NonLinearBookImpl.h"
#include "nlb/domain/MediaExportParameters.h"
#include "nlb/domain/PageImpl.h"
#include "nlb/domain/ObjImpl.h"
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
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <algorithm>

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

NonLinearBookImpl::NonLinearBookImpl(std::shared_ptr<NonLinearBook> parentNLB, std::shared_ptr<Page> parentPage)
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

NonLinearBookImpl::NonLinearBookImpl(const std::shared_ptr<NonLinearBook>& source, 
                                   std::shared_ptr<NonLinearBook> parentNLB, 
                                   std::shared_ptr<Page> parentPage) 
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

const std::vector<MediaFile>& NonLinearBookImpl::getImageFiles() const {
    return m_imageFiles;
}

const std::vector<MediaFile>& NonLinearBookImpl::getSoundFiles() const {
    return m_soundFiles;
}

std::map<std::string, std::shared_ptr<Page>> NonLinearBookImpl::getPages() const {
    std::map<std::string, std::shared_ptr<Page>> result;
    for (const auto& [id, pageImpl] : m_pages) {
        result[id] = pageImpl;
    }
    return result;
}

std::map<std::string, std::shared_ptr<Page>> NonLinearBookImpl::getDownwardPagesHeirarchy() const {
    std::map<std::string, std::shared_ptr<Page>> result = getPages();
    
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

std::map<std::string, std::shared_ptr<Page>> NonLinearBookImpl::getUpwardPagesHeirarchy() const {
    std::map<std::string, std::shared_ptr<Page>> result = getPages();
    
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

std::shared_ptr<Page> NonLinearBookImpl::getPageById(const std::string& id) const {
    auto it = m_pages.find(id);
    return (it != m_pages.end()) ? it->second : nullptr;
}

std::map<std::string, std::shared_ptr<Obj>> NonLinearBookImpl::getObjs() const {
    std::map<std::string, std::shared_ptr<Obj>> result;
    for (const auto& [id, objImpl] : m_objs) {
        result[id] = objImpl;
    }
    return result;
}

std::shared_ptr<Obj> NonLinearBookImpl::getObjById(const std::string& objId) const {
    auto it = m_objs.find(objId);
    return (it != m_objs.end()) ? it->second : nullptr;
}

void NonLinearBookImpl::exportMedia(bool recursively, const std::string& mediaDir,
                                  const std::string& exportDir,
                                  const std::vector<MediaFile>& mediaFiles,
                                  MediaFile::Type type) const {
    // Создаем директорию экспорта если не существует
    if (!FileUtils::exists(exportDir)) {
        FileUtils::createDirectory(exportDir);
    }
    
    // Экспортируем медиафайлы
    for (const auto& mediaFile : mediaFiles) {
        std::string sourceFile = FileUtils::combinePath(mediaDir, mediaFile.getFileName());
        std::string targetFile = FileUtils::combinePath(exportDir, mediaFile.getFileName());
        
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

std::shared_ptr<Page> NonLinearBookImpl::createFilteredPage(const std::string& pageId,
                                                          const History& history) const {
    auto page = getPageById(pageId);
    if (!page) {
        return nullptr;
    }
    
    // Создаем фильтрованную копию страницы
    auto pageImpl = std::static_pointer_cast<PageImpl>(page);
    
    // Определяем исключаемые объекты и ссылки на основе истории
    std::vector<std::string> objIdsToBeExcluded;
    std::vector<std::string> linkIdsToBeExcluded;
    std::vector<std::shared_ptr<Link>> linksToBeAdded;
    std::map<std::string, std::shared_ptr<void>> visitedVars;
    
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

std::shared_ptr<NonLinearBook> NonLinearBookImpl::getParentNLB() const {
    return m_parentNLB;
}

bool NonLinearBookImpl::isDummy() const {
    return false;
}

std::shared_ptr<Page> NonLinearBookImpl::getParentPage() const {
    return m_parentPage;
}

std::map<std::string, std::shared_ptr<NonLinearBook>> NonLinearBookImpl::getExternalModules() const {
    std::map<std::string, std::shared_ptr<NonLinearBook>> result;
    
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

std::shared_ptr<NonLinearBook> NonLinearBookImpl::findExternalModule(const std::string& moduleId) const {
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

std::vector<std::shared_ptr<Variable>> NonLinearBookImpl::getVariables() const {
    std::vector<std::shared_ptr<Variable>> result;
    for (const auto& [varId, variable] : m_variables) {
        result.push_back(variable);
    }
    return result;
}

std::shared_ptr<Variable> NonLinearBookImpl::getVariableById(const std::string& id) const {
    auto it = m_variables.find(id);
    return (it != m_variables.end()) ? it->second : nullptr;
}

bool NonLinearBookImpl::load(const std::string& path, const ProgressData& progressData) {
    try {
        m_rootDir = path;
        readNLB(path);
        validateVariableReferences();
        resetVariableDataTypes();
        processAutowiredPages();
        return true;
    } catch (const std::exception& /*e*/) {
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
                                        std::shared_ptr<NonLinearBook> parentNLB, 
                                        std::shared_ptr<Page> parentPage) {
    m_parentNLB = parentNLB;
    m_parentPage = parentPage;
    return load(path, DummyProgressData());
}

void NonLinearBookImpl::append(const std::shared_ptr<NonLinearBook>& source, 
                              bool generateNewIds, bool overwriteTheme) {
    if (!source || source->isDummy()) {
        return;
    }
    
    // Копируем страницы
    auto sourcePages = source->getPages();
    for (const auto& [pageId, page] : sourcePages) {
        if (!page->isDeleted()) {
            // Создаем копию страницы
            auto newPage = std::make_shared<PageImpl>(page, shared_from_this(), overwriteTheme);
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
            auto newObj = std::make_shared<ObjImpl>(obj, shared_from_this());
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
            auto newVar = std::make_shared<VariableImpl>(variable, shared_from_this());
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

void NonLinearBookImpl::copyAndAddImageFile(const FileManipulator& fileManipulator, const std::string& imageFile, const std::string& imageFileName) {
    // Заглушка - копирование и добавление изображения
}

void NonLinearBookImpl::copyAndAddSoundFile(const FileManipulator& fileManipulator, const std::string& soundFile, const std::string& soundFileName) {
    // Заглушка - копирование и добавление звука
}

void NonLinearBookImpl::removeImageFile(const FileManipulator& fileManipulator, const std::string& imageFileName) {
    // Заглушка - удаление изображения
}

void NonLinearBookImpl::removeSoundFile(const FileManipulator& fileManipulator, const std::string& soundFileName) {
    // Заглушка - удаление звука
}

// Методы получения реализаций
std::shared_ptr<PageImpl> NonLinearBookImpl::getPageImplById(const std::string& id) const {
    auto it = m_pages.find(id);
    return (it != m_pages.end()) ? it->second : nullptr;
}

std::shared_ptr<ObjImpl> NonLinearBookImpl::getObjImplById(const std::string& id) const {
    auto it = m_objs.find(id);
    return (it != m_objs.end()) ? it->second : nullptr;
}

std::shared_ptr<VariableImpl> NonLinearBookImpl::getVariableImplById(const std::string& id) const {
    auto it = m_variables.find(id);
    return (it != m_variables.end()) ? it->second : nullptr;
}

std::vector<std::shared_ptr<Link>> NonLinearBookImpl::getAssociatedLinks(std::shared_ptr<NodeItem> nodeItem) {
    std::vector<std::shared_ptr<Link>> result;
    
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

// Заглушки для команд
std::shared_ptr<NLBCommand> NonLinearBookImpl::createUpdateModificationsCommand(std::shared_ptr<ModifyingItem> modifyingItem, std::shared_ptr<ModificationsTableModel> modificationsTableModel) {
    return nullptr; // Заглушка
}

std::shared_ptr<NLBCommand> NonLinearBookImpl::createUpdateBookPropertiesCommand(const std::string& license, std::shared_ptr<Theme> theme, const std::string& language, const std::string& title, const std::string& author, const std::string& version, const std::string& perfectGameAchievementName, bool fullAutowire, bool suppressMedia, bool suppressSound, bool propagateToSubmodules) {
    return nullptr; // Заглушка
}

std::shared_ptr<NLBCommand> NonLinearBookImpl::createUpdatePageCommand(std::shared_ptr<Page> page, const std::string& imageFileName, bool imageBackground, bool imageAnimated, const std::string& soundFileName, bool soundSFX, const std::string& pageVariableName, const std::string& pageTimerVariableName, const std::string& pageDefTagVariableValue, std::shared_ptr<MultiLangString> pageText, std::shared_ptr<MultiLangString> pageCaptionText, std::shared_ptr<Theme> theme, bool useCaption, bool useMPL, const std::string& moduleName, bool moduleExternal, std::shared_ptr<MultiLangString> traverseText, bool autoTraverse, bool autoReturn, std::shared_ptr<MultiLangString> returnText, const std::string& returnPageId, const std::string& moduleConsraintVariableName, bool autowire, std::shared_ptr<MultiLangString> autowireInText, std::shared_ptr<MultiLangString> autowireOutText, bool autoIn, bool needsAction, bool autoOut, const std::string& autowireInConstraint, const std::string& autowireOutConstraint, bool globalAutowire, bool noSave, bool autosFirst, std::shared_ptr<LinksTableModel> linksTableModel) {
    return nullptr; // Заглушка
}

std::shared_ptr<NLBCommand> NonLinearBookImpl::createChangeStartPointCommand(const std::string& startPoint) {
    return nullptr; // Заглушка
}

std::shared_ptr<NLBCommand> NonLinearBookImpl::createCopyCommand(const std::vector<std::string>& pageIds, const std::vector<std::string>& objIds) {
    return nullptr; // Заглушка
}

std::shared_ptr<NLBCommand> NonLinearBookImpl::createDeleteCommand(const std::vector<std::string>& pageIds, const std::vector<std::string>& objIds) {
    return nullptr; // Заглушка
}

std::shared_ptr<NLBCommand> NonLinearBookImpl::createPasteCommand(std::shared_ptr<NonLinearBookImpl> nlbToPaste) {
    return nullptr; // Заглушка
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

void NonLinearBookImpl::writeBookProperties(std::shared_ptr<FileManipulator> fileManipulator, const std::string& rootDir) {
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

void NonLinearBookImpl::loadPages(const std::string& rootDir, std::shared_ptr<PartialProgressData> partialProgressData) {
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
        auto page = std::make_shared<PageImpl>(shared_from_this());
        page->readPage(fullPageDir);
        m_pages[page->getId()] = page;
        
        if (partialProgressData) {
            partialProgressData->setRealProgressValue();
        }
    }
}

void NonLinearBookImpl::loadObjs(const std::string& rootDir, std::shared_ptr<PartialProgressData> partialProgressData) {
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
        auto obj = std::make_shared<ObjImpl>(shared_from_this());
        obj->readObj(fullObjDir);
        m_objs[obj->getId()] = obj;
        
        if (partialProgressData) {
            partialProgressData->setRealProgressValue();
        }
    }
}

void NonLinearBookImpl::loadVariables(const std::string& rootDir, std::shared_ptr<PartialProgressData> partialProgressData) {
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
        auto variable = std::make_shared<VariableImpl>(shared_from_this());
        variable->readVariable(fullVarDir);
        m_variables[variable->getId()] = variable;
        
        if (partialProgressData) {
            partialProgressData->setRealProgressValue();
        }
    }
}

void NonLinearBookImpl::loadMediaFiles(const std::string& rootDir, const std::string& mediaDirName, std::vector<MediaFile>& mediaFiles) {
    std::string mediaDir = FileUtils::combinePath(rootDir, mediaDirName);
    if (!FileUtils::exists(mediaDir)) {
        return;
    }
    
    std::vector<std::string> files = FileUtils::getDirectoryFiles(mediaDir);
    for (const std::string& fileName : files) {
        if (!FileUtils::isDirectory(FileUtils::combinePath(mediaDir, fileName))) {
            MediaFileImpl mediaFile(fileName);
            mediaFiles.push_back(mediaFile);
        }
    }
}

void NonLinearBookImpl::writeMediaFiles(std::shared_ptr<FileManipulator> fileManipulator, const std::string& rootDir, const std::vector<MediaFile>& mediaFiles, const std::string& mediaDirName) {
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

void NonLinearBookImpl::writePages(std::shared_ptr<FileManipulator> fileManipulator, const std::string& rootDir, std::shared_ptr<PartialProgressData> partialProgressData) {
    if (m_pages.empty()) {
        return;
    }
    
    std::string pagesDir = FileUtils::combinePath(rootDir, PAGES_DIR_NAME);
    fileManipulator->createDir(pagesDir, "Cannot create pages directory");
    
    for (const auto& [pageId, page] : m_pages) {
        page->writePage(fileManipulator, pagesDir, shared_from_this(), partialProgressData);
        if (partialProgressData) {
            partialProgressData->setRealProgressValue();
        }
    }
}

void NonLinearBookImpl::writeObjs(std::shared_ptr<FileManipulator> fileManipulator, const std::string& rootDir, std::shared_ptr<PartialProgressData> partialProgressData) {
    if (m_objs.empty()) {
        return;
    }
    
    std::string objsDir = FileUtils::combinePath(rootDir, OBJECTS_DIR_NAME);
    fileManipulator->createDir(objsDir, "Cannot create objects directory");
    
    for (const auto& [objId, obj] : m_objs) {
        obj->writeObj(fileManipulator, objsDir, shared_from_this());
        if (partialProgressData) {
            partialProgressData->setRealProgressValue();
        }
    }
}

void NonLinearBookImpl::writeVariables(std::shared_ptr<FileManipulator> fileManipulator, const std::string& rootDir, std::shared_ptr<PartialProgressData> partialProgressData) {
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

void NonLinearBookImpl::writePageOrderFile(std::shared_ptr<FileManipulator> fileManipulator, const std::string& rootDir) {
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

void NonLinearBookImpl::writeObjOrderFile(std::shared_ptr<FileManipulator> fileManipulator, const std::string& rootDir) {
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

void NonLinearBookImpl::writeVarOrderFile(std::shared_ptr<FileManipulator> fileManipulator, const std::string& rootDir) {
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

void NonLinearBookImpl::writeAutowiredPagesFile(std::shared_ptr<FileManipulator> fileManipulator, const std::string& rootDir) {
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

void NonLinearBookImpl::loadBook(const std::string& path, std::shared_ptr<ProgressData> progressData) {
    m_rootDir = path;
    readNLB(path);
}

void NonLinearBookImpl::saveBook(const std::string& path, std::shared_ptr<ProgressData> progressData) {
    // Заглушка для сохранения книги
}

void NonLinearBookImpl::writeNLB(std::shared_ptr<FileManipulator> fileManipulator, const std::string& nlbDir, std::shared_ptr<PartialProgressData> partialProgressData) {
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

void NonLinearBookImpl::readNLB(const std::string& nlbDir) {
    // Читаем свойства книги
    readBookProperties(nlbDir);
    
    // Создаем заглушку для прогресса
    auto partialProgress = std::make_shared<PartialProgressData>(std::make_shared<DummyProgressData>(), 0, 100, 1);
    
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