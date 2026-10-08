#include "shunting-yard.h"
#include "builtin-features.inc"

#include "nlb/domain/NLBReader.h"
#include "nlb/domain/NLBExplorer.h"
#include "nlb/api/ConsoleProgressData.h"

#include <cstdlib>
#include <filesystem>
#include <memory>

/**
 * @brief Файл прогресса достижений (аналог prefs INSTEAD): ~/.nlbnavigator/<имя каталога книги>.achievements.
 * Если домашний каталог не определить — рядом с книгой.
 */
static std::string achievementsFilePath(const std::string& nlbPath) {
    namespace fs = std::filesystem;
    const char* home = std::getenv("USERPROFILE");
    if (!home) home = std::getenv("HOME");
    std::string bookName = fs::path(nlbPath).lexically_normal().filename().string();
    if (bookName.empty()) bookName = fs::path(nlbPath).lexically_normal().parent_path().filename().string();
    std::error_code ec;
    if (home) {
        fs::path dir = fs::path(home) / ".nlbnavigator";
        fs::create_directories(dir, ec);
        if (!ec) return (dir / (bookName + ".achievements")).string();
    }
    return (fs::path(nlbPath) / ".achievements").string();
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
        settings.achievementsPath = achievementsFilePath(nlbPath);
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
