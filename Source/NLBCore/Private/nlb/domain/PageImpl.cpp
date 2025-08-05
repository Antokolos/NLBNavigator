#include "nlb/domain/PageImpl.h"
#include "nlb/util/FileManipulator.h"
#include "nlb/util/StringHelper.h"
#include "nlb/domain/CoordsImpl.h"
#include "nlb/domain/LinkImpl.h"
#include "nlb/api/Obj.h"
#include "nlb/domain/AbstractNodeItem.h"
#include <sstream>
#include <algorithm>
#include <any>

// Constants
const std::string PageImpl::TEXT_SUBDIR_NAME = "text";
const std::string PageImpl::IMAGE_FILE_NAME = "image";
const std::string PageImpl::IMAGEBG_FILE_NAME = "imagebg";
const std::string PageImpl::IMGANIM_FILE_NAME = "imganim";
const std::string PageImpl::SOUND_FILE_NAME = "sound";
const std::string PageImpl::SOUND_SFX_FILE_NAME = "soundsfx";
const std::string PageImpl::THEME_FILE_NAME = "theme";
const std::string PageImpl::VARID_FILE_NAME = "varid";
const std::string PageImpl::TVARID_FILE_NAME = "tvarid";
const std::string PageImpl::CAPTION_SUBDIR_NAME = "caption";
const std::string PageImpl::USE_CAPT_FILE_NAME = "use_capt";
const std::string PageImpl::USE_MPL_FILE_NAME = "use_mpl";
const std::string PageImpl::MODULE_SUBDIR_NAME = "module";
const std::string PageImpl::MODNAME_FILE_NAME = "modname";
const std::string PageImpl::EXTMOD_FILE_NAME = "extmod";
const std::string PageImpl::TRAVTEXT_FILE_NAME = "travtext";
const std::string PageImpl::AUTOTRAV_FILE_NAME = "autotrav";
const std::string PageImpl::AUTORET_FILE_NAME = "autoret";
const std::string PageImpl::RETTEXT_SUBDIR_NAME = "rettext";
const std::string PageImpl::RETPAGE_FILE_NAME = "retpage";
const std::string PageImpl::MODCNSID_FILE_NAME = "modcnsid";
const std::string PageImpl::AUTO_IN_TEXT_SUBDIR_NAME = "aintext";
const std::string PageImpl::AUTO_OUT_TEXT_SUBDIR_NAME = "aouttext";
const std::string PageImpl::AUTO_IN_FILE_NAME = "auto_in";
const std::string PageImpl::NEEDS_ACTION_FILE_NAME = "needsact";
const std::string PageImpl::AUTO_OUT_FILE_NAME = "auto_out";
const std::string PageImpl::AUTOWIRE_IN_CONSTRID_FILE_NAME = "autoid";
const std::string PageImpl::AUTOWIRE_OUT_CONSTRID_FILE_NAME = "autoutid";
const std::string PageImpl::NOSAVE_FILE_NAME = "nosave";
const std::string PageImpl::AUTOS_FIRST_FILE_NAME = "autfirst";
const std::string PageImpl::GLOBAL_AUTOWIRE_FILE_NAME = "globauto";

const std::string PageImpl::DEFAULT_MODULE_NAME_FORMAT = "%s's submodule";

// Default values for constructor initialization
const std::string DEFAULT_IMAGE_FILE_NAME = "";
const std::string DEFAULT_SOUND_FILE_NAME = "";
const bool DEFAULT_IMAGE_BACKGROUND = false;
const bool DEFAULT_IMAGE_ANIMATED = false;
const bool DEFAULT_SOUND_SFX = false;
const std::string DEFAULT_VARID = "";
const std::string DEFAULT_TVARID = "";
const bool DEFAULT_USE_CAPTION = false;
const bool DEFAULT_USE_MPL = false;
const bool DEFAULT_MODULE_EXTERNAL = false;
const bool DEFAULT_AUTO_TRAVERSE = false;
const bool DEFAULT_AUTO_RETURN = false;
const std::string DEFAULT_MODULE_CONSTR_ID = "";

// Default constructor
PageImpl::PageImpl() : AbstractNodeItem() {
    init();
}

