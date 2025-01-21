#include "nlb/api/Link.h"
#include "nlb/util/MultiLangString.h"
#include "nlb/api/Constants.h"

// Define static constants
inline const std::string Link::DEFAULT_VAR_ID = nlb::Constants::EMPTY_STRING;
inline const std::string Link::DEFAULT_TARGET = nlb::Constants::EMPTY_STRING;
inline const MultiLangString Link::DEFAULT_TEXT = MultiLangString::createDefaultLinkText();
inline const MultiLangString Link::DEFAULT_ALT_TEXT = MultiLangString::createEmptyText();
inline const std::string Link::DEFAULT_CONSTR_ID = nlb::Constants::EMPTY_STRING;
inline const std::string Link::DEFAULT_STROKE = "0000FF";
inline const bool Link::DEFAULT_AUTO = false;
inline const bool Link::DEFAULT_NEEDS_ACTION = false;
inline const bool Link::DEFAULT_ONCE = false;
inline const bool Link::DEFAULT_TECHNICAL = false;