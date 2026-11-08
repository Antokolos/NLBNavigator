#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

/*!
 * \brief Файловая система для плеера: всё чтение и запись файлов ядра идёт через неё.
 *
 * По умолчанию — обычная (NativeFileSystem: WinAPI / POSIX). Фронтенд может подставить свою
 * (FileUtils::setFileSystem): Unreal — через IPlatformFile движка (pak, данные игры на консолях,
 * каталог сохранений платформы). Смонтированный пакет книги (BookPack) — слой над ней.
 * Пути — UTF-8.
 */
class IFileSystem {
public:
    virtual ~IFileSystem() = default;

    struct Info {
        bool isDirectory = false;
        uint64_t size = 0;
        int64_t mtime = 0;  ///< только для сравнения «изменился ли»; единицы зависят от системы
    };
    /// false — нет такого файла или каталога
    virtual bool info(const std::string& path, Info& out) = 0;
    /// Имена файлов и подкаталогов (без «.» и «..»)
    virtual std::vector<std::string> list(const std::string& dir) = 0;
    virtual bool readAll(const std::string& path, std::string& out) = 0;

    /// Чтение с произвольного места (большие файлы: пакет книги — не целиком в память)
    class Reader {
    public:
        virtual ~Reader() = default;
        virtual uint64_t size() = 0;
        /// Прочитать size байт с offset; false — не удалось (конец файла, ошибка)
        virtual bool read(uint64_t offset, size_t size, char* dst) = 0;
    };
    virtual std::unique_ptr<Reader> openRead(const std::string& path) = 0;

    /// Записать файл целиком (append — дописать в конец)
    virtual bool writeAll(const std::string& path, const std::string& data, bool append) = 0;
    /// Создать один каталог (родитель должен существовать)
    virtual bool makeDirectory(const std::string& path) = 0;
    virtual bool removeFile(const std::string& path) = 0;
    /// Удалить пустой каталог
    virtual bool removeDirectory(const std::string& path) = 0;
    /// Переименовать с заменой существующего
    virtual bool rename(const std::string& from, const std::string& to) = 0;
};

/*!
 * \class FileUtils
 * \brief Utility class for file system operations
 */
class FileUtils {
public:
    /*!
     * \brief Get file name from path
     * \param path Full file path
     * \return File name with extension
     */
    static std::string getFileName(const std::string& path);

    /*!
     * \brief Get directory path from full path
     * \param path Full file path
     * \return Directory path without trailing separator
     */
    static std::string getFilePath(const std::string& path);

    /*!
     * \brief Combine two path parts
     * \param path1 First path part
     * \param path2 Second path part
     * \return Combined path with proper separator
     */
    static std::string combinePath(const std::string& path1, const std::string& path2);

    /*!
     * \brief Check if file or directory exists
     * \param path File path to check
     * \return true if file exists
     */
    static bool exists(const std::string& path);

    /*!
     * \brief Check if path is a directory
     * \param path Path to check
     * \return true if path is a directory
     */
    static bool isDirectory(const std::string& path);

    /*!
     * \brief Get list of files in directory
     * \param path Directory path
     * \return Vector of file names
     */
    static std::vector<std::string> getDirectoryFiles(const std::string& path);

    /*!
     * \brief Create directory
     * \param path Directory path to create
     * \return true if successful
     */
    static bool createDirectory(const std::string& path);
    /*!
     * \brief Create directory recursively (with parent directories)
     * \param path Directory path to create
     * \return true if successful
     */
    static bool createDirectoryRecursive(const std::string& path);

    /*!
     * \brief Delete file or directory
     * \param path Path to delete
     * \param recursive If true, recursively delete directory contents
     * \return true if successful
     */
    static bool remove(const std::string& path, bool recursive = false);

    /*!
     * \brief Get platform-specific path separator
     * \return Path separator character
     */
    static char getPathSeparator();
    
    // Split path into components
    static std::vector<std::string> splitPath(const std::string& path);
    
    // Normalize path separators for current platform
    static std::string normalizePath(const std::string& path);

    /*!
     * \brief Путь для обращения к файловой системе.
     *
     * Внутри библиотеки пути хранятся в UTF-8. В Windows путь переводится в UTF-16, делается
     * абсолютным и получает префикс \\?\ — без него WinAPI и CRT не открывают файлы длиннее
     * MAX_PATH (260 символов), а у книг с вложенными модулями (Frontier) пути бывают длиннее.
     * В остальных системах — путь из UTF-8 (pathFromUtf8).
     */
    static std::filesystem::path nativePath(const std::string& utf8Path);

    /// Прочитать файл целиком (с учётом смонтированного пакета книги BookPack); false — нет файла
    static bool readFile(const std::string& path, std::string& content);
    /// Записать файл целиком (сохранения, настройки, журналы); false — не удалось
    static bool writeFile(const std::string& path, const std::string& content);
    /// Записать атомарно: во временный файл рядом и переименовать (сбой не портит прежний файл)
    static bool writeFileAtomic(const std::string& path, const std::string& content);
    /// Поток записи в файл через файловую систему (журналы): копит текст и дописывает его в
    /// файл при flush и при закрытии. nullptr — файл не создать
    static std::unique_ptr<std::ostream> openWriteStream(const std::string& path);
    /// Чтение с произвольного места (с учётом пакета — нет: для самого файла пакета и т. п.)
    static std::unique_ptr<IFileSystem::Reader> openRead(const std::string& path);

    /// Файловая система плеера (по умолчанию — обычная); фронтенд подставляет свою до открытия книги
    static IFileSystem& fileSystem();
    static void setFileSystem(std::shared_ptr<IFileSystem> fs);
    /// Новая обычная файловая система (WinAPI / POSIX) — например, чтобы своя делегировала ей часть путей
    static std::shared_ptr<IFileSystem> makeNativeFileSystem();

    /// Тип, размер и время изменения файла. В Windows — через WinAPI: функции std::filesystem
    /// в MinGW (stat из msvcrt) не принимают пути с префиксом \\?\, без которого не открыть
    /// длинные пути; поэтому с путями nativePath() std::filesystem использовать нельзя
    static bool fileInfo(const std::string& path, bool& isDirectory, uint64_t& size, int64_t& mtime);
    /// Переименовать файл с заменой существующего (атомарная запись: сначала во временный)
    static bool renameReplace(const std::string& from, const std::string& to);

    /// UTF-8 <-> std::filesystem::path. В C++17 это u8path/u8string; в C++20 u8path объявлен
    /// устаревшим, а u8string() возвращает std::u8string — эти функции работают в обоих стандартах
    static std::filesystem::path pathFromUtf8(const std::string& utf8);
    static std::string pathToUtf8(const std::filesystem::path& path);

    /// UTF-8 <-> UTF-16 (Windows). Строка, не являющаяся UTF-8, читается в кодировке ANSI.
    static std::wstring toWide(const std::string& utf8);
    static std::string fromWide(const std::wstring& wide);
};