#include "nlb/api/RootModulePage.h"
#include "nlb/api/Constants.h"
#include "nlb/domain/SearchResult.h"
#include "nlb/util/MultiLangString.h"
#include "nlb/api/NonLinearBook.h"

RootModulePage::RootModulePage(std::shared_ptr<NonLinearBook> nlb, const std::string& pageId)
    : m_nlb(nlb)
    , m_pageId(pageId) {
}

std::string RootModulePage::getImageFileName() const {
    return DEFAULT_IMAGE_FILE_NAME;
}

bool RootModulePage::isImageBackground() const {
    return DEFAULT_IMAGE_BACKGROUND;
}

bool RootModulePage::isImageAnimated() const {
    return DEFAULT_IMAGE_ANIMATED;
}

std::string RootModulePage::getSoundFileName() const {
    return DEFAULT_SOUND_FILE_NAME;
}

bool RootModulePage::isSoundSFX() const {
    return DEFAULT_SOUND_SFX;
}

std::string RootModulePage::getText() const {
    return nlb::Constants::EMPTY_STRING;
}

MultiLangString RootModulePage::getTexts() const {
    return MultiLangString::createCopy(DEFAULT_TEXT);
}

Theme RootModulePage::getTheme() const {
    return DEFAULT_THEME;
}

Theme RootModulePage::getEffectiveTheme() const {
    return DEFAULT_THEME;
}

std::string RootModulePage::getVarId() const {
    return DEFAULT_VARID;
}

std::string RootModulePage::getTimerVarId() const {
    return DEFAULT_TVARID;
}

std::string RootModulePage::getCaption() const {
    return nlb::Constants::EMPTY_STRING;
}

std::string RootModulePage::getNotes() const {
    return nlb::Constants::EMPTY_STRING;
}

MultiLangString RootModulePage::getCaptions() const {
    return MultiLangString::createCopy(DEFAULT_CAPTION);
}

bool RootModulePage::isUseCaption() const {
    return DEFAULT_USE_CAPTION;
}

bool RootModulePage::isUseMPL() const {
    return DEFAULT_USE_MPL;
}

bool RootModulePage::isLeaf() const {
    return false;
}

bool RootModulePage::isFinish() const {
    return false;
}

std::string RootModulePage::getTraverseText() const {
    return nlb::Constants::EMPTY_STRING;
}

MultiLangString RootModulePage::getTraverseTexts() const {
    return MultiLangString::createEmptyText();
}

bool RootModulePage::isAutoTraverse() const {
    return true;
}

bool RootModulePage::isAutoReturn() const {
    return false;
}

std::string RootModulePage::getReturnText() const {
    return nlb::Constants::EMPTY_STRING;
}

MultiLangString RootModulePage::getReturnTexts() const {
    return MultiLangString::createCopy(DEFAULT_RETURN_TEXT);
}

std::string RootModulePage::getReturnPageId() const {
    return nlb::Constants::EMPTY_STRING;
}

bool RootModulePage::shouldReturn() const {
    return false;
}

std::string RootModulePage::getModuleConstrId() const {
    return nlb::Constants::EMPTY_STRING;
}

std::string RootModulePage::getModuleName() const {
    return nlb::Constants::MAIN_MODULE_NAME;
}

bool RootModulePage::isModuleExternal() const {
    return false;
}

NonLinearBook* RootModulePage::getModule() const {
    return m_nlb.get();
}

bool RootModulePage::isAutowire() const {
    return false;
}

bool RootModulePage::isGlobalAutowire() const {
    return DEFAULT_GLOBAL_AUTOWIRED;
}

bool RootModulePage::isNoSave() const {
    return DEFAULT_NOSAVE;
}

bool RootModulePage::isAutosFirst() const {
    return DEFAULT_AUTOS_FIRST;
}

std::string RootModulePage::getAutowireInText() const {
    return nlb::Constants::EMPTY_STRING;
}

MultiLangString RootModulePage::getAutowireInTexts() const {
    return MultiLangString::createCopy(DEFAULT_AUTOWIRE_IN_TEXT);
}

std::string RootModulePage::getAutowireOutText() const {
    return nlb::Constants::EMPTY_STRING;
}

MultiLangString RootModulePage::getAutowireOutTexts() const {
    return MultiLangString::createCopy(DEFAULT_AUTOWIRE_OUT_TEXT);
}

bool RootModulePage::isAutoIn() const {
    return DEFAULT_AUTO_IN;
}

bool RootModulePage::isNeedsAction() const {
    return DEFAULT_NEEDS_ACTION;
}

bool RootModulePage::isAutoOut() const {
    return DEFAULT_AUTO_OUT;
}

std::string RootModulePage::getAutowireInConstrId() const {
    return DEFAULT_AUTOWIRE_IN_CONSTR_ID;
}

std::string RootModulePage::getAutowireOutConstrId() const {
    return DEFAULT_AUTOWIRE_OUT_CONSTR_ID;
}

std::string RootModulePage::getId() const {
    return m_pageId;
}

std::string RootModulePage::getFullId() const {
    return m_pageId;
}

bool RootModulePage::isDeleted() const {
    return false;
}

std::shared_ptr<IdentifiableItem> RootModulePage::getParent() const {
    return nullptr;
}

bool RootModulePage::hasDeletedParent() const {
    return false;
}

std::shared_ptr<NonLinearBook> RootModulePage::getCurrentNLB() const {
    return m_nlb;
}

std::shared_ptr<SearchResult> RootModulePage::searchText(const SearchContract& contract) const {
    return std::make_shared<SearchResult>();
}