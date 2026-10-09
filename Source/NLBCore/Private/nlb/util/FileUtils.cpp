#include "nlb/util/FileUtils.h"
#include <cstdio>
#include <algorithm>
#include <sstream>

#ifdef _WIN32
    #include <windows.h>
    #include <direct.h>
    #define PATH_SEPARATOR '\\'
#else
    #include <sys/stat.h>
    #include <unistd.h>
    #include <dirent.h>
    #define PATH_SEPARATOR '/'
#endif

#ifdef _WIN32
std::wstring FileUtils::toWide(const std::string& utf8) {
    if (utf8.empty()) return std::wstring();
    UINT codePage = CP_UTF8;
    int len = MultiByteToWideChar(codePage, MB_ERR_INVALID_CHARS, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    if (len <= 0) {
        codePage = CP_ACP;  // не UTF-8: например, путь из argv в кодировке ANSI
        len = MultiByteToWideChar(codePage, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    }
    std::wstring result(static_cast<size_t>(len), L'\0');
    MultiByteToWideChar(codePage, 0, utf8.data(), static_cast<int>(utf8.size()), &result[0], len);
    return result;
}

std::string FileUtils::fromWide(const std::wstring& wide) {
    if (wide.empty()) return std::string();
    int len = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
    std::string result(static_cast<size_t>(len), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), &result[0], len, nullptr, nullptr);
    return result;
}

std::filesystem::path FileUtils::nativePath(const std::string& utf8Path) {
    std::wstring path = toWide(utf8Path);
    std::replace(path.begin(), path.end(), L'/', L'\\');
    if (path.empty() || path.rfind(L"\\\\?\\", 0) == 0) {
        return std::filesystem::path(path);
    }
    // Префикс \\?\ отключает нормализацию путей в WinAPI, поэтому путь сначала делается абсолютным
    DWORD len = GetFullPathNameW(path.c_str(), 0, nullptr, nullptr);
    if (len == 0) {
        return std::filesystem::path(path);
    }
    std::wstring full(len, L'\0');
    len = GetFullPathNameW(path.c_str(), len, &full[0], nullptr);
    full.resize(len);
    if (full.rfind(L"\\\\", 0) == 0) {
        return std::filesystem::path(L"\\\\?\\UNC\\" + full.substr(2));  // \\server\share
    }
    return std::filesystem::path(L"\\\\?\\" + full);
}
#else
std::wstring FileUtils::toWide(const std::string& utf8) {
    return std::filesystem::u8path(utf8).wstring();
}

std::string FileUtils::fromWide(const std::wstring& wide) {
    return std::filesystem::path(wide).u8string();
}

std::filesystem::path FileUtils::nativePath(const std::string& utf8Path) {
    return std::filesystem::u8path(utf8Path);
}
#endif

char FileUtils::getPathSeparator() {
    return PATH_SEPARATOR;
}

std::string FileUtils::getFileName(const std::string& path) {
    if (path.empty()) return "";
    
    size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos) return path;
    
    return path.substr(pos + 1);
}

std::string FileUtils::getFilePath(const std::string& path) {
    if (path.empty()) return "";
    
    size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos) return "";
    
    return path.substr(0, pos);
}

std::string FileUtils::combinePath(const std::string& path1, const std::string& path2) {
    if (path1.empty()) return normalizePath(path2);
    if (path2.empty()) return normalizePath(path1);
    
    std::string normalized1 = normalizePath(path1);
    std::string normalized2 = normalizePath(path2);
    
    // Remove trailing separator from first path
    while (!normalized1.empty() && (normalized1.back() == '/' || normalized1.back() == '\\')) {
        normalized1.pop_back();
    }
    
    // Remove leading separator from second path
    while (!normalized2.empty() && (normalized2.front() == '/' || normalized2.front() == '\\')) {
        normalized2.erase(0, 1);
    }
    
    return normalized1 + getPathSeparator() + normalized2;
}

