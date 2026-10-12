#include "shunting-yard.h"
#include "builtin-features.inc"

#include "nlb/domain/NLBReader.h"
#include "nlb/domain/NLBExplorer.h"
#include "nlb/api/ConsoleProgressData.h"
#include "nlb/api/DummyProgressData.h"
#include "PlatformPaths.h"
#include "nlb/util/BookPack.h"
#include "SessionLog.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

/**
 * @brief Файл прогресса достижений (аналог prefs INSTEAD): ~/.nlbnavigator/<имя каталога книги>.achievements.
 * Если домашний каталог недоступен — рядом с книгой. Возвращается путь в UTF-8 (так его ждёт
 * PlayerContext); при любой ошибке — пустая строка, и достижения хранятся только в памяти.
 */
static std::string achievementsFilePath(const std::filesystem::path& bookPath) {
    namespace fs = std::filesystem;
    try {
        const fs::path book = bookPath.lexically_normal();
        fs::path bookName = book.filename();
        if (bookName.empty()) bookName = book.parent_path().filename();
        if (bookName.extension() == ".nlbpack") bookName = bookName.stem();
        fs::path achievementsName = bookName;
        achievementsName += ".achievements";
        std::error_code ec;
        const fs::path home = homeDir();
        if (!home.empty()) {
            const fs::path dir = home / ".nlbnavigator";
            fs::create_directories(dir, ec);
            if (!ec) return (dir / achievementsName).u8string();
        }
        return (book / ".achievements").u8string();
    } catch (const std::exception& e) {
        std::cerr << "Warning: achievements will not be saved: " << e.what() << std::endl;
        return std::string();
    }
}

/**
 * @brief Каталог сохранений: ~/.nlbnavigator/<имя каталога книги>.saves (UTF-8); пусто — недоступен
 */
static std::string savesDirectory(const std::filesystem::path& bookPath) {
    namespace fs = std::filesystem;
    try {
        const fs::path book = bookPath.lexically_normal();
        fs::path bookName = book.filename();
        if (bookName.empty()) bookName = book.parent_path().filename();
        if (bookName.extension() == ".nlbpack") bookName = bookName.stem();
        const fs::path home = homeDir();
        fs::path dir = home.empty() ? book / ".saves" : home / ".nlbnavigator" / bookName;
        if (!home.empty()) dir += ".saves";
        std::error_code ec;
        fs::create_directories(dir, ec);
        return ec ? std::string() : dir.u8string();
    } catch (const std::exception& e) {
        std::cerr << "Warning: saves are not available: " << e.what() << std::endl;
        return std::string();
    }
}

/**
 * @brief Источник данных книги перед загрузкой. Файл .nlbpack монтируется как каталог книги.
 * Для каталога используется кэш ~/.nlbnavigator/<книга>.nlbpack: если он совпадает с каталогом
 * (те же файлы, размеры и времена изменения — сверяются только метаданные), книга читается из
 * него; иначе кэш пересобирается. Ошибки кэша не мешают загрузке: читается сам каталог.
 */
static void prepareBookSource(const std::string& nlbPath, bool useCache) {
    namespace fs = std::filesystem;
    std::string error;
    const fs::path path = fs::u8path(nlbPath);
    std::error_code ec;
    if (fs::is_regular_file(path, ec)) {
        auto pack = BookPack::read(nlbPath, error);
        if (!pack) {
            std::cerr << "Warning: " << error << std::endl;
            return;
        }
        BookPack::mount(std::shared_ptr<BookPack>(std::move(pack)), nlbPath);
        return;
    }
    if (!useCache) return;
    try {
        fs::path bookName = path.lexically_normal().filename();
        if (bookName.empty()) bookName = path.lexically_normal().parent_path().filename();
        const fs::path home = homeDir();
        if (home.empty()) return;
        const fs::path dir = home / ".nlbnavigator";
        fs::create_directories(dir, ec);
        const std::string cachePath = (dir / bookName).u8string() + ".nlbpack";
        auto cached = BookPack::read(cachePath, error);
        if (cached && cached->matches(nlbPath)) {
            BookPack::mount(std::shared_ptr<BookPack>(std::move(cached)), nlbPath);
            return;
        }
        // Кэша нет или книга изменилась: один проход по каталогу — и в пакет, и в память
        auto fresh = BookPack::build(nlbPath, error);
        if (!fresh) return;
        fresh->write(cachePath, error);
        BookPack::mount(std::shared_ptr<BookPack>(std::move(fresh)), nlbPath);
    } catch (const std::exception&) {
        BookPack::unmount();  // кэш — только ускорение: при сбое читаем каталог как обычно
    }
}

/**
 * @brief Main function demonstrating NLB Reader usage
 */
