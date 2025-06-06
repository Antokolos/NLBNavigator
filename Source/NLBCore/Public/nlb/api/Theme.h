#pragma once

#include <string>

enum class Theme {
    DEFAULT,
    STANDARD,
    VN
};

class NLBCORE_API ThemeUtils {
public:
    // Convert string to enum
    static Theme fromString(const std::string& name);
    // Convert enum to string
    static std::string toString(Theme theme);
};
