#include "nlb/api/SpecialVariablesNameHelper.h"
#include <algorithm>

std::string SpecialVariablesNameHelper::decorateId(const std::string& id) {
    return replaceHyphens(id);
}

std::string SpecialVariablesNameHelper::decorateLinkVisitStateVar(const std::string& linkId) {
    return "lvs_" + replaceHyphens(linkId);
}

std::string SpecialVariablesNameHelper::decorateId(const std::string& id, const std::string& autowiredId) {
    std::string result = "vl_";
    
    if (!autowiredId.empty()) {
        result += replaceHyphens(autowiredId) + "_";
    }
    
    result += replaceHyphens(id);
    return result;
}

std::string SpecialVariablesNameHelper::replaceHyphens(const std::string& str) {
    std::string result = str;
    std::replace(result.begin(), result.end(), '-', '_');
    return result;
}