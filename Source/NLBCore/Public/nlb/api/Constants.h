#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <string>

namespace nlb {

/**
 * The Constants class
 *
 * @author Anton P. Kolosov
 * @version 1.0
 */
class Constants {
public:
    static constexpr float FL_ZERO_TOLERANCE = 1e-6f;
    static const char* const UNICODE_ENCODING;
    static const char* const MEDIA_FILE_NAME_SEP;
    static const char* const RU;
    static const char* const EN;
    static const char* const MAIN_MODULE_NAME;
    static const char* const EMPTY_STRING;
    static const char* const VOID;
    static const char* const EOL_STRING;
    static const char* const MODIFICATIONS_UNDO_ID_POSTFIX;
    static const char* const NLB_MODULE_ATTR;
    static const char* const NLB_PAGE_ATTR;
    static const char* const NLB_OBJ_ATTR;
    static const char* const NLB_LINK_ATTR;
    static const char* const YES;
    static const char* const NO;
};

} // namespace nlb

#endif // CONSTANTS_H