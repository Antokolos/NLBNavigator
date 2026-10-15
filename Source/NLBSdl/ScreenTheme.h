#pragma once

#include <SDL3/SDL.h>

#include <string>

/**
 * @brief Оформление экрана как в теме INSTEAD (theme.ini + theme_standard.lua / theme_vn.lua).
 *
 * Тема принадлежит не книге NLB, а INSTEAD-экспорту (у каждой игры может быть своя), поэтому
 * проигрыватель берёт её из каталога темы (--theme <каталог>, например themes/default из
 * экспорта игры), а без него использует тему по умолчанию экспорта NLB (NLBW/res/vnstead/themes/default).
 */
struct ScreenTheme {
    struct Window {
        SDL_FRect rect;
        SDL_Color fg, link, alink;
        std::string fontFile;   ///< путь к .ttf (пусто — шрифт проигрывателя)
        float fontSize = 24;
        float lineHeight = 1.0f;  ///< fnt.height: множитель высоты строки
        std::string upImage, downImage;  ///< gfx.up / gfx.down — стрелки прокрутки
    };
    float screenW = 1920, screenH = 1080;
    SDL_Color background{0x01, 0x13, 0x22, 255};
    std::string backgroundImage;      ///< scr.gfx.bg, если это картинка (а не box:…)
    SDL_FRect pictureArea{40, 30, 1787, 640};  ///< scr.gfx.x/y/w/h — картинка страницы (STANDARD)
    Window text;                      ///< win.*
    Window inventory;                 ///< inv.*
    bool inventoryHorizontal = true;  ///< inv.mode = horizontal-*
    std::string align = "justify";    ///< win.align: left, center, right, justify (по умолчанию INSTEAD)
    std::string menuButton;           ///< menu.gfx.button
    float menuButtonX = 0, menuButtonY = 0;  ///< menu.button.x/y
    std::string frameLeft, frameRight;       ///< gfx/fl.png, gfx/fr.png — рамки окна текста VN

    /// Тема по умолчанию экспорта NLB: standard или vn
    static ScreenTheme defaults(bool visualNovel);
    /// Загрузить тему из каталога (theme.ini, затем theme_standard.lua или theme_vn.lua);
    /// чего нет в файлах — остаётся по умолчанию
    static ScreenTheme load(const std::string& dir, bool visualNovel);
    /// Цвет INSTEAD: имя (black, brown, orange, white, gold…) или #RRGGBB
    static SDL_Color color(const std::string& value, SDL_Color fallback);
};