PageImpl::PageImpl(std::shared_ptr<NonLinearBook> currentNLB) 
    : AbstractNodeItem(currentNLB), 
      m_imageFileName(DEFAULT_IMAGE_FILE_NAME),
      m_imageBackground(DEFAULT_IMAGE_BACKGROUND),
      m_imageAnimated(DEFAULT_IMAGE_ANIMATED),
      m_soundFileName(DEFAULT_SOUND_FILE_NAME),
      m_soundSFX(DEFAULT_SOUND_SFX),
      m_varId(DEFAULT_VARID),
      m_timerVarId(DEFAULT_TVARID),
      m_useCaption(DEFAULT_USE_CAPTION),
      m_useMPL(DEFAULT_USE_MPL),
      m_theme(Theme::DEFAULT),
      m_moduleExternal(DEFAULT_MODULE_EXTERNAL),
      m_autoTraverse(DEFAULT_AUTO_TRAVERSE),
      m_autoReturn(DEFAULT_AUTO_RETURN),
      m_moduleConstrId(DEFAULT_MODULE_CONSTR_ID)
{
    init();
}

PageImpl::PageImpl(std::shared_ptr<NonLinearBook> currentNLB, float left, float top)
    : AbstractNodeItem(currentNLB, left, top), 
      m_imageFileName(DEFAULT_IMAGE_FILE_NAME),
      m_imageBackground(DEFAULT_IMAGE_BACKGROUND),
      m_imageAnimated(DEFAULT_IMAGE_ANIMATED),
      m_soundFileName(DEFAULT_SOUND_FILE_NAME),
      m_soundSFX(DEFAULT_SOUND_SFX),
      m_varId(DEFAULT_VARID),
      m_timerVarId(DEFAULT_TVARID),
      m_useCaption(DEFAULT_USE_CAPTION),
      m_useMPL(DEFAULT_USE_MPL),
      m_theme(Theme::DEFAULT),
      m_moduleExternal(DEFAULT_MODULE_EXTERNAL),
      m_autoTraverse(DEFAULT_AUTO_TRAVERSE),
      m_autoReturn(DEFAULT_AUTO_RETURN),
      m_moduleConstrId(DEFAULT_MODULE_CONSTR_ID)
{
    init();
}

// Copy constructor
PageImpl::PageImpl(const std::shared_ptr<Page>& source, 
                   std::shared_ptr<NonLinearBook> currentNLB, 
                   bool overwriteTheme)
    : AbstractNodeItem(source, currentNLB)
{
    init();
    
    // Copy all properties from source page
    m_imageFileName = source->getImageFileName();
    m_imageBackground = source->isImageBackground();
    m_imageAnimated = source->isImageAnimated();
    m_soundFileName = source->getSoundFileName();
    m_soundSFX = source->isSoundSFX();
    m_varId = source->getVarId();
    m_timerVarId = source->getTimerVarId();
    m_moduleConstrId = source->getModuleConstrId();
    m_caption = source->getCaptions();
    m_useCaption = source->isUseCaption();
    m_useMPL = source->isUseMPL();
    m_text = source->getTexts();
    
    if (overwriteTheme) {
        m_theme = source->getTheme();
    }
    
    m_moduleName = source->getModuleName();
    m_moduleExternal = source->isModuleExternal();
    m_traverseText = source->getTraverseTexts();
    m_autoTraverse = source->isAutoTraverse();
    m_autoReturn = source->isAutoReturn();
    m_returnText = source->getReturnTexts();
    m_returnPageId = source->getReturnPageId();
    m_autowireInText = source->getAutowireInTexts();
    m_autowireOutText = source->getAutowireOutTexts();
    m_autoIn = source->isAutoIn();
    m_needsAction = source->isNeedsAction();
    m_autoOut = source->isAutoOut();
    m_autowireInConstrId = source->getAutowireInConstrId();
    m_autowireOutConstrId = source->getAutowireOutConstrId();
    
    // ... продолжение копирования атрибутов
    
    // Инициализация модуля
    m_module = std::make_shared<NonLinearBookImpl>(currentNLB, std::enable_shared_from_this<PageImpl>::shared_from_this());
    m_module->append(
        std::make_shared<NonLinearBookImpl>(source->getModule(), std::enable_shared_from_this<PageImpl>::shared_from_this()),
        true, overwriteTheme);
}

void PageImpl::init() {
    m_module = std::make_shared<NonLinearBookImpl>(AbstractNodeItem::getCurrentNLB(), std::enable_shared_from_this<PageImpl>::shared_from_this());
    resetDefaultModuleName();
    m_moduleName = m_defaultModuleName;
    m_moduleExternal = DEFAULT_MODULE_EXTERNAL;
    // Инициализация других полей
}

void PageImpl::resetDefaultModuleName() {
    // Форматирование строки по аналогии с Java реализацией
    std::stringstream ss;
    ss << AbstractModifyingItem::getId() << "'s submodule";
    m_defaultModuleName = ss.str();
}

// Геттеры и сеттеры (заглушки)
void PageImpl::setImageFileName(const std::string& imageFileName) {
    m_imageFileName = imageFileName;
}

std::string PageImpl::getImageFileName() const {
    return m_imageFileName;
}

