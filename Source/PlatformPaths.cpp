#include "PlatformPaths.h"

#include <cstdlib>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>  // CommandLineToArgvW
#endif

std::vector<std::string> utf8Args(int argc, char** argv) {
    std::vector<std::string> result;
#ifdef _WIN32
    int count = 0;
    LPWSTR* wargv = CommandLineToArgvW(GetCommandLineW(), &count);
    if (wargv) {
        for (int i = 0; i < count; ++i) {
            result.push_back(std::filesystem::path(wargv[i]).u8string());
        }
        LocalFree(wargv);
        return result;
    }
#endif
    for (int i = 0; i < argc; ++i) {
        result.push_back(argv[i] ? argv[i] : "");
    }
    return result;
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
