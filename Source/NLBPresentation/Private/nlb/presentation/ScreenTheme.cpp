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
        {"goldenrod", {218, 165, 32, 255}}, {"darkgoldenrod", {184, 134, 11, 255}}, {"lime", {0, 255, 0, 255}},
        {"aqua", {0, 255, 255, 255}}, {"cyan", {0, 255, 255, 255}}, {"fuchsia", {255, 0, 255, 255}},
        {"magenta", {255, 0, 255, 255}}, {"purple", {128, 0, 128, 255}}, {"olive", {128, 128, 0, 255}},
        {"teal", {0, 128, 128, 255}}, {"darkgray", {169, 169, 169, 255}}, {"darkgrey", {169, 169, 169, 255}},
        {"lightgray", {211, 211, 211, 255}}, {"lightgrey", {211, 211, 211, 255}}, {"pink", {255, 192, 203, 255}},
        {"violet", {238, 130, 238, 255}}, {"darkblue", {0, 0, 139, 255}}, {"darkred", {139, 0, 0, 255}},
        {"darkgreen", {0, 100, 0, 255}}, {"lightgreen", {144, 238, 144, 255}}, {"skyblue", {135, 206, 235, 255}},
        {"tan", {210, 180, 140, 255}}, {"wheat", {245, 222, 179, 255}}, {"beige", {245, 245, 220, 255}},
        {"ivory", {255, 255, 240, 255}}, {"khaki", {240, 230, 140, 255}}, {"salmon", {250, 128, 114, 255}},
        {"tomato", {255, 99, 71, 255}}, {"coral", {255, 127, 80, 255}}, {"chocolate", {210, 105, 30, 255}},
        {"sienna", {160, 82, 45, 255}}, {"crimson", {220, 20, 60, 255}}, {"indigo", {75, 0, 130, 255}},
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
    return load("", dir, visualNovel);
}

namespace {
/// Значение theme.ini и каталог темы, где оно задано (пути к файлам — относительно него)
struct IniValue {
    std::string value;
    std::string dir;
};

void readIni(const std::string& dir, std::map<std::string, IniValue>& ini) {
    if (dir.empty() || !FileUtils::isDirectory(dir)) return;
    std::istringstream in(readText(FileUtils::combinePath(dir, "theme.ini")));
    for (std::string line; std::getline(in, line);) {
        line = trim(line);
        if (line.empty() || line[0] == ';') continue;
        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string value = line.substr(eq + 1);
        const auto comment = value.find(';');
        if (comment != std::string::npos) value = value.substr(0, comment);
        ini[trim(line.substr(0, eq))] = IniValue{trim(value), dir};  // тема игры перезаписывает базовую
    }
}
}

