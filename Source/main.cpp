#include "shunting-yard.h"
#include "builtin-features.inc"

#include "nlb/domain/NLBReader.h"
#include "nlb/domain/NLBExplorer.h"
#include "nlb/api/ConsoleProgressData.h"
#include "PlatformPaths.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>

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
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <path_to_nlb_directory> [mode]" << std::endl;
        std::cout << "Modes: info (default) | explore [vn|standard]" << std::endl;
        std::cout << "  explore vn       - play as exported by exportToVNSTEADFile (default)" << std::endl;
        std::cout << "  explore standard - play as exported by exportToSTEADFile" << std::endl;
        return 1;
    }
    
    std::string nlbPath = argv[1];
    std::string mode = (argc > 2) ? argv[2] : "info";
    
    if (mode == "explore") {
        auto book = std::make_unique<NonLinearBookImpl>();
        ConsoleProgressData progressData;
        if (!book->load(nlbPath, progressData)) {
            std::cout << "Cannot load book: " << nlbPath << std::endl;
            return 1;
        }
        std::cout << "Loaded: " << book->getTitle() << " by " << book->getAuthor() << std::endl;
        PlayerEngine::Settings settings;
        settings.achievementsPath = achievementsFilePath(bookPathArg(argv[1]));
        const std::string exportMode = (argc > 3) ? argv[3] : "vn";
        settings.exportMode = (exportMode == "standard")
            ? PlayerEngine::ExportMode::Standard : PlayerEngine::ExportMode::VN;
        NLBExplorer explorer(book.get(), book->getStartPoint(), settings);
        explorer.explore();
    } else {
        NLBReader reader;
        reader.readBook(nlbPath);
    }
    
    return 0;
}
