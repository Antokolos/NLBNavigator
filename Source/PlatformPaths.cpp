#include "PlatformPaths.h"

#include <cstdlib>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>  // CommandLineToArgvW
#endif

std::filesystem::path bookPathArg(const char* narrowArg) {
#ifdef _WIN32
    int count = 0;
    LPWSTR* wargv = CommandLineToArgvW(GetCommandLineW(), &count);
    if (wargv) {
        std::filesystem::path result = count > 1 ? std::filesystem::path(wargv[1]) : std::filesystem::path();
        LocalFree(wargv);
        if (!result.empty()) return result;
    }
#endif
    return std::filesystem::path(narrowArg ? narrowArg : "");
}

std::filesystem::path homeDir() {
#ifdef _WIN32
    if (const wchar_t* home = _wgetenv(L"USERPROFILE")) return std::filesystem::path(home);
    if (const wchar_t* home = _wgetenv(L"HOME")) return std::filesystem::path(home);
#else
    if (const char* home = std::getenv("HOME")) return std::filesystem::path(home);
#endif
    return {};
}