void PageImpl::setImageBackground(bool imageBackground) {
    m_imageBackground = imageBackground;
}

bool PageImpl::isImageBackground() const {
    return m_imageBackground;
}

void PageImpl::setImageAnimated(bool imageAnimated) {
    m_imageAnimated = imageAnimated;
}

bool PageImpl::isImageAnimated() const {
    return m_imageAnimated;
}

void PageImpl::setSoundFileName(const std::string& soundFileName) {
    m_soundFileName = soundFileName;
}

std::string PageImpl::getSoundFileName() const {
    return m_soundFileName;
}

void PageImpl::setSoundSFX(bool soundSFX) {
    m_soundSFX = soundSFX;
}

bool PageImpl::isSoundSFX() const {
    return m_soundSFX;
}

void PageImpl::setText(const std::string& text) {
    m_text.put(getCurrentNLB()->getLanguage(), text);
}

std::string PageImpl::getText() const {
    return m_text.get(getCurrentNLB()->getLanguage());
}

void PageImpl::setTexts(const MultiLangString& text) {
    m_text = text;
}

MultiLangString PageImpl::getTexts() const {
    return m_text;
}

void PageImpl::setTheme(Theme theme) {
    m_theme = theme;
}

Theme PageImpl::getTheme() const {
    return m_theme;
}

Theme PageImpl::getEffectiveTheme() const {
    Theme bookTheme = getCurrentNLB()->getTheme();
    if (m_theme == Theme::DEFAULT) {
        return bookTheme;
    }
    return m_theme;
}

void PageImpl::setVarId(const std::string& varId) {
    m_varId = varId;
}

std::string PageImpl::getVarId() const {
    return m_varId;
}

void PageImpl::setTimerVarId(const std::string& timerVarId) {
    m_timerVarId = timerVarId;
}

std::string PageImpl::getTimerVarId() const {
    return m_timerVarId;
}

void PageImpl::setModuleConstrId(const std::string& moduleConstrId) {
    m_moduleConstrId = moduleConstrId;
}

std::string PageImpl::getModuleConstrId() const {
    return m_moduleConstrId;
}

void PageImpl::setCaption(const std::string& caption) {
    m_caption.put(getCurrentNLB()->getLanguage(), caption);
}

std::string PageImpl::getCaption() const {
    return m_caption.get(getCurrentNLB()->getLanguage());
}

std::string PageImpl::getNotes() const {
    return ""; // Default implementation returns empty string
}

void PageImpl::setCaptions(const MultiLangString& caption) {
    m_caption = caption;
}

MultiLangString PageImpl::getCaptions() const {
    return m_caption;
}

bool PageImpl::isUseCaption() const {
    return m_useCaption;
}

bool PageImpl::isUseMPL() const {
    return m_useMPL;
}

void PageImpl::setUseCaption(bool useCaption) {
    m_useCaption = useCaption;
}

void PageImpl::setUseMPL(bool useMPL) {
    m_useMPL = useMPL;
}

bool PageImpl::isLeaf() const {
    return getLinks().empty();
}

bool PageImpl::isFinish() const {
    return isLeaf() && !isModuleExternal();
}

void PageImpl::setTraverseText(const std::string& traverseText) {
    m_traverseText.put(getCurrentNLB()->getLanguage(), traverseText);
}

std::string PageImpl::getTraverseText() const {
    return m_traverseText.get(getCurrentNLB()->getLanguage());
}

void PageImpl::setTraverseTexts(const MultiLangString& traverseText) {
    m_traverseText = traverseText;
}

MultiLangString PageImpl::getTraverseTexts() const {
    return m_traverseText;
}

void PageImpl::setAutoTraverse(bool autoTraverse) {
    m_autoTraverse = autoTraverse;
}

bool PageImpl::isAutoTraverse() const {
    return m_autoTraverse;
}

void PageImpl::setAutoReturn(bool autoReturn) {
    m_autoReturn = autoReturn;
}

bool PageImpl::isAutoReturn() const {
    return m_autoReturn;
}

void PageImpl::setReturnText(const std::string& returnText) {
    m_returnText.put(getCurrentNLB()->getLanguage(), returnText);
}

std::string PageImpl::getReturnText() const {
    return m_returnText.get(getCurrentNLB()->getLanguage());
}

void PageImpl::setReturnTexts(const MultiLangString& returnText) {
    m_returnText = returnText;
}

MultiLangString PageImpl::getReturnTexts() const {
    return m_returnText;
}

void PageImpl::setReturnPageId(const std::string& returnPageId) {
    m_returnPageId = returnPageId;
}

std::string PageImpl::getReturnPageId() const {
    return m_returnPageId;
}

bool PageImpl::shouldReturn() const {
    return !m_returnPageId.empty();
}

