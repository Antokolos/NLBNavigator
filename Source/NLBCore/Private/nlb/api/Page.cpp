#include "nlb/api/Page.h"
#include "nlb/api/Constants.h"
#include "nlb/util/MultiLangString.h"

// Initialize static string and object constants
const std::string Page::DEFAULT_IMAGE_FILE_NAME = nlb::Constants::EMPTY_STRING;
const std::string Page::DEFAULT_SOUND_FILE_NAME = nlb::Constants::EMPTY_STRING;
const MultiLangString Page::DEFAULT_TEXT = MultiLangString::createEmptyText();
const Theme Page::DEFAULT_THEME = Theme::DEFAULT;
const std::string Page::DEFAULT_VARID = nlb::Constants::EMPTY_STRING;
const std::string Page::DEFAULT_TVARID = nlb::Constants::EMPTY_STRING;
const MultiLangString Page::DEFAULT_CAPTION = MultiLangString::createEmptyText();
const MultiLangString Page::DEFAULT_TRAVERSE_TEXT = MultiLangString::createDefaultTraverseText();
const MultiLangString Page::DEFAULT_AUTOWIRE_IN_TEXT = MultiLangString::createDefaultLinkText();
const MultiLangString Page::DEFAULT_AUTOWIRE_OUT_TEXT = MultiLangString::createDefaultLinkText();
const std::string Page::DEFAULT_AUTOWIRE_IN_CONSTR_ID = nlb::Constants::EMPTY_STRING;
const std::string Page::DEFAULT_AUTOWIRE_OUT_CONSTR_ID = nlb::Constants::EMPTY_STRING;
const MultiLangString Page::DEFAULT_RETURN_TEXT = MultiLangString::createEmptyText();
const std::string Page::DEFAULT_RETURN_PAGE_ID = nlb::Constants::EMPTY_STRING;
const std::string Page::DEFAULT_MODULE_CONSTR_ID = nlb::Constants::EMPTY_STRING;