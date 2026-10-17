#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

/**
 * @file ScreenModel.h
 * @brief Снимок экрана: что должно быть видно игроку прямо сейчас. ЧЕРНОВИК ДЛЯ СОГЛАСОВАНИЯ.
 *
 * Модель декларативная (retained): движок после каждой команды и каждого тика времени отдаёт
 * новый снимок, а фронтенд (консоль, SDL3, Godot, Unreal) приводит экран в соответствие с ним.
 * Сам снимок ничего не знает о способе вывода. Разовые вещи (звуковой эффект, достижение,
 * переход во внешнюю игру) приходят отдельно — событиями (PlayerEngine::Event).
 *
 * Источник истины для всех полей — поведение INSTEAD-экспорта (STEAD/VNSTEAD), как и раньше.
 * Имена в комментариях вида «файл effect» — файлы объекта в каталоге книги (ObjImpl.java),
 * «eff/arm/iarm/maxStep» — поля gobj в экспортированном коде и vn.lua.
 */
namespace nlb::presentation {

/// Ссылка на медиафайл книги: логический путь ("images/029.1.png"), уже с учётом выбора по тегу
/// и перенаправлений (.constrid/.redirect). Как превратить его в текстуру — забота IMediaSource
/// и фронтенда (в Godot — res://, в Unreal — ассет)
struct MediaRef {
    std::string path;
    bool empty() const { return path.empty(); }
};

/// Картинка, в том числе анимированная. Флаг «Animated image» (файл animated у страницы и
/// объекта): число перед расширением — количество кадров, intro-1478.png -> кадры
/// intro-1.png .. intro-1478.png, которые сменяются по таймеру
struct ImageRef {
    MediaRef file;                ///< статичная картинка; для анимации — пусто
    std::string framePattern;     ///< "images/intro-%d.png" для анимации, иначе пусто
    int frameCount = 0;           ///< 0 — не анимация
    int currentFrame = 0;         ///< 1..frameCount: какой кадр показывать сейчас (по таймеру)
    bool looped = false;          ///< «Looped» (файл looped): после последнего кадра — снова первый
    bool empty() const { return file.empty() && framePattern.empty(); }
};

/// Угол экрана, от которого отсчитывается положение графического объекта (coords-origin)
enum class Anchor { LeftTop, MiddleTop, RightTop, LeftMiddle, MiddleMiddle, RightMiddle,
                    LeftBottom, MiddleBottom, RightBottom };

/// Положение графического объекта так, как его задаёт экспорт (decorateObjEffect, vn.lua).
/// Как vn.lua (postoxy) переводит это в точку экрана для корневого объекта размером vw×vh:
///   x: left — armX; right — 1920 − vw; middle — (1920 − vw) / 2; затем + offsetX
///   y: top — armY; bottom — 1080 − vh; middle — (1080 − vh) / 2; затем + offsetY
/// Вложенный: точка родителя + свой arm (iarm или столбик).
/// Корневой объект: угол экрана + offset (если задан) или arm (координаты редактора,
/// пересчитанные на экран 1920×1080). Вложенный: eff с 0,0, а положение внутри родителя —
/// iarm (координаты редактора относительно родителя или его offset, без пересчёта).
/// Фронтенд сам переводит это в свои координаты и масштаб
struct Placement {
    Anchor anchor = Anchor::LeftTop;   ///< «Coords origin» (файл coordsor)
    bool hasOffset = false;
    int offsetX = 0, offsetY = 0;      ///< «Offset» (файл offset): eff = "<угол>@<x>,<y>"
    int armX = 0, armY = 0;            ///< arm = {[0] = {x, y}}
    bool hasInnerArm = false;          ///< вложен в объект в самой книге: положение задано iarm
    int innerArmX = 0, innerArmY = 0;  ///< iarm = {[0] = {x, y}}
    // Вложенный объект без iarm (положен в контейнер во время игры) vn.lua ставит в столбик:
    // arm = {0, сумма высот предыдущих таких детей}. Фронтенд раскладывает их так же
};

/// Эффект появления графического объекта («Effect», файл effect). В экспорте — приставка к eff:
/// "fadein-left-top@1520,40", "moveinleft-...". Проигрывается шагами от curStep до maxStep
enum class AppearEffect {
    None,     ///< просто показать
    MoveIn,   ///< выезжает из-за края экрана (направление — MoveDirection)
    MoveOut,  ///< уезжает за край экрана
    FadeIn,   ///< появляется из прозрачности
    FadeOut,  ///< уходит в прозрачность
    ZoomIn,   ///< разворачивается из точки
    ZoomOut,  ///< сворачивается в точку
    Overlap,  ///< в vn.lua рисуется как None, но не вызывает перерисовку того, что под ним
};

/// Направление движения («Movement direction», файл mvdirection): откуда выезжает или куда
/// уезжает объект при MoveIn/MoveOut (и с какой стороны для остальных эффектов)
enum class MoveDirection { None, Left, Right, Top, Bottom };

/// Анимация появления и кадров графического объекта (поля gobj: maxStep, startFrame, curStep)
struct Appearance {
    AppearEffect effect = AppearEffect::None;
    MoveDirection direction = MoveDirection::None;
    int maxStep = 0;        ///< шагов эффекта: кадров анимированной картинки или «Max frame» (maxframe)
    int startFrame = 0;     ///< «Start frame» (startframe)
    int curStep = 0;        ///< начальный шаг: startFrame, иначе 0 для FadeIn/FadeOut/None и 2 для прочих
    int pauseFrames = 0;    ///< «Pause frames» (pausef): пауза между циклами
    int preloadFrames = 0;  ///< «Preload frames» (preloadf): сколько кадров подгрузить заранее
};

/// Фрагмент текста. Текст страницы приходит последовательностью фрагментов вместо строки
/// с метками [..]: графическому интерфейсу нужно знать, что кликабельно и куда ведёт
struct TextRun {
    enum class Kind {
        Plain,        ///< обычный текст
        ObjectLink,   ///< {метка} из описания объекта: щелчок = действие с объектом target
        ChoiceLink,   ///< ссылка-переход внутри текста (xact): target — номер варианта
    };
    Kind kind = Kind::Plain;
    std::string text;
    std::string target;
};

/// Абзац ("^" и переводы строк из книги уже разбиты на абзацы)
struct Paragraph {
    std::vector<TextRun> runs;
    /// Оформление абзаца (раскладка экрана в духе INSTEAD; снимок движка их не задаёт)
    enum class Style { Normal, Title, Emphasis };
    Style style = Style::Normal;   ///< Title — заголовок комнаты (цвет ссылки), Emphasis — курсив (txtem)
    bool centered = false;         ///< по центру (заголовок, пути — как в INSTEAD)
    float spacing = 0;             ///< пустой абзац: отступ в пикселях (0 — полстроки)
};

/// Вариант перехода (ссылка страницы, служебные ссылки модулей и autowired)
struct Choice {
    int index = 0;            ///< номер для PlayerEngine::choose()
    std::string text;
    bool enabled = true;      ///< false — ссылка с ограничением: показывается её alt-текст
    bool trivial = false;     ///< текст по умолчанию («Далее»/«Next»): просто «продолжить»
    bool inlineLink = false;  ///< в стандартной теме — ссылка внизу текста (xact), а не путь
    std::string altText;
};

/// Графический или текстовый объект на странице (сцене)
struct SceneObject {
    std::string id;                 ///< экземпляр (для клонов — "proto#n")
    std::string parentId;           ///< объект-контейнер; пусто — лежит прямо на странице
    std::string label;              ///< подпись: метка {..}, disp или текст действия
    bool graphical = false;         ///< gobj: рисуется картинкой в своём месте экрана
    ImageRef image;                 ///< картинка объекта (с учётом тега объекта)
    ImageRef hoverImage;            ///< morphover: что показать под курсором; пусто — нет
    Placement placement;
    Appearance appearance;
    int zOrder = 0;                 ///< порядок отрисовки: родитель раньше детей
    bool visible = true;            ///< ограничение объекта выполнено
    bool collapsed = false;         ///< collapsable-свиток свёрнут: сам и содержимое не видны
    bool active = true;             ///< щелчок что-то делает
    bool takable = false;
    bool preserved = false;         ///< «Preserved» (preserved): остаётся при смене страниц (VN)
    std::string tooltip;
    // Подсказки отрисовки из книги; фронтенд может их учитывать или игнорировать
    bool showOnCursor = false;      ///< «Show on cursor» (showoncur): объект следует за курсором
    bool clearUnderTooltip = false; ///< clearutt: очищать область под подсказкой
    bool actOnKey = false;          ///< actonkey: срабатывает по клавише
    bool loadOnce = false;          ///< loadonce: кадры загружаются один раз
    bool cacheText = false;         ///< cachetxt: кэшировать отрисованный текст
    bool noRedrawOnAct = false;     ///< noactredraw: не перерисовывать по щелчку
};

/// Предмет инвентаря
struct InventoryItem {
    std::string id;
    std::string parentId;           ///< вложенный предмет (кнопки боя Frontier внутри панели)
    std::string label;
    ImageRef icon;
    bool active = true;             ///< false — только для чтения (STAT, надписи)
};

/// Оформление окна текста и инвентаря: модификации WINGEOM/INVGEOM/WINCOLOR/INVCOLOR
/// и геометрия текстового окна VN (vn:geom). Пустые значения — по умолчанию фронтенда
struct WindowStyle {
    std::optional<std::string> textGeometry;       ///< WINGEOM, как задано в книге
    std::optional<std::string> inventoryGeometry;  ///< INVGEOM
    std::optional<std::string> textColor;          ///< WINCOLOR
    std::optional<std::string> inventoryColor;     ///< INVCOLOR
};

enum class Layout {
    Standard,     ///< тема STANDARD: картинка, текст, ссылки, инвентарь
    VisualNovel,  ///< тема VN: фон на весь экран, текстовое окно, графические объекты
};

struct ScreenModel {
    uint64_t revision = 0;          ///< растёт при каждом изменении: фронтенд может пропускать одинаковые
    Layout layout = Layout::Standard;
    std::string pageId;
    std::string caption;            ///< заголовок страницы (если показывается)
    /// Название комнаты, как nam в экспорте STEAD: заголовок страницы (если показывается), иначе
    /// название модуля (книги), где она лежит, иначе «...» — INSTEAD выводит его над текстом
    std::string roomTitle;

