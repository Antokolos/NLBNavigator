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
 */
namespace nlb::presentation {

/// Ссылка на медиафайл книги: логический путь ("images/029.1.png"), уже с учётом выбора по тегу
/// и перенаправлений (.constrid/.redirect). Как превратить его в текстуру — забота IMediaSource
/// и фронтенда (в Godot — res://, в Unreal — ассет)
struct MediaRef {
    std::string path;
    bool empty() const { return path.empty(); }
};

/// Картинка, в том числе анимированная (флаг animated: intro-1478.png -> кадры intro-1..1478)
struct ImageRef {
    MediaRef file;                ///< статичная картинка; для анимации — пусто
    std::string framePattern;     ///< "images/intro-%d.png" для анимации, иначе пусто
    int frameCount = 0;           ///< 0 — не анимация
    int currentFrame = 0;         ///< 1..frameCount: какой кадр показывать сейчас (по таймеру)
    bool empty() const { return file.empty() && framePattern.empty(); }
};

/// Угол экрана, от которого отсчитывается положение графического объекта (coords-origin)
enum class Anchor { LeftTop, MiddleTop, RightTop, LeftMiddle, MiddleMiddle, RightMiddle,
                    LeftBottom, MiddleBottom, RightBottom };

/// Положение графического объекта так, как его задаёт экспорт (decorateObjEffect, vn.lua):
/// корневой — угол + (offset или arm), вложенный — относительно родителя (arm). Экран 1920×1080.
/// Фронтенд сам переводит это в свои координаты и масштаб
struct Placement {
    Anchor anchor = Anchor::LeftTop;
    bool hasOffset = false;
    int offsetX = 0, offsetY = 0;  ///< eff = "<угол>@<x>,<y>"
    int armX = 0, armY = 0;        ///< arm = {[0] = {x, y}}
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
};

/// Вариант перехода (ссылка страницы, служебные ссылки модулей и autowired)
struct Choice {
    int index = 0;            ///< номер для PlayerEngine::choose()
    std::string text;
    bool enabled = true;      ///< false — ссылка с ограничением: показывается её alt-текст
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
    int zOrder = 0;                 ///< порядок отрисовки: родитель раньше детей
    bool visible = true;            ///< ограничение объекта выполнено
    bool collapsed = false;         ///< collapsable-свиток свёрнут: сам и содержимое не видны
    bool active = true;             ///< щелчок что-то делает
    bool takable = false;
    bool preserved = false;         ///< остаётся на экране при смене страниц (VN)
    std::string tooltip;
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

    ImageRef pageImage;             ///< картинка страницы
    bool imageIsBackground = false; ///< imagebg: картинка — фон всего экрана

    std::vector<Paragraph> pageText;      ///< текст страницы и описания объектов
    std::vector<Paragraph> reactionText;  ///< ответ на последнее действие (lasttext)
    std::vector<Choice> choices;
    std::vector<SceneObject> sceneObjects;
    std::vector<InventoryItem> inventory;
    WindowStyle style;

    bool timerActive = false;       ///< у страницы есть таймер: время должно идти
    int timerIntervalMs = 200;      ///< как timer:set() в экспорте (200, у быстрой анимации 1)
    bool finished = false;          ///< ходов больше нет
    bool canSave = true;            ///< nosave
    std::optional<std::string> externalGame;  ///< идёт внешняя игра (мини-игра, 3D-сцена)
};

}  // namespace nlb::presentation
