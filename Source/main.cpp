#include "shunting-yard.h"
#include "builtin-features.inc"

#include "nlb/domain/NLBReader.h"
#include "nlb/domain/NLBExplorer.h"
#include "nlb/api/ConsoleProgressData.h"
#include "nlb/api/DummyProgressData.h"
#include "PlatformPaths.h"
#include "ImageConvert.h"
#include "nlb/api/PropertyManager.h"
#include "nlb/util/BookPack.h"
#ifdef NLB_WITH_SDL3
#include "nlb/audio/SdlAudioBackend.h"
#include "nlb/presentation/AudioDirector.h"
#include "nlb/presentation/BookMediaSource.h"
#endif
#include "nlb/util/FileUtils.h"
#include "SessionLog.h"

#include <cstdlib>
#include <filesystem>
#include <algorithm>
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
            if (!ec) return FileUtils::pathToUtf8(dir / achievementsName);
        }
        return FileUtils::pathToUtf8(book / ".achievements");
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
        return ec ? std::string() : FileUtils::pathToUtf8(dir);
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
    const fs::path path = FileUtils::pathFromUtf8(nlbPath);
    std::error_code ec;
    if (FileUtils::exists(nlbPath) && !FileUtils::isDirectory(nlbPath)) {
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
        const std::string cachePath = FileUtils::pathToUtf8(dir / bookName) + ".nlbpack";
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
    bool withMedia = false;  // pack: класть в пакет и картинки со звуками
    std::string configPath;  // nlb.json (настройки экспорта); пусто — в текущем каталоге или у программы
    int png2jpg = -1;        // pack: PNG -> JPG (-1 — по nlb.json, 0 — нет, 1 — да)
    int jpgQuality = 0;      // pack: качество JPG (0 — по nlb.json)
    std::string packTheme;   // pack: каталог темы игры (INSTEAD-экспорт), зашивается в пакет
    std::string packBaseTheme;  // pack: каталог базовой темы (оформление по умолчанию)
    bool sound = true;       // звук через SDL3, если плеер собран с ним
    // Детерминированная случайность (PlayerContext::setDeterministic): k-й rnd(n) даёт
    // (k mod n) + 1, SHUFFLE разворачивает список. Для воспроизводимых журналов (сверка с
    // INSTEAD и Java-плеером NLB); -1 — выключено, иначе начальное значение счётчика
    int64_t deterministicStart = -1;
    for (size_t i = 1; i < args.size(); ++i) {
        const std::string& arg = args[i];
        if (arg == "--log" && i + 1 < args.size()) {
            logPath = args[++i];
        } else if (arg.rfind("--log=", 0) == 0) {
            logPath = arg.substr(6);
        } else if (arg == "--screen-reader") {
            screenReader = true;
        } else if (arg == "--theme" && i + 1 < args.size()) {
            packTheme = args[++i];
        } else if (arg == "--base-theme" && i + 1 < args.size()) {
            packBaseTheme = args[++i];
        } else if (arg == "--with-media") {
            withMedia = true;
        } else if (arg == "--config" && i + 1 < args.size()) {
            configPath = args[++i];
        } else if (arg == "--png2jpg") {
            png2jpg = 1;
        } else if (arg == "--no-png2jpg") {
            png2jpg = 0;
        } else if (arg == "--quality" && i + 1 < args.size()) {
            jpgQuality = std::atoi(args[++i].c_str());
        } else if (arg == "--no-sound") {
            sound = false;
        } else if (arg == "--deterministic") {
            deterministicStart = 0;
        } else if (arg.rfind("--deterministic=", 0) == 0) {
            deterministicStart = std::atoll(arg.c_str() + 16);
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
        std::cout << "  --with-media     - pack: include images and sounds into the .nlbpack file" << std::endl;
        std::cout << "  --theme <dir>    - pack: include the game theme (INSTEAD theme directory) into the .nlbpack file" << std::endl;
        std::cout << "  --base-theme <dir> - pack: include the base theme (default look; the game theme overrides it)" << std::endl;
        std::cout << "  --config <file>  - export settings (nlb.json); default: ./nlb.json or next to the program" << std::endl;
        std::cout << "  --png2jpg | --no-png2jpg - pack: convert PNG images to JPG (default: nlb.json, convertpng2jpg)" << std::endl;
        std::cout << "  --quality <1..100> - pack: JPG quality (default: nlb.json, quality)" << std::endl;
        std::cout << "  --no-sound       - do not play music and sounds" << std::endl;
        std::cout << "  --deterministic[=<n>] - reproducible random: k-th rnd(n) gives (k mod n) + 1," << std::endl;
        std::cout << "                     SHUFFLE reverses the list; <n> - start of the counter (default 0)" << std::endl;
        return 1;
    }

    // Журнал сессии: весь вывод и ввод дублируются в файл
    SessionLog sessionLog;
    if (!logPath.empty() && !sessionLog.open(logPath)) {
        std::cerr << "Warning: cannot open log file: " << logPath << std::endl;
    }

    const std::string nlbPath = positional[0];
    const std::string mode = positional.size() > 1 ? positional[1] : "info";

    // Настройки экспорта (nlb.json): PNG -> JPG, тексты по умолчанию, needs-action-count
    {
        std::string error;
        if (!configPath.empty()) {
            if (!PropertyManager::loadConfig(configPath, error)) {
                std::cerr << "Cannot read the settings: " << error << std::endl;
                return 1;
            }
        } else {
            PropertyManager::loadDefaultConfig(FileUtils::pathToUtf8(FileUtils::pathFromUtf8(args[0]).parent_path()));
        }
    }
    if (mode == "pack") {
        // Упаковать каталог книги в один файл: для поставки игры и быстрой загрузки
        const std::string out = positional.size() > 2 ? positional[2]
            : FileUtils::pathToUtf8(FileUtils::pathFromUtf8(nlbPath).lexically_normal()) + ".nlbpack";
        std::string error;
        std::cout << "Reading " << nlbPath << "..." << std::endl;
        auto pack = BookPack::build(nlbPath, error, withMedia, packTheme, packBaseTheme);
        if (!pack) {
            std::cerr << "Cannot pack the book: " << error << std::endl;
            return 1;
        }
        const ExportConfig& exp = PropertyManager::getSettings().getDefaultConfig().getExport();
        BookPack::WriteOptions options;
        options.convertDefault = png2jpg < 0 ? exp.isConvertpng2jpg() : png2jpg == 1;
        options.quality = jpgQuality > 0 ? std::min(100, jpgQuality) : exp.getQuality();
        if (withMedia) options.convertPng = convertPngToJpg;  // без медиа картинок в пакете нет
        // прогресс: строка, которая переписывается на месте (при каждом новом проценте)
        int lastPercent = -1;
        options.progress = [&](size_t done, size_t total, const std::string& rel) {
            const int percent = total ? static_cast<int>(done * 100 / total) : 100;
            if (percent == lastPercent && done != total) return;
            lastPercent = percent;
            const std::string name = rel.size() > 50 ? "..." + rel.substr(rel.size() - 47) : rel;
            std::cout << "\r[" << (percent < 10 ? "  " : percent < 100 ? " " : "") << percent << "%] "
                      << done << "/" << total << " " << name << std::string(50 - std::min<size_t>(50, name.size()), ' ') << std::flush;
        };
        if (withMedia) {
            std::cout << "PNG -> JPG: " << (options.convertDefault ? "yes, quality " + std::to_string(options.quality)
                                                                 : std::string("no (only COMPRESSED images)")) << std::endl;
        }
        BookPack::WriteStats stats;
        if (!pack->write(out, error, options, &stats)) {
            std::cerr << std::endl << "Cannot pack the book: " << error << std::endl;
            return 1;
        }
        std::cout << std::endl << "Packed " << pack->fileCount() << " files into " << out << std::endl;
        if (stats.converted || stats.keptPng) {
            std::cout << "PNG -> JPG: " << stats.converted << " converted, " << stats.keptPng << " kept as PNG; images "
                      << (stats.bytesBefore >> 20) << " MB -> " << (stats.bytesAfter >> 20) << " MB" << std::endl;
        }
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
        settings.achievementsPath = achievementsFilePath(FileUtils::pathFromUtf8(nlbPath));
        const std::string exportMode = positional.size() > 2 ? positional[2] : "vn";
        settings.exportMode = (exportMode == "standard")
            ? PlayerEngine::ExportMode::Standard : PlayerEngine::ExportMode::VN;
        if (deterministicStart >= 0) {
            settings.deterministic = true;
            settings.deterministicStart = deterministicStart;
        }
        NLBExplorer explorer(book.get(), book->getStartPoint(), settings);
        explorer.setScreenReaderMode(screenReader);
        explorer.setSavesDirectory(savesDirectory(FileUtils::pathFromUtf8(nlbPath)));
#ifdef NLB_WITH_SDL3
        // Звук: SDL3_mixer через AudioDirector — та же логика, что у графических фронтендов
        std::unique_ptr<nlb::presentation::BookMediaSource> media;
        std::unique_ptr<nlb::audio::SdlAudioBackend> audio;
        std::unique_ptr<nlb::presentation::AudioDirector> director;
        if (sound && !book->isSuppressSound()) {
            media = std::make_unique<nlb::presentation::BookMediaSource>(nlbPath);
            audio = std::make_unique<nlb::audio::SdlAudioBackend>(*media);
            if (audio->isAvailable()) {
                director = std::make_unique<nlb::presentation::AudioDirector>(*audio);
                explorer.setEventListener([&director](const std::vector<PlayerEngine::Event>& events) {
                    director->apply(events);
                });
                explorer.setSoundSwitch([&director](bool enable) {
                    director->setMuted(!enable);
                    return enable;
                }, true);
            } else {
                std::cerr << "Warning: sound is not available: " << audio->lastError() << std::endl;
            }
        }
#else
        (void)sound;
#endif
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