bool FileUtils::exists(const std::string& path) {
#ifdef _WIN32
    DWORD attrs = GetFileAttributesW(nativePath(path).c_str());
    return (attrs != INVALID_FILE_ATTRIBUTES);
#else
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
#endif
}

bool FileUtils::isDirectory(const std::string& path) {
#ifdef _WIN32
    DWORD attrs = GetFileAttributesW(nativePath(path).c_str());
    return (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0 && S_ISDIR(buffer.st_mode));
#endif
}

std::vector<std::string> FileUtils::getDirectoryFiles(const std::string& path) {
    std::vector<std::string> files;

#ifdef _WIN32
    WIN32_FIND_DATAW findData;
    const std::wstring pattern = nativePath(path).wstring() + L"\\*";
    HANDLE hFind = FindFirstFileW(pattern.c_str(), &findData);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            std::string filename = fromWide(findData.cFileName);
            if (filename != "." && filename != "..") {
                files.push_back(filename);
            }
        } while (FindNextFileW(hFind, &findData));
        FindClose(hFind);
    }
#else
    DIR* dir = opendir(path.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir))) {
            std::string filename = entry->d_name;
            if (filename != "." && filename != "..") {
                files.push_back(filename);
            }
        }
        closedir(dir);
    }
#endif

    return files;
}

bool FileUtils::createDirectory(const std::string& path) {
#ifdef _WIN32
    return CreateDirectoryW(nativePath(path).c_str(), nullptr) != 0;
#else
    return mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0;
#endif
}

bool FileUtils::createDirectoryRecursive(const std::string& path) {
    if (path.empty()) return false;
    
    // Если директория уже существует, возвращаем true
    if (exists(path) && isDirectory(path)) {
        return true;
    }
    
    // Получаем родительскую директорию
    std::string parentPath = getFilePath(path);
    
    // Если есть родительская директория и она не существует, создаем её рекурсивно
    if (!parentPath.empty() && !exists(parentPath)) {
        if (!createDirectoryRecursive(parentPath)) {
            return false;
        }
    }
    
    // Создаем текущую директорию
    return createDirectory(path);
}

bool FileUtils::remove(const std::string& path, bool recursive) {
    if (!exists(path)) return true;

    if (isDirectory(path)) {
        if (recursive) {
            std::vector<std::string> files = getDirectoryFiles(path);
            for (const auto& file : files) {
                std::string fullPath = combinePath(path, file);
                if (!remove(fullPath, true)) return false;
            }
        }
#ifdef _WIN32
        return RemoveDirectoryW(nativePath(path).c_str()) != 0;
#else
        return rmdir(path.c_str()) == 0;
#endif
    } else {
#ifdef _WIN32
        return DeleteFileW(nativePath(path).c_str()) != 0;
#else
        return unlink(path.c_str()) == 0;
#endif
    }
}

std::vector<std::string> FileUtils::splitPath(const std::string& path) {
    std::vector<std::string> components;
    std::stringstream ss(path);
    std::string item;

    while (std::getline(ss, item, '/')) {
        if (!item.empty() && item != "\\") {
            size_t pos = 0;
            while ((pos = item.find('\\', pos)) != std::string::npos) {
                std::string subitem = item.substr(0, pos);
                if (!subitem.empty()) {
                    components.push_back(subitem);
                }
                item = item.substr(pos + 1);
                pos = 0;
            }
            if (!item.empty()) {
                components.push_back(item);
            }
        }
    }

    return components;
}

std::string FileUtils::normalizePath(const std::string& path) {
    std::vector<std::string> components = splitPath(path);
    std::string result;
    
    for (size_t i = 0; i < components.size(); ++i) {
        if (i > 0) result += getPathSeparator();
        result += components[i];
    }
    
    // Preserve leading slash if it existed
    if (!path.empty() && (path[0] == '/' || path[0] == '\\')) {
        result = getPathSeparator() + result;
    }
    
    return result;
}