void PageImpl::setModuleName(const std::string& moduleName) {
    m_moduleName = moduleName;
}

std::string PageImpl::getModuleName() const {
    return m_moduleName;
}

void PageImpl::setModuleExternal(bool moduleExternal) {
    m_moduleExternal = moduleExternal;
}

bool PageImpl::isModuleExternal() const {
    return m_moduleExternal;
}

std::shared_ptr<NonLinearBook> PageImpl::getModule() const {
    return m_module;
}

void PageImpl::setAutoIn(bool autoIn) {
    m_autoIn = autoIn;
}

void PageImpl::setNeedsAction(bool needsAction) {
    m_needsAction = needsAction;
}

void PageImpl::setAutoOut(bool autoOut) {
    m_autoOut = autoOut;
}

void PageImpl::setAutowireInConstrId(const std::string& autowireInConstrId) {
    m_autowireInConstrId = autowireInConstrId;
}

void PageImpl::setAutowireOutConstrId(const std::string& autowireOutConstrId) {
    m_autowireOutConstrId = autowireOutConstrId;
}

bool PageImpl::isAutowire() const {
    return isAutoIn() || isAutoOut();
}

void PageImpl::setGlobalAutowire(bool globalAutowire) {
    m_globalAutoWired = globalAutowire;
}

bool PageImpl::isGlobalAutowire() const {
    return m_globalAutoWired;
}

void PageImpl::setNoSave(bool noSave) {
    m_noSave = noSave;
}

bool PageImpl::isNoSave() const {
    return m_noSave;
}

void PageImpl::setAutosFirst(bool autosFirst) {
    m_autosFirst = autosFirst;
}

bool PageImpl::isAutosFirst() const {
    return m_autosFirst;
}

void PageImpl::setAutowireInText(const std::string& autowireInText) {
    m_autowireInText.put(getCurrentNLB()->getLanguage(), autowireInText);
}

std::string PageImpl::getAutowireInText() const {
    return m_autowireInText.get(getCurrentNLB()->getLanguage());
}

void PageImpl::setAutowireInTexts(const MultiLangString& autowireInText) {
    m_autowireInText = autowireInText;
}

MultiLangString PageImpl::getAutowireInTexts() const {
    return m_autowireInText;
}

void PageImpl::setAutowireOutText(const std::string& autowireOutText) {
    m_autowireOutText.put(getCurrentNLB()->getLanguage(), autowireOutText);
}

std::string PageImpl::getAutowireOutText() const {
    return m_autowireOutText.get(getCurrentNLB()->getLanguage());
}

void PageImpl::setAutowireOutTexts(const MultiLangString& autowireOutText) {
    m_autowireOutText = autowireOutText;
}

MultiLangString PageImpl::getAutowireOutTexts() const {
    return m_autowireOutText;
}

bool PageImpl::isAutoIn() const {
    return m_autoIn;
}

bool PageImpl::isNeedsAction() const {
    return m_needsAction;
}

bool PageImpl::isAutoOut() const {
    return m_autoOut;
}

std::string PageImpl::getAutowireInConstrId() const {
    return m_autowireInConstrId;
}

std::string PageImpl::getAutowireOutConstrId() const {
    return m_autowireOutConstrId;
}

