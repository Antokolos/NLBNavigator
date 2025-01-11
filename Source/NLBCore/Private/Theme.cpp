#include "Theme.h"
#include <algorithm>

Theme ThemeUtils::fromString(const std::string& name) {
    std::string upperName = name;
    std::transform(upperName.begin(), upperName.end(), upperName.begin(), ::toupper);
    
    if (upperName == "STANDARD") {
        return Theme::STANDARD;
    } else if (upperName == "VN") {
        return Theme::VN;
    }
    return Theme::DEFAULT;
}

std::string ThemeUtils::toString(Theme theme) {
    switch (theme) {
        case Theme::STANDARD:
            return "STANDARD";
        case Theme::VN:
            return "VN";
        default:
            return "DEFAULT";
    }
}