// nlb_sdl — графический проигрыватель книг NLB на SDL3.
//   nlb_sdl <каталог книги | файл.nlbpack> [vn|standard] [--font <ttf>] [--no-sound]
//           [--window WxH] [--script <файл>]   (сценарий — для тестов: команды и снимки экрана)
#include "SdlPlayer.h"

#define SDL_MAIN_HANDLED
#include <SDL3/SDL_main.h>

#include <iostream>
#include <memory>

#include "PlatformPaths.h"
#include "nlb/util/BookPack.h"
#include "nlb/util/FileUtils.h"
#include "shunting-yard.h"
#include "builtin-features.inc"

namespace {
/// Каталог ~/.nlbnavigator/<имя книги>.<suffix> (как у консольного nlb_main)
std::string dataPath(const std::string& bookPath, const std::string& suffix, bool directory) {
    namespace fs = std::filesystem;
    try {
        fs::path book = FileUtils::pathFromUtf8(bookPath).lexically_normal();
        fs::path name = book.filename();
        if (name.empty()) name = book.parent_path().filename();
        if (name.extension() == ".nlbpack") name = name.stem();
        const fs::path home = homeDir();
        if (home.empty()) return "";
        fs::path dir = home / ".nlbnavigator";
        std::error_code ec;
        fs::create_directories(dir, ec);
        fs::path result = dir / name;
        result += suffix;
        if (directory) fs::create_directories(result, ec);
        return FileUtils::pathToUtf8(result);
    } catch (const std::exception&) {
        return "";
    }
}
}

int main(int argc, char** argv) {
    SDL_SetMainReady();
    cparse_startup();
    const std::vector<std::string> args = utf8Args(argc, argv);
    SdlPlayer::Options options;
    std::vector<std::string> positional;
    for (size_t i = 1; i < args.size(); ++i) {
        const std::string& a = args[i];
        if (a == "--font" && i + 1 < args.size()) options.fontPath = args[++i];
        else if (a == "--theme" && i + 1 < args.size()) options.themeDir = args[++i];
        else if (a == "--no-sound") options.sound = false;
        else if (a == "--script" && i + 1 < args.size()) options.scriptPath = args[++i];
        else if (a == "--window" && i + 1 < args.size()) std::sscanf(args[++i].c_str(), "%dx%d", &options.windowWidth, &options.windowHeight);
        else positional.push_back(a);
    }
    if (positional.empty()) {
        std::cout << "Usage: nlb_sdl <book directory | file.nlbpack> [vn|standard] [--theme <INSTEAD theme dir>] [--font <ttf>] [--no-sound] [--window WxH]" << std::endl;
        return 1;
    }
    options.bookPath = positional[0];
    if (positional.size() > 1 && positional[1] == "standard") options.exportMode = PlayerEngine::ExportMode::Standard;
    // Пакет .nlbpack — монтируется как каталог книги
    if (FileUtils::exists(options.bookPath) && !FileUtils::isDirectory(options.bookPath)) {
        std::string error;
        auto pack = BookPack::read(options.bookPath, error);
        if (!pack) { std::cerr << error << std::endl; return 1; }
        BookPack::mount(std::shared_ptr<BookPack>(std::move(pack)), options.bookPath);
    }
    // Тема, зашитая в пакет (pack --theme), если другая не указана
    if (options.themeDir.empty() && BookPack::mounted()) {
        const std::string packed = FileUtils::combinePath(options.bookPath, BookPack::THEME_DIR);
        if (FileUtils::isDirectory(packed)) options.themeDir = packed;
    }
    options.savesDir = dataPath(options.bookPath, ".saves", true);
    options.achievementsPath = dataPath(options.bookPath, ".achievements", false);
    SdlPlayer player(options);
    return player.run();
}