void PageImpl::writePage(
    std::shared_ptr<FileManipulator> fileManipulator,
    const std::string& pagesDir,
    std::shared_ptr<NonLinearBookImpl> nonLinearBook,
    std::shared_ptr<PartialProgressData> partialProgressData)
{
    writeToFile(fileManipulator, pagesDir, getId(), nonLinearBook);
    
    const std::string pageDir = pagesDir + "/" + getId();
    
    if (!isDeleted()) {
        // Write basic page properties
        fileManipulator->writeOptionalString(pageDir, IMAGE_FILE_NAME, m_imageFileName, DEFAULT_IMAGE_FILE_NAME);
        fileManipulator->writeOptionalString(pageDir, IMAGEBG_FILE_NAME, 
            m_imageBackground ? "true" : "false", DEFAULT_IMAGE_BACKGROUND ? "true" : "false");
        fileManipulator->writeOptionalString(pageDir, IMGANIM_FILE_NAME,
            m_imageAnimated ? "true" : "false", DEFAULT_IMAGE_ANIMATED ? "true" : "false");
        fileManipulator->writeOptionalString(pageDir, SOUND_FILE_NAME, m_soundFileName, DEFAULT_SOUND_FILE_NAME);
        fileManipulator->writeOptionalString(pageDir, SOUND_SFX_FILE_NAME,
            m_soundSFX ? "true" : "false", DEFAULT_SOUND_SFX ? "true" : "false");
        fileManipulator->writeOptionalString(pageDir, THEME_FILE_NAME, 
            ThemeUtils::toString(m_theme), ThemeUtils::toString(DEFAULT_THEME));
        fileManipulator->writeOptionalString(pageDir, VARID_FILE_NAME, m_varId, DEFAULT_VARID);
        fileManipulator->writeOptionalString(pageDir, TVARID_FILE_NAME, m_timerVarId, DEFAULT_TVARID);
        
        // Write text content
        fileManipulator->writeOptionalMultiLangString(pageDir + "/" + TEXT_SUBDIR_NAME, m_text, DEFAULT_TEXT);
        fileManipulator->writeOptionalMultiLangString(pageDir + "/" + CAPTION_SUBDIR_NAME, m_caption, DEFAULT_CAPTION);
        
        // Write module properties
        const std::string moduleDir = pageDir + "/" + MODULE_SUBDIR_NAME;
        fileManipulator->writeOptionalString(moduleDir, MODNAME_FILE_NAME, m_moduleName, m_defaultModuleName);
        fileManipulator->writeOptionalString(moduleDir, EXTMOD_FILE_NAME,
            m_moduleExternal ? "true" : "false", DEFAULT_MODULE_EXTERNAL ? "true" : "false");
        fileManipulator->writeOptionalString(pageDir, MODCNSID_FILE_NAME, m_moduleConstrId, DEFAULT_MODULE_CONSTR_ID);
        
        // Write traverse properties
        fileManipulator->writeOptionalMultiLangString(pageDir + "/" + TRAVTEXT_FILE_NAME, m_traverseText, DEFAULT_TRAVERSE_TEXT);
        fileManipulator->writeOptionalString(pageDir, AUTOTRAV_FILE_NAME,
            m_autoTraverse ? "true" : "false", DEFAULT_AUTO_TRAVERSE ? "true" : "false");
        fileManipulator->writeOptionalString(pageDir, AUTORET_FILE_NAME,
            m_autoReturn ? "true" : "false", DEFAULT_AUTO_RETURN ? "true" : "false");
        
        // Write return properties
        fileManipulator->writeOptionalMultiLangString(pageDir + "/" + RETTEXT_SUBDIR_NAME, m_returnText, DEFAULT_RETURN_TEXT);
        fileManipulator->writeOptionalString(pageDir, RETPAGE_FILE_NAME, m_returnPageId, DEFAULT_RETURN_PAGE_ID);
        
        // Write autowire properties
        fileManipulator->writeOptionalMultiLangString(pageDir + "/" + AUTO_IN_TEXT_SUBDIR_NAME, m_autowireInText, DEFAULT_AUTOWIRE_IN_TEXT);
        fileManipulator->writeOptionalMultiLangString(pageDir + "/" + AUTO_OUT_TEXT_SUBDIR_NAME, m_autowireOutText, DEFAULT_AUTOWIRE_OUT_TEXT);
    }
}