int main(int argc, char* argv[]) {

    cparse_startup();

    GlobalScope::default_global()["x"] = 10;
    std::cout << calculator::calculate("'Hello ' + 'World'") << std::endl;
    std::cout << calculator::calculate("x + 1", GlobalScope::default_global()) << std::endl;

    // Консоль в UTF-8 (в Windows — без вывода "Active code page", который читал бы диктор)
    setupConsoleUtf8();
    // Аргументы — в UTF-8 (в Windows из UTF-16 командной строки)
    const std::vector<std::string> args = utf8Args(argc, argv);
    std::vector<std::string> positional;
    std::string logPath;
    bool screenReader = false;
    // Кэш каталога выключен по умолчанию: проверка его свежести обходит весь каталог, и на
    // Linux это не быстрее самой загрузки. Для поставки игры — упакованный файл (pack)
    bool useCache = false;
    for (size_t i = 1; i < args.size(); ++i) {
        const std::string& arg = args[i];
        if (arg == "--log" && i + 1 < args.size()) {
            logPath = args[++i];
        } else if (arg.rfind("--log=", 0) == 0) {
            logPath = arg.substr(6);
        } else if (arg == "--screen-reader") {
            screenReader = true;
        } else if (arg == "--cache") {
            useCache = true;
        } else if (arg == "--no-cache") {
            useCache = false;
        } else {
            positional.push_back(arg);
        }
    }
    if (positional.empty()) {
        std::cout << "Usage: " << args[0] << " <path_to_nlb_directory> [mode] [--log <file>]" << std::endl;
        std::cout << "Modes: info (default) | explore [vn|standard]" << std::endl;
        std::cout << "  explore vn       - play as exported by exportToVNSTEADFile (default)" << std::endl;
        std::cout << "  explore standard - play as exported by exportToSTEADFile" << std::endl;
        std::cout << "  --log <file>     - save the session (output and input) to the file" << std::endl;
        std::cout << "  --screen-reader  - mode for screen readers (NVDA, JAWS, Orca)" << std::endl;
        std::cout << "  pack [<file>]    - pack the book directory into a single .nlbpack file" << std::endl;
        std::cout << "  <file.nlbpack>   - a packed book can be played instead of a directory" << std::endl;
        std::cout << "  --cache          - keep a packed copy of the book directory in ~/.nlbnavigator" << std::endl;
        return 1;
    }

    // Журнал сессии: весь вывод и ввод дублируются в файл
    SessionLog sessionLog;
    if (!logPath.empty() && !sessionLog.open(logPath)) {
        std::cerr << "Warning: cannot open log file: " << logPath << std::endl;
    }

    const std::string nlbPath = positional[0];
    const std::string mode = positional.size() > 1 ? positional[1] : "info";

    if (mode == "pack") {
        // Упаковать каталог книги в один файл: для поставки игры и быстрой загрузки
        const std::string out = positional.size() > 2 ? positional[2]
            : std::filesystem::u8path(nlbPath).lexically_normal().u8string() + ".nlbpack";
        std::string error;
        auto pack = BookPack::build(nlbPath, error);
        if (!pack || !pack->write(out, error)) {
            std::cerr << "Cannot pack the book: " << error << std::endl;
            return 1;
        }
        std::cout << "Packed " << pack->fileCount() << " files into " << out << std::endl;
        return 0;
    }
    prepareBookSource(nlbPath, useCache);

    if (mode == "explore") {
        auto book = std::make_unique<NonLinearBookImpl>();
        bool loaded;
        if (screenReader) {
            // Индикатор, переписывающий строку, диктор зачитал бы десятки раз
            std::cout << "Загрузка книги..." << std::endl;
            DummyProgressData progressData;
            loaded = book->load(nlbPath, progressData);
        } else {
            ConsoleProgressData progressData;
            loaded = book->load(nlbPath, progressData);
        }
        if (!loaded) {
            std::cout << (screenReader ? "Не удалось загрузить книгу: " : "Cannot load book: ") << nlbPath << std::endl;
            return 1;
        }
        if (screenReader) {
            std::cout << "Книга загружена: " << book->getTitle()
                      << (book->getAuthor().empty() ? "" : ", автор: " + book->getAuthor()) << "." << std::endl;
        } else {
            std::cout << "Loaded: " << book->getTitle() << " by " << book->getAuthor() << std::endl;
        }
        PlayerEngine::Settings settings;
        settings.achievementsPath = achievementsFilePath(std::filesystem::u8path(nlbPath));
        const std::string exportMode = positional.size() > 2 ? positional[2] : "vn";
        settings.exportMode = (exportMode == "standard")
            ? PlayerEngine::ExportMode::Standard : PlayerEngine::ExportMode::VN;
        NLBExplorer explorer(book.get(), book->getStartPoint(), settings);
        explorer.setScreenReaderMode(screenReader);
        explorer.setSavesDirectory(savesDirectory(std::filesystem::u8path(nlbPath)));
        if (sessionLog.isOpen()) {
            explorer.setInputListener([&sessionLog](const std::string& line) { sessionLog.recordInput(line); });
        }
        explorer.explore();
    } else {
        NLBReader reader;
        reader.readBook(nlbPath);
    }
    
    return 0;
}
