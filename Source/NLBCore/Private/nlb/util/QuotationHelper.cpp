#include "nlb/util/QuotationHelper.h"
#include "nlb/util/StringHelper.h"

// Initialize the static regex pattern - equivalent to Java's static block
const std::regex QuotationHelper::QUOTES = QuotationHelper::initializeQuotesPattern();

std::regex QuotationHelper::initializeQuotesPattern() {
    // This is the equivalent of the Java static initialization block
    // Pattern matches various Unicode quotation marks as defined in the original Java code
    // The pattern includes:
    // ` - backtick (U+0060)
    // ″ - double prime (U+2033) 
    // ״ - gershayim (U+05F4)
    // ؞ - arabic double quote (U+059E)
    // « - left double angle quote (U+00AB)
    // ‹ - left single angle quote (U+2039)
    // » - right double angle quote (U+00BB)
    // › - right single angle quote (U+203A)
    // „ - double low quotation mark (U+201E)
    // ‚ - single low quotation mark (U+201A)
    // " - left double quotation mark (U+201C)
    // " - right double quotation mark (U+201D)
    // ‟ - double high-reversed-9 quotation mark (U+201F)
    // ' - left single quotation mark (U+2018)
    // ‛ - single high-reversed-9 quotation mark (U+201B)
    // ' - right single quotation mark (U+2019)
    
    std::string pattern = R"([`\u2033\u05F4\u059E\u00AB\u2039\u00BB\u203A\u201E\u201A\u201C\u201F\u2018\u201B\u201D\u2019]+)";
    
    try {
        return std::regex(pattern, std::regex::ECMAScript);
    } catch (const std::regex_error& /*e*/) {
        // Fallback pattern if Unicode escapes are not supported
        // This covers basic ASCII quotation marks
        return std::regex(R"([`"'""''„‚«»‹›]+)", std::regex::ECMAScript);
    }
}

bool QuotationHelper::find(const std::string& source) {
    // Handle null/empty string case - equivalent to Java's StringHelper.isEmpty() check
    if (StringHelper::isEmpty(source)) {
        return false;
    }
    
    try {
        // Use regex_search to find any occurrence of quotation marks in the string
        return std::regex_search(source, QUOTES);
    } catch (const std::regex_error& /*e*/) {
        // If regex fails, return false
        return false;
    }
}

bool QuotationHelper::find(const char* source) {
    // Handle null pointer case
    if (source == nullptr) {
        return false;
    }
    
    // Convert to string and delegate to the string version
    return find(std::string(source));
}