void PageImpl::readPage(const std::string& pageDir) {
    // Устанавливаем ID из имени директории
    setId(FileUtils::getFileName(pageDir));
    
    // Читаем базовые свойства страницы
    m_imageFileName = FileManipulator::getOptionalFileAsString(
        pageDir, IMAGE_FILE_NAME, DEFAULT_IMAGE_FILE_NAME);
    
    m_imageBackground = FileManipulator::getOptionalFileAsString(
        pageDir, IMAGEBG_FILE_NAME, DEFAULT_IMAGE_BACKGROUND ? "true" : "false") == "true";
    
    m_imageAnimated = FileManipulator::getOptionalFileAsString(
        pageDir, IMGANIM_FILE_NAME, DEFAULT_IMAGE_ANIMATED ? "true" : "false") == "true";
    
    m_soundFileName = FileManipulator::getOptionalFileAsString(
        pageDir, SOUND_FILE_NAME, DEFAULT_SOUND_FILE_NAME);
    
    m_soundSFX = FileManipulator::getOptionalFileAsString(
        pageDir, SOUND_SFX_FILE_NAME, DEFAULT_SOUND_SFX ? "true" : "false") == "true";
    
    // Читаем тему
    std::string themeStr = FileManipulator::getOptionalFileAsString(
        pageDir, THEME_FILE_NAME, ThemeUtils::toString(DEFAULT_THEME));
    m_theme = ThemeUtils::fromString(themeStr);
    
    // Читаем переменные
    m_varId = FileManipulator::getOptionalFileAsString(
        pageDir, VARID_FILE_NAME, DEFAULT_VARID);
    
    m_timerVarId = FileManipulator::getOptionalFileAsString(
        pageDir, TVARID_FILE_NAME, DEFAULT_TVARID);
    
    // Читаем многоязычные тексты
    m_text = FileManipulator::readOptionalMultiLangString(
        pageDir + "/" + TEXT_SUBDIR_NAME, DEFAULT_TEXT);
    
    m_caption = FileManipulator::readOptionalMultiLangString(
        pageDir + "/" + CAPTION_SUBDIR_NAME, DEFAULT_CAPTION);
    
    // Читаем флаги использования
    m_useCaption = FileManipulator::getOptionalFileAsString(
        pageDir, USE_CAPT_FILE_NAME, DEFAULT_USE_CAPTION ? "true" : "false") == "true";
    
    m_useMPL = FileManipulator::getOptionalFileAsString(
        pageDir, USE_MPL_FILE_NAME, DEFAULT_USE_MPL ? "true" : "false") == "true";
    
    // Читаем модульные свойства
    const std::string moduleDir = FileUtils::combinePath(pageDir, MODULE_SUBDIR_NAME);
    m_moduleName = FileManipulator::getOptionalFileAsString(
        moduleDir, MODNAME_FILE_NAME, m_defaultModuleName);
    
    m_moduleExternal = FileManipulator::getOptionalFileAsString(
        moduleDir, EXTMOD_FILE_NAME, DEFAULT_MODULE_EXTERNAL ? "true" : "false") == "true";
    
    m_moduleConstrId = FileManipulator::getOptionalFileAsString(
        pageDir, MODCNSID_FILE_NAME, DEFAULT_MODULE_CONSTR_ID);
    
    // Читаем свойства обхода
    m_traverseText = FileManipulator::readOptionalMultiLangString(
        pageDir + "/" + TRAVTEXT_FILE_NAME, DEFAULT_TRAVERSE_TEXT);
    
    m_autoTraverse = FileManipulator::getOptionalFileAsString(
        pageDir, AUTOTRAV_FILE_NAME, DEFAULT_AUTO_TRAVERSE ? "true" : "false") == "true";
    
    m_autoReturn = FileManipulator::getOptionalFileAsString(
        pageDir, AUTORET_FILE_NAME, DEFAULT_AUTO_RETURN ? "true" : "false") == "true";
    
    // Читаем свойства возврата
    m_returnText = FileManipulator::readOptionalMultiLangString(
        pageDir + "/" + RETTEXT_SUBDIR_NAME, DEFAULT_RETURN_TEXT);
    
    m_returnPageId = FileManipulator::getOptionalFileAsString(
        pageDir, RETPAGE_FILE_NAME, DEFAULT_RETURN_PAGE_ID);
    
    // Читаем свойства автосвязи
    m_autowireInText = FileManipulator::readOptionalMultiLangString(
        pageDir + "/" + AUTO_IN_TEXT_SUBDIR_NAME, DEFAULT_AUTOWIRE_IN_TEXT);
    
    m_autowireOutText = FileManipulator::readOptionalMultiLangString(
        pageDir + "/" + AUTO_OUT_TEXT_SUBDIR_NAME, DEFAULT_AUTOWIRE_OUT_TEXT);
    
    m_autoIn = FileManipulator::getOptionalFileAsString(
        pageDir, AUTO_IN_FILE_NAME, DEFAULT_AUTO_IN ? "true" : "false") == "true";
    
    m_needsAction = FileManipulator::getOptionalFileAsString(
        pageDir, NEEDS_ACTION_FILE_NAME, DEFAULT_NEEDS_ACTION ? "true" : "false") == "true";
    
    m_autoOut = FileManipulator::getOptionalFileAsString(
        pageDir, AUTO_OUT_FILE_NAME, DEFAULT_AUTO_OUT ? "true" : "false") == "true";
    
    m_autowireInConstrId = FileManipulator::getOptionalFileAsString(
        pageDir, AUTOWIRE_IN_CONSTRID_FILE_NAME, DEFAULT_AUTOWIRE_IN_CONSTR_ID);
    
    m_autowireOutConstrId = FileManipulator::getOptionalFileAsString(
        pageDir, AUTOWIRE_OUT_CONSTRID_FILE_NAME, DEFAULT_AUTOWIRE_OUT_CONSTR_ID);
    
    // Читаем дополнительные флаги
    m_globalAutoWired = FileManipulator::getOptionalFileAsString(
        pageDir, GLOBAL_AUTOWIRE_FILE_NAME, DEFAULT_GLOBAL_AUTOWIRED ? "true" : "false") == "true";
    
    m_noSave = FileManipulator::getOptionalFileAsString(
        pageDir, NOSAVE_FILE_NAME, DEFAULT_NOSAVE ? "true" : "false") == "true";
    
    m_autosFirst = FileManipulator::getOptionalFileAsString(
        pageDir, AUTOS_FIRST_FILE_NAME, DEFAULT_AUTOS_FIRST ? "true" : "false") == "true";
    
    // Читаем общие свойства узла (координаты, ссылки, модификации)
    readNodeItemProperties(pageDir);
    
    // Инициализируем модуль, если еще не создан
    if (!m_module) {
        m_module = std::make_shared<NonLinearBookImpl>(getCurrentNLB(), 
            std::enable_shared_from_this<PageImpl>::shared_from_this());
    }
    
    // Загружаем модуль в зависимости от типа (внешний или локальный)
    if (isModuleExternal()) {
        // Для внешних модулей
        m_module->clear();
        std::shared_ptr<NonLinearBook> externalModule = getCurrentNLB()->findExternalModule(m_moduleName);
        if (externalModule) {
            m_module->append(externalModule, true, true);
        }
    } else {
        // Для локальных модулей
        if (FileUtils::exists(moduleDir) && FileUtils::isDirectory(moduleDir)) {
            try {
                std::string canonicalModuleDir = FileUtils::normalizePath(moduleDir);
                m_module->loadAndSetParent(canonicalModuleDir, getCurrentNLB(), 
                    std::enable_shared_from_this<PageImpl>::shared_from_this());
            } catch (const std::exception& e) {
                throw NLBIOException("Error loading module from directory: " + moduleDir + 
                    ". Error: " + e.what());
            }
        }
    }
    
    // Читаем модификации (важно делать это в конце)
    readModifications(pageDir);
}