ScreenTheme ScreenTheme::load(const std::string& baseDir, const std::string& gameDir, bool visualNovel) {
    ScreenTheme t = defaults(visualNovel);
    std::map<std::string, IniValue> ini;
    readIni(baseDir, ini);
    readIni(gameDir, ini);
    auto has = [&](const std::string& k) { return ini.count(k) != 0; };
    auto str = [&](const std::string& k) { return ini.at(k).value; };
    auto path = [&](const std::string& k, const std::string& name) { return FileUtils::combinePath(ini.at(k).dir, name); };

    // Экран темы scr.w×scr.h -> 1920×1080 с полями: координаты и размеры шрифтов из theme.ini
    float sw = 1920, sh = 1080;
    if (has("scr.w")) try { sw = std::stof(str("scr.w")); } catch (...) {}
    if (has("scr.h")) try { sh = std::stof(str("scr.h")); } catch (...) {}
    if (sw <= 0 || sh <= 0) { sw = 1920; sh = 1080; }
    const float k = std::min(1920.0f / sw, 1080.0f / sh);
    const float ox = (1920.0f - sw * k) / 2, oy = (1080.0f - sh * k) / 2;
    t.screenW = sw;
    t.screenH = sh;
    t.scale = k;
    t.screenArea = {ox, oy, sw * k, sh * k};
    auto raw = [&](const std::string& key, float& v) -> bool {
        if (!has(key)) return false;
        try { v = std::stof(str(key)); return true; } catch (...) { return false; }
    };
    auto posX = [&](const std::string& key, float& v) { float r; if (raw(key, r)) v = ox + r * k; };
    auto posY = [&](const std::string& key, float& v) { float r; if (raw(key, r)) v = oy + r * k; };
    auto len = [&](const std::string& key, float& v) { float r; if (raw(key, r) && r >= 0) v = r * k; };
    auto num = [&](const std::string& key, float& v) { raw(key, v); };
    auto col = [&](const std::string& key, Color& c) { if (has(key)) c = color(str(key), c); };

    col("scr.col.bg", t.background);
    if (has("scr.gfx.bg")) {
        const std::string bg = str("scr.gfx.bg");
        if (bg.rfind("box:", 0) == 0) {
            const auto comma = bg.find(',');
            if (comma != std::string::npos) t.background = color(bg.substr(comma + 1), t.background);
        } else {
            t.backgroundImage = path("scr.gfx.bg", bg);
        }
    }
    posX("scr.gfx.x", t.pictureArea.x); posY("scr.gfx.y", t.pictureArea.y);
    len("scr.gfx.w", t.pictureArea.w); len("scr.gfx.h", t.pictureArea.h);
    for (auto [prefix, w] : {std::pair<std::string, Window*>{"win", &t.text}, {"inv", &t.inventory}}) {
        posX(prefix + ".x", w->rect.x); posY(prefix + ".y", w->rect.y);
        len(prefix + ".w", w->rect.w); len(prefix + ".h", w->rect.h);
        col(prefix + ".col.fg", w->fg); col(prefix + ".col.link", w->link); col(prefix + ".col.alink", w->alink);
        len(prefix + ".fnt.size", w->fontSize);
        num(prefix + ".fnt.height", w->lineHeight);
        if (has(prefix + ".gfx.up")) w->upImage = path(prefix + ".gfx.up", str(prefix + ".gfx.up"));
        if (has(prefix + ".gfx.down")) w->downImage = path(prefix + ".gfx.down", str(prefix + ".gfx.down"));
        if (has(prefix + ".fnt.name")) w->fontFile = path(prefix + ".fnt.name", firstFontVariant(str(prefix + ".fnt.name")));
    }
    if (has("inv.mode")) t.inventoryHorizontal = str("inv.mode").rfind("horizontal", 0) == 0;
    if (has("win.align")) t.align = str("win.align");
    if (has("menu.gfx.button")) t.menuButton = path("menu.gfx.button", str("menu.gfx.button"));
    posX("menu.button.x", t.menuButtonX); posY("menu.button.y", t.menuButtonY);

    // nlb:std_bg(): gfx/bg.jpg активной темы — фон страниц STANDARD без своего фона
    for (const std::string& dir : {baseDir, gameDir}) {
        if (dir.empty() || !FileUtils::isDirectory(dir)) continue;
        const std::string bg = FileUtils::combinePath(dir, "gfx/bg.jpg");
        if (FileUtils::exists(bg)) t.pageBackground = bg;
    }

    // Рамки текстового окна VN и theme_standard.lua / theme_vn.lua — базовой темы, затем темы игры
    for (const std::string& dir : {baseDir, gameDir}) {
        if (dir.empty() || !FileUtils::isDirectory(dir)) continue;
        if (FileUtils::exists(FileUtils::combinePath(dir, "gfx/fl.png"))) t.frameLeft = FileUtils::combinePath(dir, "gfx/fl.png");
        if (FileUtils::exists(FileUtils::combinePath(dir, "gfx/fr.png"))) t.frameRight = FileUtils::combinePath(dir, "gfx/fr.png");
        // вызовы theme.win.geom(…), theme.win.color(…), theme.set(…) — в координатах экрана темы
        const std::string lua = readText(FileUtils::combinePath(dir, visualNovel ? "theme_vn.lua" : "theme_standard.lua"));
        const std::regex geom(R"(theme\.(win|inv)\.geom\(\s*([-0-9.]+)\s*,\s*([-0-9.]+)\s*,\s*([-0-9.]+)\s*,\s*([-0-9.]+)\s*\))");
        for (auto it = std::sregex_iterator(lua.begin(), lua.end(), geom); it != std::sregex_iterator(); ++it) {
            Window& w = (*it)[1] == "win" ? t.text : t.inventory;
            w.rect = {ox + std::stof((*it)[2]) * k, oy + std::stof((*it)[3]) * k, std::stof((*it)[4]) * k, std::stof((*it)[5]) * k};
        }
        const std::regex colors(R"(theme\.(win|inv)\.color\(\s*'([^']*)'\s*,\s*'([^']*)'\s*,\s*'([^']*)'\s*\))");
        for (auto it = std::sregex_iterator(lua.begin(), lua.end(), colors); it != std::sregex_iterator(); ++it) {
            Window& w = (*it)[1] == "win" ? t.text : t.inventory;
            w.fg = color((*it)[2], w.fg); w.link = color((*it)[3], w.link); w.alink = color((*it)[4], w.alink);
        }
        const std::regex setSize(R"(theme\.set\('(win|inv)\.fnt\.size'\s*,\s*([0-9.]+)\))");
        for (auto it = std::sregex_iterator(lua.begin(), lua.end(), setSize); it != std::sregex_iterator(); ++it) {
            ((*it)[1] == "win" ? t.text : t.inventory).fontSize = std::stof((*it)[2]) * k;
        }
        const std::regex setFont(R"(theme\.set\('(win|inv)\.fnt\.name'\s*,\s*(?:tr\s*\.\.\s*)?'([^']*)'\))");
        for (auto it = std::sregex_iterator(lua.begin(), lua.end(), setFont); it != std::sregex_iterator(); ++it) {
            ((*it)[1] == "win" ? t.text : t.inventory).fontFile = FileUtils::combinePath(dir, firstFontVariant((*it)[2]));
        }
    }
    return t;
}

}  // namespace nlb::presentation
