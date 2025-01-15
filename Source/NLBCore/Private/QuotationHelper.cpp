#include "QuotationHelper.h"
#include "StringHelper.h"

// Initialize the static regex pattern
const std::regex QuotationHelper::QUOTES{
    R"([`\\u2033\\u05F4\\u059E\\u00AB\\u2039\\u00BB\\u203A\\u201E\\u201A\\u201C\\u201F\\u2018\\u201B\\u201D\\u2019]+)",
    std::regex::ECMAScript
};

bool QuotationHelper::find(const std::string& source) {
    if (StringHelper::isEmpty(source)) {
        return false;
    }
    return std::regex_search(source, QUOTES);
}