#include "shunting-yard.h"
#include "builtin-features.inc"

#include "nlb/domain/NLBReader.h"
#include "nlb/domain/NLBExplorer.h"
#include "nlb/api/ConsoleProgressData.h"
#include "PlatformPaths.h"
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
 * @brief Main function demonstrating NLB Reader usage
 */
int main(int argc, char* argv[]) {

    cparse_startup();

    GlobalScope::default_global()["x"] = 10;
    std::cout << calculator::calculate("'Hello ' + 'World'") << std::endl;
    std::cout << calculator::calculate("x + 1", GlobalScope::default_global()) << std::endl;

#ifdef _WIN32
    system("chcp 65001");
#endif
    // Аргументы — в UTF-8 (в Windows из UTF-16 командной строки)
    const std::vector<std::string> args = utf8Args(argc, argv);
    std::vector<std::string> positional;
    std::string logPath;
    for (size_t i = 1; i < args.size(); ++i) {
        const std::string& arg = args[i];
        if (arg == "--log" && i + 1 < args.size()) {
            logPath = args[++i];
        } else if (arg.rfind("--log=", 0) == 0) {
            logPath = arg.substr(6);
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
        return 1;
    }

    // Журнал сессии: весь вывод и ввод дублируются в файл
    SessionLog sessionLog;
    if (!logPath.empty() && !sessionLog.open(logPath)) {
        std::cerr << "Warning: cannot open log file: " << logPath << std::endl;
    }

    const std::string nlbPath = positional[0];
    const std::string mode = positional.size() > 1 ? positional[1] : "info";

    if (mode == "explore") {
        auto book = std::make_unique<NonLinearBookImpl>();
        ConsoleProgressData progressData;
        if (!book->load(nlbPath, progressData)) {
            std::cout << "Cannot load book: " << nlbPath << std::endl;
            return 1;
        }
        std::cout << "Loaded: " << book->getTitle() << " by " << book->getAuthor() << std::endl;
        PlayerEngine::Settings settings;
        settings.achievementsPath = achievementsFilePath(std::filesystem::u8path(nlbPath));
        const std::string exportMode = positional.size() > 2 ? positional[2] : "vn";
        settings.exportMode = (exportMode == "standard")
            ? PlayerEngine::ExportMode::Standard : PlayerEngine::ExportMode::VN;
        NLBExplorer explorer(book.get(), book->getStartPoint(), settings);
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