std::shared_ptr<PageImpl> PageImpl::createFilteredCloneWithSubstitutions(
    const std::vector<std::string>& objIdsToBeExcluded,
    const std::vector<std::string>& linkIdsToBeExcluded,
    const std::vector<std::shared_ptr<Link>>& linksToBeAdded,
    std::map<std::string, std::shared_ptr<void>> visitedVars)
{
    // Создаем новую страницу-результат
    auto result = std::make_shared<PageImpl>(AbstractNodeItem::getCurrentNLB());
    
    // 1. Основные идентификаторы и флаги
    result->AbstractIdentifiableItem::setId(AbstractIdentifiableItem::getId());
    result->AbstractIdentifiableItem::setDeleted(AbstractIdentifiableItem::isDeleted());
    
    // 2. Преобразуем visitedVars для использования с StringHelper::replaceVariables
    std::map<std::string, Any> visitedVarsAny;
    for (const auto& [key, value] : visitedVars) {
        try {
            // Пытаемся получить строковое значение
            auto strPtr = std::static_pointer_cast<std::string>(value);
            if (strPtr) {
                visitedVarsAny[key] = *strPtr;
            }
        } catch (...) {
            // Если не удалось преобразовать в строку, используем пустую строку
            visitedVarsAny[key] = std::string("");
        }
    }
    
    // 3. Текст с заменой переменных и добавлением текста объектов
    std::string replacedText = StringHelper::replaceVariables(getText(), visitedVarsAny);
    std::string objText = generateObjText(objIdsToBeExcluded, visitedVars);
    result->setText(replacedText + objText);
    
    // 4. Копирование координат
    auto sourceCoords = getCoords();
    auto resultCoords = result->getCoords();
    auto resultCoordsImpl = std::static_pointer_cast<CoordsImpl>(resultCoords);
    resultCoordsImpl->setLeft(sourceCoords->getLeft());
    resultCoordsImpl->setTop(sourceCoords->getTop());
    resultCoordsImpl->setWidth(sourceCoords->getWidth());
    resultCoordsImpl->setHeight(sourceCoords->getHeight());
    
    // 5. Копирование всех основных свойств страницы (строго в том же порядке, что в Java)
    result->setImageFileName(getImageFileName());
    result->setReturnPageId(getReturnPageId());
    result->setTheme(getTheme());
    result->setVarId(getVarId());
    result->setTimerVarId(getTimerVarId());
    result->setCaption(getCaption());
    result->setModuleConstrId(getModuleConstrId());
    result->setModuleName(getModuleName());
    result->setModuleExternal(isModuleExternal());
    result->setReturnText(getReturnText());
    result->setTraverseText(getTraverseText());
    result->setUseCaption(isUseCaption());
    result->setUseMPL(isUseMPL());
    result->setAutoTraverse(isAutoTraverse());
    result->setAutoReturn(isAutoReturn());
    result->setAutowireInText(getAutowireInText());
    result->setAutowireOutText(getAutowireOutText());
    result->setAutoIn(isAutoIn());
    result->setAutoOut(isAutoOut());
    //result->setAutowire(isAutowire()); not needed because autowired is autoIn || autoOut
    result->setGlobalAutowire(isGlobalAutowire());
    result->setNoSave(isNoSave());
    result->setAutosFirst(isAutosFirst());
    result->setAutowireInConstrId(getAutowireInConstrId());
    result->setAutowireOutConstrId(getAutowireOutConstrId());
    result->setFill(getFill());
    result->setStroke(getStroke());
    result->setDefaultTagId(getDefaultTagId());
    result->setTextColor(getTextColor());
    
    // 6. Дополнительные свойства
    result->setImageBackground(isImageBackground());
    result->setImageAnimated(isImageAnimated());
    result->setSoundFileName(getSoundFileName());
    result->setSoundSFX(isSoundSFX());
    result->setNeedsAction(isNeedsAction());
    
    // 7. Копирование содержимого объектов (если есть)
    for (const std::string& objId : getContainedObjIds()) {
        // Проверяем, что объект не исключен
        if (std::find(objIdsToBeExcluded.begin(), objIdsToBeExcluded.end(), objId) == objIdsToBeExcluded.end()) {
            result->addContainedObjId(objId);
        }
    }
    
    // 8. Установка родителя
    result->AbstractIdentifiableItem::setParent(AbstractIdentifiableItem::getParent());
    
    // 9. Фильтрация существующих ссылок (исключаем linkIdsToBeExcluded)
    AbstractNodeItem::filterTargetLinkList(
        std::static_pointer_cast<AbstractNodeItem>(result),
        std::static_pointer_cast<AbstractNodeItem>(std::enable_shared_from_this<PageImpl>::shared_from_this()),
        linkIdsToBeExcluded
    );
    
    // 10. Добавление новых ссылок
    for (const auto& link : linksToBeAdded) {
        auto newLink = std::make_shared<LinkImpl>(
            std::static_pointer_cast<AbstractNodeItem>(result), 
            link
        );
        result->addLink(newLink);
    }
    
    // 11. Замена переменных в ссылках (метод с побочными эффектами)
    result->replaceVariablesInLinks(visitedVars);
    
    return result;
}

