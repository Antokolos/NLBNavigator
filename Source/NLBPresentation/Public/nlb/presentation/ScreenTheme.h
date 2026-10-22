#pragma once

#include <cstdint>
#include <string>

namespace nlb::presentation {

/// Цвет RGBA
struct Color { uint8_t r = 0, g = 0, b = 0, a = 255; };
/// Прямоугольник на экране 1920×1080
struct RectF { float x = 0, y = 0, w = 0, h = 0; };

/**
 * @brief Оформление экрана как в теме INSTEAD (theme.ini + theme_standard.lua / theme_vn.lua).
 *
 * Тема принадлежит не книге NLB, а INSTEAD-экспорту (у каждой игры может быть своя), поэтому
 * проигрыватель берёт её из каталога темы (--theme <каталог>, например themes/default из
 * экспорта игры), а без него использует тему по умолчанию экспорта NLB (NLBW/res/vnstead/themes/default).
 *
 * Базовая тема (--base-theme <каталог>, например стандартная тема INSTEAD «default») задаёт
 * оформление по умолчанию, а тема игры перезаписывает её параметры: сначала читается theme.ini
 * базовой темы, поверх него — theme.ini темы игры, затем theme_standard.lua / theme_vn.lua обеих.
 * Пути к файлам (шрифты, картинки) берутся относительно той темы, где задан параметр.
 * Координаты задаются в экране темы scr.w×scr.h (у стандартной темы INSTEAD — 800×600) и
 * пересчитываются в экран проигрывателя 1920×1080 с полями, как scr.gfx.scalable в INSTEAD.
 */
struct ScreenTheme {
    struct Window {
        RectF rect;
        Color fg, link, alink;
        std::string fontFile;   ///< путь к .ttf (пусто — шрифт проигрывателя)
        float fontSize = 24;
        float lineHeight = 1.0f;  ///< fnt.height: множитель высоты строки
        std::string upImage, downImage;  ///< gfx.up / gfx.down — стрелки прокрутки
        std::string fontItalicFile;  ///< курсив: третий вариант из fnt.name «{обычный,жирный,курсив,…}»
        std::string fontBoldFile;    ///< полужирный: второй вариант (заголовок комнаты в INSTEAD)
        /// win.up.x/y, win.down.x/y (inv.…): левый верхний угол стрелок листания на экране
        /// 1920×1080; -1 — по умолчанию (справа от окна: вверху и внизу)
        float upX = -1, upY = -1, downX = -1, downY = -1;
    };
    float screenW = 1920, screenH = 1080;
    Color background{0x01, 0x13, 0x22, 255};
    Color border{0, 0, 0, 255};       ///< scr.col.brd: вокруг экрана темы и вокруг фона меньше экрана
    /// scr.gfx.mode (game.c INSTEAD): Float — картинка страницы STANDARD в области scr.gfx (pictureArea);
    /// Fixed — в окне текста под заголовком, не прокручивается; Embedded — в тексте сцены, прокручивается
    enum class GfxMode { Float, Fixed, Embedded };
    GfxMode gfxMode = GfxMode::Float;
    float gfxMaxHeight = -1;          ///< scr.gfx.h для Fixed/Embedded: наибольшая высота картинки; -1 — без предела
    std::string backgroundImage;      ///< scr.gfx.bg, если это картинка (а не box:…)
    /// Фон страниц STANDARD: экспорт задаёт каждой такой странице bgimg = nlb:std_bg() —
    /// gfx/bg.jpg активной темы (темы игры, иначе базовой), если у страницы нет своего фона.
    /// Он заменяет scr.gfx.bg из theme.ini (поэтому там bg.jpg обычно закомментирован)
    std::string pageBackground;
    RectF pictureArea{40, 30, 1787, 640};  ///< scr.gfx.x/y/w/h — картинка страницы (STANDARD)
    Window text;                      ///< win.*
    Window inventory;                 ///< inv.*
    bool inventoryHorizontal = true;  ///< inv.mode = horizontal-*
    std::string inventoryAlign = "center";  ///< inv.mode = …-left / -center / -right
    std::string align = "justify";    ///< win.align: left, center, right, justify (по умолчанию INSTEAD)
    bool waysBottom = false;          ///< win.ways.mode = bottom: пути под текстом (по умолчанию — над ним)
    std::string menuButton;           ///< menu.gfx.button
    float menuButtonX = 0, menuButtonY = 0;  ///< menu.button.x/y
    std::string frameLeft, frameRight;       ///< gfx/fl.png, gfx/fr.png — рамки окна текста VN
    /// Кнопка меню, которую ставит игра NLB (nlb.lua, _syscall_showmenubtn): gfx/menubtn.png и
    /// gfx/menubtn_ru.png активной темы, в правом верхнем углу (scr.w − ширина, 0)
    std::string nlbMenuButton, nlbMenuButtonRu;
    float scale = 1.0f;               ///< масштаб экрана темы (scr.w×scr.h) к 1920×1080: картинки темы
    std::string clickSound;           ///< snd.click — звук щелчка по ссылке (стандартная тема INSTEAD)
    RectF screenArea{0, 0, 1920, 1080};  ///< где экран темы на экране 1920×1080 (фон scr.gfx.bg — сюда)

    /// Тема по умолчанию экспорта NLB: standard или vn
    static ScreenTheme defaults(bool visualNovel);
    /// Загрузить тему из каталога (theme.ini, затем theme_standard.lua или theme_vn.lua);
    /// чего нет в файлах — остаётся по умолчанию
    static ScreenTheme load(const std::string& dir, bool visualNovel);
    /// Базовая тема и поверх неё тема игры (любая из них может быть пустой)
    static ScreenTheme load(const std::string& baseDir, const std::string& gameDir, bool visualNovel);
    /// Цвет INSTEAD: имя (black, brown, orange, white, gold…) или #RRGGBB
    static Color color(const std::string& value, Color fallback);
};

}  // namespace nlb::presentation
