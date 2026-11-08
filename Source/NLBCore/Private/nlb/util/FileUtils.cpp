#include "nlb/util/FileUtils.h"
#include "nlb/util/BookPack.h"
#include <fstream>
#include <iterator>
#include <cstdio>
#include <algorithm>
#include <sstream>

#ifdef _WIN32
    // Без NOMINMAX макросы min/max из windows.h ломают std::min/std::max (MSVC)
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
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
    return pathFromUtf8(utf8).wstring();
}

std::string FileUtils::fromWide(const std::wstring& wide) {
    return pathToUtf8(std::filesystem::path(wide));
}

std::filesystem::path FileUtils::nativePath(const std::string& utf8Path) {
    return pathFromUtf8(utf8Path);
}
#endif

bool FileUtils::readFile(const std::string& path, std::string& content) {
    std::string rel;
    if (BookPack::resolve(path, rel)) {
        return BookPack::mounted()->readContent(rel, content);  // крупные файлы — с диска по запросу
    }
    return fileSystem().readAll(path, content);
}

bool FileUtils::writeFile(const std::string& path, const std::string& content) {
    return fileSystem().writeAll(path, content, false);
}

bool FileUtils::writeFileAtomic(const std::string& path, const std::string& content) {
    const std::string tmp = path + ".tmp";
    return writeFile(tmp, content) && renameReplace(tmp, path);
}

namespace {
/// Поток, который копит текст и дописывает его в файл через файловую систему
class FsStreamBuf : public std::stringbuf {
public:
    explicit FsStreamBuf(std::string path) : m_path(std::move(path)) {}
    ~FsStreamBuf() override { flushToFile(); }
protected:
    int sync() override { return flushToFile() ? 0 : -1; }
private:
    bool flushToFile() {
        const std::string chunk = str();
        if (chunk.empty()) return true;
        str(std::string());
        return FileUtils::fileSystem().writeAll(m_path, chunk, true);
    }
    std::string m_path;
};

class FsOStream : public std::ostream {
public:
    explicit FsOStream(const std::string& path) : std::ostream(nullptr), m_buf(path) { rdbuf(&m_buf); }
    ~FsOStream() override { m_buf.pubsync(); }
private:
    FsStreamBuf m_buf;
};
}

std::unique_ptr<std::ostream> FileUtils::openWriteStream(const std::string& path) {
    if (!fileSystem().writeAll(path, std::string(), false)) return nullptr;  // создать / обнулить
    return std::make_unique<FsOStream>(path);
}

std::unique_ptr<IFileSystem::Reader> FileUtils::openRead(const std::string& path) {
    return fileSystem().openRead(path);
}

bool FileUtils::fileInfo(const std::string& path, bool& isDirectory, uint64_t& size, int64_t& mtime) {
    std::string rel;
    if (BookPack::resolve(path, rel)) {
        const BookPack::Entry* e = BookPack::mounted()->find(rel);
        if (!e) return false;
        isDirectory = e->type == BookPack::Entry::Type::Directory;
        size = e->size;
        mtime = e->mtime;
        return true;
    }
    IFileSystem::Info info;
    if (!fileSystem().info(path, info)) return false;
    isDirectory = info.isDirectory;
    size = info.size;
    mtime = info.mtime;
    return true;
}

bool FileUtils::renameReplace(const std::string& from, const std::string& to) {
    return fileSystem().rename(from, to);
}

std::filesystem::path FileUtils::pathFromUtf8(const std::string& utf8) {
#ifndef _WIN32
    // вне Windows пути и так в UTF-8 (а libc++ Android NDK собран без char8_t в filesystem)
    return std::filesystem::path(utf8);
#elif defined(__cpp_char8_t)
    return std::filesystem::path(std::u8string(utf8.begin(), utf8.end()));
#else
    return std::filesystem::u8path(utf8);
#endif
}

std::string FileUtils::pathToUtf8(const std::filesystem::path& path) {
#ifndef _WIN32
    return path.string();
#elif defined(__cpp_char8_t)
    const std::u8string s = path.u8string();
    return std::string(s.begin(), s.end());
#else
    return path.u8string();
#endif
}

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
    std::string rel;
    if (BookPack::resolve(path, rel)) return BookPack::mounted()->find(rel) != nullptr;
    IFileSystem::Info info;
    return fileSystem().info(path, info);
}

bool FileUtils::isDirectory(const std::string& path) {
    std::string rel;
    if (BookPack::resolve(path, rel)) {
        const BookPack::Entry* e = BookPack::mounted()->find(rel);
        return e && e->type == BookPack::Entry::Type::Directory;
    }
    IFileSystem::Info info;
    return fileSystem().info(path, info) && info.isDirectory;
}

std::vector<std::string> FileUtils::getDirectoryFiles(const std::string& path) {
    std::string rel;
    if (BookPack::resolve(path, rel)) return BookPack::mounted()->children(rel);
    return fileSystem().list(path);
}

bool FileUtils::createDirectory(const std::string& path) {
    return fileSystem().makeDirectory(path);
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
            for (const auto& file : getDirectoryFiles(path)) {
                if (!remove(combinePath(path, file), true)) return false;
            }
        }
        return fileSystem().removeDirectory(path);
    }
    return fileSystem().removeFile(path);
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

