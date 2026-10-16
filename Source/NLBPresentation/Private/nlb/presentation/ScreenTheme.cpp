#include "nlb/presentation/ScreenTheme.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <regex>
#include <sstream>

#include "nlb/util/FileUtils.h"

namespace nlb::presentation {

namespace {
std::string trim(std::string s) {
    s.erase(0, s.find_first_not_of(" \t\r\n"));
    const auto e = s.find_last_not_of(" \t\r\n");
    return e == std::string::npos ? "" : s.substr(0, e + 1);
}

std::string readText(const std::string& path) {
    std::string content;
    FileUtils::readFile(path, content);
    return content;
}

/// "fonts/{sans,sans-b,sans-i,sans-bi}.ttf" -> "fonts/sans.ttf" (обычное начертание)
std::string firstFontVariant(const std::string& name) {
    const auto open = name.find('{');
    const auto close = name.find('}', open == std::string::npos ? 0 : open);
    if (open == std::string::npos || close == std::string::npos) return name;
    std::string inner = name.substr(open + 1, close - open - 1);
    const auto comma = inner.find(',');
    if (comma != std::string::npos) inner = inner.substr(0, comma);
    return name.substr(0, open) + inner + name.substr(close + 1);
}
}

Color ScreenTheme::color(const std::string& raw, Color fallback) {
    static const std::map<std::string, Color> NAMES = {
        {"black", {0, 0, 0, 255}}, {"white", {255, 255, 255, 255}}, {"brown", {165, 42, 42, 255}},
        {"orange", {255, 165, 0, 255}}, {"gold", {255, 215, 0, 255}}, {"red", {255, 0, 0, 255}},
        {"green", {0, 128, 0, 255}}, {"blue", {0, 0, 255, 255}}, {"yellow", {255, 255, 0, 255}},
        {"gray", {128, 128, 128, 255}}, {"grey", {128, 128, 128, 255}}, {"silver", {192, 192, 192, 255}},
        {"navy", {0, 0, 128, 255}}, {"maroon", {128, 0, 0, 255}}, {"lightblue", {173, 216, 230, 255}},
    };
    std::string v = trim(raw);
    std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (v.size() == 7 && v[0] == '#') {
        try {
            const unsigned long rgb = std::stoul(v.substr(1), nullptr, 16);
            return {static_cast<uint8_t>(rgb >> 16), static_cast<uint8_t>((rgb >> 8) & 0xFF), static_cast<uint8_t>(rgb & 0xFF), 255};
        } catch (...) {
            return fallback;
        }
    }
    auto it = NAMES.find(v);
    return it == NAMES.end() ? fallback : it->second;
}

ScreenTheme ScreenTheme::defaults(bool visualNovel) {
    ScreenTheme t;
    if (visualNovel) {
        // themes/default/theme_vn.lua
        t.text = {{248, 838, 1424, 204}, {255, 255, 255, 255}, {0, 255, 0, 255}, {255, 215, 0, 255}, "", 32};
        t.inventory = {{0, 1050, 1920, 30}, {255, 255, 255, 255}, {0, 255, 0, 255}, {255, 215, 0, 255}, "", 24};
    } else {
        // themes/default/theme.ini + theme_standard.lua
        // Цвета — как в реальных экспортах (white / #00FF00 / gold на #011322): в themes/default
        // стоит чёрный текст, рассчитанный на светлый фон gfx/bg.jpg, который там закомментирован
        t.text = {{68, 670, 1704, 301}, {255, 255, 255, 255}, {0, 255, 0, 255}, {255, 215, 0, 255}, "", 24};
        t.inventory = {{68, 982, 1704, 90}, {255, 255, 255, 255}, {0, 255, 0, 255}, {255, 215, 0, 255}, "", 24};
    }
    return t;
}

ScreenTheme ScreenTheme::load(const std::string& dir, bool visualNovel) {
    ScreenTheme t = defaults(visualNovel);
    if (dir.empty() || !FileUtils::isDirectory(dir)) return t;
    std::map<std::string, std::string> ini;
    std::istringstream in(readText(FileUtils::combinePath(dir, "theme.ini")));
    for (std::string line; std::getline(in, line);) {
        line = trim(line);
        if (line.empty() || line[0] == ';') continue;
        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string value = line.substr(eq + 1);
        const auto comment = value.find(';');
        if (comment != std::string::npos) value = value.substr(0, comment);
        ini[trim(line.substr(0, eq))] = trim(value);
    }
    auto num = [&](const char* k, float& v) { auto it = ini.find(k); if (it != ini.end()) try { v = std::stof(it->second); } catch (...) {} };
    auto col = [&](const char* k, Color& c) { auto it = ini.find(k); if (it != ini.end()) c = color(it->second, c); };
    num("scr.w", t.screenW); num("scr.h", t.screenH);
    col("scr.col.bg", t.background);
    if (ini.count("scr.gfx.bg")) {
        const std::string bg = ini["scr.gfx.bg"];
        if (bg.rfind("box:", 0) == 0) {
            const auto comma = bg.find(',');
            if (comma != std::string::npos) t.background = color(bg.substr(comma + 1), t.background);
        } else {
            t.backgroundImage = FileUtils::combinePath(dir, bg);
        }
    }
    num("scr.gfx.x", t.pictureArea.x); num("scr.gfx.y", t.pictureArea.y);
    num("scr.gfx.w", t.pictureArea.w); num("scr.gfx.h", t.pictureArea.h);
    for (auto [prefix, w] : {std::pair<std::string, Window*>{"win", &t.text}, {"inv", &t.inventory}}) {
        num((prefix + ".x").c_str(), w->rect.x); num((prefix + ".y").c_str(), w->rect.y);
        num((prefix + ".w").c_str(), w->rect.w); num((prefix + ".h").c_str(), w->rect.h);
        col((prefix + ".col.fg").c_str(), w->fg); col((prefix + ".col.link").c_str(), w->link);
        col((prefix + ".col.alink").c_str(), w->alink);
        num((prefix + ".fnt.size").c_str(), w->fontSize);
        num((prefix + ".fnt.height").c_str(), w->lineHeight);
        if (ini.count(prefix + ".gfx.up")) w->upImage = FileUtils::combinePath(dir, ini[prefix + ".gfx.up"]);
        if (ini.count(prefix + ".gfx.down")) w->downImage = FileUtils::combinePath(dir, ini[prefix + ".gfx.down"]);
        if (ini.count(prefix + ".fnt.name")) w->fontFile = FileUtils::combinePath(dir, firstFontVariant(ini[prefix + ".fnt.name"]));
    }
    if (ini.count("inv.mode")) t.inventoryHorizontal = ini["inv.mode"].rfind("horizontal", 0) == 0;
    if (ini.count("win.align")) t.align = ini["win.align"];
    if (ini.count("menu.gfx.button")) t.menuButton = FileUtils::combinePath(dir, ini["menu.gfx.button"]);
    num("menu.button.x", t.menuButtonX); num("menu.button.y", t.menuButtonY);
    // рамки текстового окна VN (vn.lua: gfx/fl.png, gfx/fr.png в каталоге темы)
    if (FileUtils::exists(FileUtils::combinePath(dir, "gfx/fl.png"))) t.frameLeft = FileUtils::combinePath(dir, "gfx/fl.png");
    if (FileUtils::exists(FileUtils::combinePath(dir, "gfx/fr.png"))) t.frameRight = FileUtils::combinePath(dir, "gfx/fr.png");

    // theme_standard.lua / theme_vn.lua: вызовы theme.win.geom(…), theme.win.color(…), theme.set(…)
    const std::string lua = readText(FileUtils::combinePath(dir, visualNovel ? "theme_vn.lua" : "theme_standard.lua"));
    std::smatch m;
    const std::regex geom(R"(theme\.(win|inv)\.geom\(\s*([-0-9.]+)\s*,\s*([-0-9.]+)\s*,\s*([-0-9.]+)\s*,\s*([-0-9.]+)\s*\))");
    for (auto it = std::sregex_iterator(lua.begin(), lua.end(), geom); it != std::sregex_iterator(); ++it) {
        Window& w = (*it)[1] == "win" ? t.text : t.inventory;
        w.rect = {std::stof((*it)[2]), std::stof((*it)[3]), std::stof((*it)[4]), std::stof((*it)[5])};
    }
    const std::regex colors(R"(theme\.(win|inv)\.color\(\s*'([^']*)'\s*,\s*'([^']*)'\s*,\s*'([^']*)'\s*\))");
    for (auto it = std::sregex_iterator(lua.begin(), lua.end(), colors); it != std::sregex_iterator(); ++it) {
        Window& w = (*it)[1] == "win" ? t.text : t.inventory;
        w.fg = color((*it)[2], w.fg); w.link = color((*it)[3], w.link); w.alink = color((*it)[4], w.alink);
    }
    const std::regex setSize(R"(theme\.set\('(win|inv)\.fnt\.size'\s*,\s*([0-9.]+)\))");
    for (auto it = std::sregex_iterator(lua.begin(), lua.end(), setSize); it != std::sregex_iterator(); ++it) {
        ((*it)[1] == "win" ? t.text : t.inventory).fontSize = std::stof((*it)[2]);
    }
    const std::regex setFont(R"(theme\.set\('(win|inv)\.fnt\.name'\s*,\s*(?:tr\s*\.\.\s*)?'([^']*)'\))");
    for (auto it = std::sregex_iterator(lua.begin(), lua.end(), setFont); it != std::sregex_iterator(); ++it) {
        ((*it)[1] == "win" ? t.text : t.inventory).fontFile = FileUtils::combinePath(dir, firstFontVariant((*it)[2]));
    }
    return t;
}

}  // namespace nlb::presentation
