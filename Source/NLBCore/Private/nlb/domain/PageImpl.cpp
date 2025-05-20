#include "nlb/domain/PageImpl.h"
#include <sstream>
#include <algorithm>

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

// Default constructor
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
    // Copy constructor implementation
    m_imageFileName = source->getImageFileName();
    m_imageBackground = source->isImageBackground();
    m_imageAnimated = source->isImageAnimated();
    m_soundFileName = source->getSoundFileName();
    setSoundSFX(source->isSoundSFX());
    
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

// Остальные геттеры и сеттеры следуют аналогичному шаблону

std::string PageImpl::getText() const {
    // Заглушка, в реальной реализации нужно будет использовать MultiLangString
    return m_text.get(AbstractNodeItem::getCurrentNLB()->getLanguage());
}

Theme PageImpl::getEffectiveTheme() const {
    Theme bookTheme = AbstractNodeItem::getCurrentNLB()->getTheme();
    if (m_theme == Theme::DEFAULT) {
        return bookTheme;
    }
    return m_theme;
}

void PageImpl::writePage(
    std::shared_ptr<FileManipulator> fileManipulator,
    const std::string& pagesDir,
    std::shared_ptr<NonLinearBookImpl> nonLinearBook,
    std::shared_ptr<PartialProgressData> partialProgressData)
{
    // Заглушка для метода writePage
    // Здесь должна быть полная логика сохранения страницы
}

void PageImpl::readPage(const std::string& pageDir) {
    // Заглушка для метода readPage
    // Здесь должна быть полная логика чтения страницы
}

std::shared_ptr<PageImpl> PageImpl::createFilteredCloneWithSubstitutions(
    const std::vector<std::string>& objIdsToBeExcluded,
    const std::vector<std::string>& linkIdsToBeExcluded,
    const std::vector<std::shared_ptr<Link>>& linksToBeAdded,
    std::map<std::string, std::shared_ptr<void>> visitedVars)
{
    // Реализация клонирования страницы с подстановкой переменных
    auto result = std::make_shared<PageImpl>(AbstractNodeItem::getCurrentNLB());
    // Копирование и подстановка атрибутов
    
    return result;
}

void PageImpl::replaceVariablesInLinks(std::map<std::string, std::shared_ptr<void>> visitedVars) {
    // Замена переменных в ссылках
}

std::string PageImpl::generateObjText(
    const std::vector<std::string>& objIdsToBeExcluded, 
    std::map<std::string, std::shared_ptr<void>> visitedVars)
{
    // Генерация текста объектов
    std::stringstream result;
    // Логика генерации
    return result.str();
}

////////////////

// Add these implementations to PageImpl.cpp

std::string PageImpl::getSoundFileName() const {
    return m_soundFileName;
}

bool PageImpl::isSoundSFX() const {
    return m_soundSFX;
}

void PageImpl::setSoundSFX(bool soundSFX) {
    m_soundSFX = soundSFX;
}

bool PageImpl::isImageAnimated() const {
    return m_imageAnimated;
}

void PageImpl::setImageAnimated(bool imageAnimated) {
    m_imageAnimated = imageAnimated;
}

MultiLangString PageImpl::getTexts() const {
    return m_text;
}

Theme PageImpl::getTheme() const {
    return m_theme;
}

std::string PageImpl::getVarId() const {
    return m_varId;
}

std::string PageImpl::getTimerVarId() const {
    return m_timerVarId;
}

std::string PageImpl::getModuleConstrId() const {
    return m_moduleConstrId;
}

std::string PageImpl::getCaption() const {
    return m_caption.get(AbstractNodeItem::getCurrentNLB()->getLanguage());
}

std::string PageImpl::getNotes() const {
    return ""; // Default implementation returns empty string
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

bool PageImpl::isLeaf() const {
    return getLinks().empty();
}

bool PageImpl::isFinish() const {
    return isLeaf() && !isModuleExternal();
}

std::string PageImpl::getTraverseText() const {
    return m_traverseText.get(AbstractNodeItem::getCurrentNLB()->getLanguage());
}

MultiLangString PageImpl::getTraverseTexts() const {
    return m_traverseText;
}

bool PageImpl::isAutoTraverse() const {
    return m_autoTraverse;
}

bool PageImpl::isAutoReturn() const {
    return m_autoReturn;
}

std::string PageImpl::getReturnText() const {
    return m_returnText.get(AbstractNodeItem::getCurrentNLB()->getLanguage());
}

MultiLangString PageImpl::getReturnTexts() const {
    return m_returnText;
}

std::string PageImpl::getReturnPageId() const {
    return m_returnPageId;
}

bool PageImpl::shouldReturn() const {
    return !m_returnPageId.empty();
}

std::string PageImpl::getModuleName() const {
    return m_moduleName;
}

bool PageImpl::isModuleExternal() const {
    return m_moduleExternal;
}

std::shared_ptr<NonLinearBook> PageImpl::getModule() const {
    return m_module;
}

bool PageImpl::isAutowire() const {
    return isAutoIn() || isAutoOut();
}

bool PageImpl::isGlobalAutowire() const {
    return m_globalAutoWired;
}

bool PageImpl::isNoSave() const {
    return m_noSave;
}

bool PageImpl::isAutosFirst() const {
    return m_autosFirst;
}

std::string PageImpl::getAutowireInText() const {
    return m_autowireInText.get(AbstractNodeItem::getCurrentNLB()->getLanguage());
}

MultiLangString PageImpl::getAutowireInTexts() const {
    return m_autowireInText;
}

std::string PageImpl::getAutowireOutText() const {
    return m_autowireOutText.get(AbstractNodeItem::getCurrentNLB()->getLanguage());
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