// ============================================================================ NativeFileSystem
// Обычная файловая система: WinAPI (UTF-16, длинные пути через nativePath) или POSIX

namespace {
class NativeReader : public IFileSystem::Reader {
public:
    explicit NativeReader(const std::string& path) : m_in(FileUtils::nativePath(path), std::ios::binary) {
        if (m_in) {
            m_in.seekg(0, std::ios::end);
            m_size = static_cast<uint64_t>(m_in.tellg());
        }
    }
    bool ok() const { return static_cast<bool>(m_in); }
    uint64_t size() override { return m_size; }
    bool read(uint64_t offset, size_t size, char* dst) override {
        m_in.clear();
        m_in.seekg(static_cast<std::streamoff>(offset));
        return size == 0 || static_cast<bool>(m_in.read(dst, static_cast<std::streamsize>(size)));
    }
private:
    std::ifstream m_in;
    uint64_t m_size = 0;
};

class NativeFileSystem : public IFileSystem {
public:
    bool info(const std::string& path, Info& out) override {
        bool isDirectory = false;
        uint64_t size = 0;
        int64_t mtime = 0;
        if (!nativeInfo(path, isDirectory, size, mtime)) return false;
        out.isDirectory = isDirectory;
        out.size = size;
        out.mtime = mtime;
        return true;
    }
    std::vector<std::string> list(const std::string& path) override {
        std::vector<std::string> files;

#ifdef _WIN32
    WIN32_FIND_DATAW findData;
    const std::wstring pattern = FileUtils::nativePath(path).wstring() + L"\\*";
    HANDLE hFind = FindFirstFileW(pattern.c_str(), &findData);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            std::string filename = FileUtils::fromWide(findData.cFileName);
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
    bool readAll(const std::string& path, std::string& content) override {
        bool isDirectory = false;
        uint64_t size = 0;
        int64_t mtime = 0;
        if (!nativeInfo(path, isDirectory, size, mtime) || isDirectory) return false;  // каталог — не файл
        std::ifstream in(FileUtils::nativePath(path), std::ios::binary);
        if (!in) return false;
        content.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
        return true;
    }
    std::unique_ptr<Reader> openRead(const std::string& path) override {
        auto r = std::make_unique<NativeReader>(path);
        if (!r->ok()) return nullptr;
        return r;
    }
    bool writeAll(const std::string& path, const std::string& data, bool append) override {
        std::ofstream out(FileUtils::nativePath(path), std::ios::binary | (append ? std::ios::app : std::ios::trunc));
        if (!out) return false;
        out.write(data.data(), static_cast<std::streamsize>(data.size()));
        return static_cast<bool>(out);
    }
    bool makeDirectory(const std::string& path) override {
#ifdef _WIN32
    return CreateDirectoryW(FileUtils::nativePath(path).c_str(), nullptr) != 0;
#else
    return mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0;
#endif
    }
    bool removeFile(const std::string& path) override {
#ifdef _WIN32
        return DeleteFileW(FileUtils::nativePath(path).c_str()) != 0;
#else
        return unlink(path.c_str()) == 0;
#endif
    }
    bool removeDirectory(const std::string& path) override {
#ifdef _WIN32
        return RemoveDirectoryW(FileUtils::nativePath(path).c_str()) != 0;
#else
        return rmdir(path.c_str()) == 0;
#endif
    }
    bool rename(const std::string& from, const std::string& to) override {
#ifdef _WIN32
    return MoveFileExW(FileUtils::nativePath(from).c_str(), FileUtils::nativePath(to).c_str(),
                       MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED) != 0;
#else
    return std::rename(from.c_str(), to.c_str()) == 0;
#endif

    }

private:
    static bool nativeInfo(const std::string& path, bool& isDirectory, uint64_t& size, int64_t& mtime) {
#ifdef _WIN32
    WIN32_FILE_ATTRIBUTE_DATA data;
    if (!GetFileAttributesExW(FileUtils::nativePath(path).c_str(), GetFileExInfoStandard, &data)) {
        return false;
    }
    isDirectory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    size = (static_cast<uint64_t>(data.nFileSizeHigh) << 32) | data.nFileSizeLow;
    mtime = static_cast<int64_t>((static_cast<uint64_t>(data.ftLastWriteTime.dwHighDateTime) << 32)
                                 | data.ftLastWriteTime.dwLowDateTime);
    return true;
#else
    struct stat st;
    if (stat(path.c_str(), &st) != 0) {
        return false;
    }
    isDirectory = S_ISDIR(st.st_mode);
    size = static_cast<uint64_t>(st.st_size);
    mtime = static_cast<int64_t>(st.st_mtime) * 1000000000LL
#if defined(__linux__)
        + st.st_mtim.tv_nsec
#endif
        ;
    return true;
#endif
    }
};

std::shared_ptr<IFileSystem>& fileSystemSlot() {
    static std::shared_ptr<IFileSystem> fs = std::make_shared<NativeFileSystem>();
    return fs;
}
}

IFileSystem& FileUtils::fileSystem() {
    return *fileSystemSlot();
}

std::shared_ptr<IFileSystem> FileUtils::makeNativeFileSystem() {
    return std::make_shared<NativeFileSystem>();
}

void FileUtils::setFileSystem(std::shared_ptr<IFileSystem> fs) {
    fileSystemSlot() = fs ? std::move(fs) : std::make_shared<NativeFileSystem>();
}