    ImageRef pageImage;             ///< картинка страницы
    bool imageIsBackground = false; ///< imagebg: картинка — фон всего экрана

    std::vector<Paragraph> pageText;      ///< текст страницы и описания объектов
    std::vector<Paragraph> reactionText;  ///< ответ на последнее действие (lasttext)
    // То же по частям — для раскладки как в INSTEAD (STEAD: сцена = описание комнаты ^^ описания
    // объектов через пробел; после действия — курсивом ответ ^^ описания объектов)
    std::vector<Paragraph> roomText;      ///< текст страницы (по строкам)
    std::vector<Paragraph> objectsText;   ///< описания объектов — одним абзацем, через пробел
    std::vector<Paragraph> modText;       ///< текст модификаций страницы
    std::vector<Choice> choices;
    std::vector<SceneObject> sceneObjects;
    std::vector<InventoryItem> inventory;
    WindowStyle style;

    bool timerActive = false;       ///< у страницы есть таймер: время должно идти
    int timerIntervalMs = 200;      ///< как timer:set(200) в экспорте; у анимации 40 (nlb._fps = 25)
    bool finished = false;          ///< ходов больше нет
    bool canSave = true;            ///< nosave
    std::optional<std::string> externalGame;  ///< идёт внешняя игра (мини-игра, 3D-сцена)
};

}  // namespace nlb::presentation
