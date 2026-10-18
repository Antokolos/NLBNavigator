// nlb_sdl — графический проигрыватель книг NLB на SDL3.
//   nlb_sdl <каталог книги | файл.nlbpack> [explore] [vn|standard] [--font <ttf>] [--no-sound]
//           [--choices-screen | --choices-in-text]   (по умолчанию: vn — экран вариантов, standard — в тексте)
//           [--window WxH] [--script <файл>]   (сценарий — для тестов: команды и снимки экрана)
#include "SdlPlayer.h"

// На Android точку входа вызывает SDLActivity (SDL_main из libmain.so) — там SDL_main.h должен
// переименовать main; на ПК main — обычная функция
#ifndef __ANDROID__
#define SDL_MAIN_HANDLED
#endif
#include <SDL3/SDL_main.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <vector>

#include "PlatformPaths.h"
#include "nlb/util/BookPack.h"
#include "nlb/util/FileUtils.h"
#include "shunting-yard.h"
#include "builtin-features.inc"

namespace {
/// Каталог данных: ~/.nlbnavigator (как у nlb_main), на Android — хранилище приложения
std::filesystem::path dataRoot() {
#ifdef __ANDROID__
    char* pref = SDL_GetPrefPath("NLBNavigator", "data");
    if (!pref) return {};
    std::filesystem::path p = FileUtils::pathFromUtf8(pref);
    SDL_free(pref);
    return p;
#else
    const std::filesystem::path home = homeDir();
    return home.empty() ? home : home / ".nlbnavigator";
#endif
}

/// Встроенная книга (самодостаточная игра): book.nlbpack рядом с программой, а на Android — в
/// assets приложения (или в install-time asset pack). На Android пакет при первом запуске (и когда
/// он в приложении сменился — по размеру) копируется в хранилище приложения: плеер читает книгу
/// обычным файловым вводом-выводом. Пусто — встроенной книги нет
std::string embeddedBook() {
    const char* NAME = "book.nlbpack";
#ifdef __ANDROID__
    SDL_IOStream* in = SDL_IOFromFile(NAME, "rb");  // SDL на Android читает и из assets
    if (!in) return "";
    const Sint64 size = SDL_GetIOSize(in);
    const std::filesystem::path target = dataRoot() / NAME;
    const std::string targetUtf8 = FileUtils::pathToUtf8(target);
    std::error_code ec;
    if (size <= 0 || !std::filesystem::exists(target, ec) || static_cast<Sint64>(std::filesystem::file_size(target, ec)) != size) {
        const std::filesystem::path tmp = dataRoot() / "book.nlbpack.part";
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        std::vector<char> buf(1 << 20);
        size_t got = 0;
        while ((got = SDL_ReadIO(in, buf.data(), buf.size())) > 0) out.write(buf.data(), static_cast<std::streamsize>(got));
        out.close();
        std::filesystem::rename(tmp, target, ec);
        if (ec) { SDL_CloseIO(in); return ""; }
    }
    SDL_CloseIO(in);
    return targetUtf8;
#else
    const char* base = SDL_GetBasePath();  // каталог программы (строку SDL хранит сам)
    if (!base) return "";
    const std::string path = FileUtils::combinePath(base, NAME);
    return FileUtils::exists(path) ? path : std::string();
#endif
}

/// <каталог данных>/<имя книги>.<suffix>
std::string dataPath(const std::string& bookPath, const std::string& suffix, bool directory) {
    namespace fs = std::filesystem;
    try {
        fs::path book = FileUtils::pathFromUtf8(bookPath).lexically_normal();
        fs::path name = book.filename();
        if (name.empty()) name = book.parent_path().filename();
        if (name.extension() == ".nlbpack") name = name.stem();
        fs::path dir = dataRoot();
        if (dir.empty()) return "";
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
    int choicesScreen = -1;  // -1 — по режиму экспорта: vn — отдельным экраном, standard — в тексте
    options.args = args;  // EOS: -AUTH_PASSWORD и др. от Epic Games Launcher
    for (size_t i = 1; i < args.size(); ++i) {
        const std::string& a = args[i];
        if (a == "--font" && i + 1 < args.size()) options.fontPath = args[++i];
        else if (a == "--theme" && i + 1 < args.size()) options.themeDir = args[++i];
        else if (a == "--base-theme" && i + 1 < args.size()) options.baseThemeDir = args[++i];
        else if (a == "--no-sound") options.sound = false;
        else if (a == "--choices-screen") choicesScreen = 1;
        else if (a == "--choices-in-text") choicesScreen = 0;
        else if (a == "--fullscreen") options.fullscreen = true;
        else if (a == "--platform" && i + 1 < args.size()) options.platforms.push_back(args[++i]);
        else if (a == "--platform-config" && i + 1 < args.size()) options.platformConfig = args[++i];
        else if (a == "--log" && i + 1 < args.size()) options.logPath = args[++i];
        else if (a.rfind("--log=", 0) == 0) options.logPath = a.substr(6);
        else if (a == "--script" && i + 1 < args.size()) options.scriptPath = args[++i];
        else if (a == "--window" && i + 1 < args.size()) std::sscanf(args[++i].c_str(), "%dx%d", &options.windowWidth, &options.windowHeight);
        else positional.push_back(a);
    }
    // Самодостаточная игра: книга встроена в приложение (book.nlbpack рядом с программой, на
    // Android — в assets). Тогда путь к книге не нужен и другую книгу не открыть: режим (vn/standard)
    // и ключи по-прежнему можно передать
    {
        const bool noBook = positional.empty() || positional[0] == "vn" || positional[0] == "standard" || positional[0] == "explore";
        if (noBook) {
            const std::string book = embeddedBook();
            if (!book.empty()) positional.insert(positional.begin(), book);
        }
    }
#ifdef __ANDROID__
    options.fullscreen = true;  // на телефоне — весь экран
#endif
    if (positional.empty()) {
        std::cout << "Usage: nlb_sdl <book directory | file.nlbpack> [explore] [vn|standard] [--theme <INSTEAD theme dir>] [--base-theme <dir>] [--font <ttf>] [--no-sound]"
                     " [--choices-screen | --choices-in-text] [--window WxH] [--fullscreen] [--log <file>]"
                     " [--platform steam|epic|log[:file]]... [--platform-config <platform.ini>] [--lang <ru|en|…>]" << std::endl;
        std::cout << "  vn (default)  - as exported to VN-INSTEAD; choices on a separate screen by default" << std::endl;
        std::cout << "  standard      - as exported to INSTEAD; choices at the end of the text by default" << std::endl;
        return 1;
    }
    options.bookPath = positional[0];
    // режим: «vn» или «standard» (как у nlb_main explore; само слово explore можно указывать)
    for (size_t i = 1; i < positional.size(); ++i) {
        if (positional[i] == "standard") options.exportMode = PlayerEngine::ExportMode::Standard;
        else if (positional[i] == "vn") options.exportMode = PlayerEngine::ExportMode::VN;
    }
    // Варианты по умолчанию — как в соответствующем экспорте: в VN-INSTEAD текст порциями и экран
    // вариантов, в обычном INSTEAD — ссылки в конце текста. Ключи --choices-screen / --choices-in-text
    // задают явно; в меню режим можно переключить
    options.choicesScreen = choicesScreen >= 0 ? choicesScreen == 1 : options.exportMode == PlayerEngine::ExportMode::VN;
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
    // Базовая тема, зашитая в пакет (pack --base-theme), если другая не указана
    if (options.baseThemeDir.empty() && BookPack::mounted()) {
        const std::string packed = FileUtils::combinePath(options.bookPath, BookPack::BASE_THEME_DIR);
        if (FileUtils::isDirectory(packed)) options.baseThemeDir = packed;
    }
    options.savesDir = dataPath(options.bookPath, ".saves", true);
    options.achievementsPath = dataPath(options.bookPath, ".achievements", false);
    SdlPlayer player(options);
    return player.run();
}