// Вспомогательный метод для замены переменных в ссылках
void PageImpl::replaceVariablesInLinks(std::map<std::string, std::shared_ptr<void>> visitedVars) {
    // Преобразуем visitedVars для использования с StringHelper::replaceVariables
    std::map<std::string, Any> visitedVarsAny;
    for (const auto& [key, value] : visitedVars) {
        try {
            // Пытаемся получить строковое значение
            auto strPtr = std::static_pointer_cast<std::string>(value);
            if (strPtr) {
                visitedVarsAny[key] = *strPtr;
            }
        } catch (...) {
            // Если не удалось преобразовать в строку, используем пустую строку
            visitedVarsAny[key] = std::string("");
        }
    }
    
    // Заменяем переменные в тексте каждой ссылки
    auto linkImpls = getLinkImpls();
    for (auto& link : linkImpls) {
        std::string originalText = link->getText();
        std::string replacedText = StringHelper::replaceVariables(originalText, visitedVarsAny);
        link->setText(replacedText);
        
        // Также заменяем переменные в альтернативном тексте ссылки
        std::string originalAltText = link->getAltText();
        std::string replacedAltText = StringHelper::replaceVariables(originalAltText, visitedVarsAny);
        link->setAltText(replacedAltText);
    }
}

// Вспомогательный метод для генерации текста объектов
std::string PageImpl::generateObjText(
    const std::vector<std::string>& objIdsToBeExcluded, 
    std::map<std::string, std::shared_ptr<void>> visitedVars)
{
    std::ostringstream result;
    
    // Получаем все объекты, содержащиеся на данной странице
    std::vector<std::string> containedObjIds = getContainedObjIds();
    
    // Преобразуем visitedVars для getCumulativeText (который принимает std::map<std::string, std::string>&)
    std::map<std::string, std::string> visitedVarsString;
    for (const auto& [key, value] : visitedVars) {
        try {
            // Пытаемся получить строковое значение
            auto strPtr = std::static_pointer_cast<std::string>(value);
            if (strPtr) {
                visitedVarsString[key] = *strPtr;
            }
        } catch (...) {
            // Если не удалось преобразовать, используем строковое представление "UNDEFINED"
            visitedVarsString[key] = "UNDEFINED";
        }
    }
    
    for (const std::string& objId : containedObjIds) {
        // Проверяем, что объект не исключен
        if (std::find(objIdsToBeExcluded.begin(), objIdsToBeExcluded.end(), objId) == objIdsToBeExcluded.end()) {
            // Получаем объект из книги
            auto obj = getCurrentNLB()->getObjById(objId);
            if (obj && !obj->isDeleted()) {
                // Добавляем кумулятивный текст объекта
                result << obj->getCumulativeText(objIdsToBeExcluded, visitedVarsString);
            }
        }
    }
    
    return result.str();
}