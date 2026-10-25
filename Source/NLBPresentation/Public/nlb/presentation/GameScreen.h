#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "nlb/presentation/ScreenModel.h"
#include "nlb/presentation/ScreenTheme.h"
#include "nlb/presentation/Session.h"

namespace nlb::presentation {

/// Шрифт: файл .ttf (пусто — шрифт проигрывателя) и размер в пикселях (как TTF_OpenFont / INSTEAD)
struct FontRef {
    std::string file;
    float size = 32;
    bool operator<(const FontRef& o) const { return file < o.file || (file == o.file && size < o.size); }
};

/**
 * @brief Чем рисует фронтенд (SDL, Unreal, Godot). Координаты — логический экран 1920×1080;
 * вписать его в окно с полями (letterbox) — забота фронтенда.
 *
 * Картинки называются путями: имя медиа книги ("page3.jpg", с учётом тега и кадра) или полный
 * путь к файлу темы (на диске или внутри пакета). Фронтенд сам загружает и кэширует их.
 */
class IScreenRenderer {
public:
    virtual ~IScreenRenderer() = default;
    /// Размер картинки; false — картинки нет
    virtual bool imageSize(const std::string& path, float& w, float& h) = 0;
    virtual void drawImage(const std::string& path, const RectF& dst, float alpha) = 0;
    virtual void fillRect(const RectF& r, Color c) = 0;
    virtual float textWidth(const FontRef& font, const std::string& text) = 0;
    /// Высота строки шрифта (без межстрочного интервала) и шаг между строками
    virtual float fontHeight(const FontRef& font) = 0;
    virtual float lineSkip(const FontRef& font) = 0;
    /// Строка текста: (x, y) — левый верхний угол строки
    virtual void drawText(const FontRef& font, const std::string& text, Color c, float x, float y, float alpha) = 0;
    /// Ограничить рисование прямоугольником; nullptr — снять ограничение
    virtual void setClip(const RectF* clip) = 0;
    /// Сколько пикселей окна приходится на единицу экрана 1920×1080. INSTEAD раскладывает текст
    /// шрифтом уже в масштабе окна (font_size × scale) и округляет высоту строки в пикселях;
    /// рендерер, который делает так же, сообщает масштаб, чтобы раскладка совпала (по умолчанию 1)
    virtual float pixelScale() const { return 1.0f; }
};

/**
 * @brief Экран игры как в INSTEAD — общий для всех фронтендов.
 *
 * Раскладка и поведение: тема INSTEAD (окно текста, инвентарь, цвета, шрифты, кнопка меню),
 * полоса текста VN (vn:geom), выравнивание по ширине, листание страницами, экран вариантов,
 * спрайты по правилам vn.lua с эффектами появления, меню (сохранение, загрузка, звук…),
 * всплывающие достижения. Фронтенд вызывает render() каждый кадр со своим IScreenRenderer
 * и передаёт ввод (mouseMove, click, wheel, key); время — update(ms).
 */
class GameScreen {
public:
    enum class Key { Escape, Space, Enter, PageUp, PageDown, Save, Load, Digit };

    struct Options {
        std::string themeDir;            ///< каталог темы игры INSTEAD (может быть внутри пакета)
        std::string baseThemeDir;        ///< базовая тема: оформление по умолчанию, тема игры его дополняет
        /// Варианты отдельным экраном (как VN в INSTEAD с модулем paginator; только на страницах
        /// с темой VN, на страницах STANDARD — всегда варианты в тексте): текст выдаётся
        /// порциями по абзацам, разделённым пустой строкой, без прокрутки; дочитав последнюю
        /// порцию, игрок видит экран вариантов, а если вариант один и тривиальный («Далее») —
        /// сразу переходит дальше. Иначе — прокрутка и варианты в конце текста
        bool choicesScreen = false;
        bool soundAvailable = true;      ///< есть звук (пункт меню «звук»)
        FontRef uiFont{"", 32}, titleFont{"", 46}, smallFont{"", 26};  ///< шрифты меню и сообщений
        /// Масштаб всех шрифтов экрана (текст, варианты, инвентарь, подписи, меню): 1 — как в теме.
        /// Для маленьких экранов (телефон) — 1.25…2. Выбор игрока в меню («Размер текста»,
        /// сохраняется в settings.ini) главнее
        float textScale = 1.0f;
        /// Монотонное время, мс (эффекты появления, всплывающие сообщения); пусто — steady_clock
        std::function<uint64_t()> clock;
    };

    GameScreen(Session& session, Options options);

    // ---- кадр
    void update(int milliseconds);                 ///< время: таймеры книги и анимация
    void render(IScreenRenderer& r);

    // ---- ввод (координаты экрана 1920×1080)
    void mouseMove(float x, float y);
    void click(float x, float y, bool rightButton);
    void wheel(float lines);                       ///< >0 — вверх (как колесо SDL)
    void key(Key key, int digit = 0);              ///< digit: 1..9 для Key::Digit

    /// После команд, отданных напрямую session.engine() (сценарии тестов)
    void afterCommand();
    /// Какие темы подключены и что из них взято (для журнала фронтенда: разобраться с оформлением)
    std::string describeThemes() const;
    /// Полноэкранный режим — дело фронтенда: если он умеет, пусть даст переключатель и текущее
    /// состояние, тогда в меню появится пункт «Полный экран»
    void setFullscreenHandler(std::function<void(bool)> setFullscreen, std::function<bool()> isFullscreen) {
        m_setFullscreen = std::move(setFullscreen);
        m_isFullscreen = std::move(isFullscreen);
    }
    /// Сейчас показан экран вариантов (режим choicesScreen)
    bool choicesScreenShown() const { return m_showChoices; }
    /// Показано сообщение (конец игры, внешняя игра, ошибка) — для тестов
    bool messageShown() const { return m_menu == Menu::Message; }
    /// Игрок выбрал «Выйти» в меню
    bool quitRequested() const { return m_quit; }
    void clearQuitRequest() { m_quit = false; }
    /// События движка, которые экран забрал из сессии (достижения, внешние игры, конец) — для
    /// фронтенда, которому они тоже нужны (Unreal: делегаты подсистемы)
    void setEventListener(std::function<void(const std::vector<PlayerEngine::Event>&)> listener) { m_eventListener = std::move(listener); }
    /// Внешняя игра: если обработчик задан, экран не показывает сообщение «её нет», а вызывает его
    void setExternalGameHandler(std::function<void(const std::string&)> handler) { m_externalHandler = std::move(handler); }
    /// Центр кликабельной области объекта или предмета на последнем кадре (для тестов)
    bool hitCenter(const std::string& id, bool inventory, float& x, float& y) const;
    /// Центр пункта меню с номером index на последнем кадре (для тестов)
    bool menuItemCenter(int index, float& x, float& y) const;
    /// Центр перехода с номером index (-1 — заголовок комнаты) на последнем кадре (для тестов)
    bool choiceCenter(int index, float& x, float& y) const;

    static constexpr float SCREEN_W = 1920.0f, SCREEN_H = 1080.0f;

private:
    struct Hit {
        enum class Kind { Choice, Object, Inventory, Menu, ScrollUp, ScrollDown, MenuButton };
        RectF rect;
        Kind kind;
        std::string id;
        int index = -1;
    };
    enum class Menu { None, Main, Save, Load, Message, ConfirmQuit, Language };
    struct Effect {
        uint64_t start = 0;
        int steps = 25;
        bool hiding = false;
        SceneObject object;
    };
    struct EffectState { float alpha = 1, dx = 0, dy = 0, scale = 1; bool done = true; };

    void processEvents(const std::vector<PlayerEngine::Event>& events);
    void updateEffects(const ScreenModel& before);
    EffectState effectState(const std::string& id, const RectF& r);
    void drawObject(const SceneObject& o, const RectF& r, const FontRef& labelFont, Color labelColor, bool spriteLabel);
    RectF objectRect(size_t index);
    float renderParagraphs(const std::vector<Paragraph>& text, float x, float y, float width, const FontRef& font,
                           const ScreenTheme::Window& colors, bool draw);
    float renderChoices(float x, float y, float width, const FontRef& font, const ScreenTheme::Window& colors, bool draw);
    void renderTextWindow(const ScreenTheme::Window& window);
    void renderStandard();
    void renderVisualNovel();
    void renderSceneObjects(const ScreenTheme& theme, bool spriteLabels, bool hidden);
    void renderInventory(const ScreenTheme::Window& window);
    void renderVanishing(const FontRef& labelFont, Color labelColor, bool spriteLabel);
    void renderMenuButton(const ScreenTheme& theme);
    void renderChoicesScreen();
    void renderOverlayMessages();
    void renderMenu();
    bool advanceToChoices();
    /// «Дальше» (пробел, щелчок мимо ссылок): следующая строка-страница текста, следующая
    /// порция, а после последней — экран вариантов или переход по единственной тривиальной ссылке
    bool advanceText();
    /// Порции текста (режим экрана вариантов), как экспорт в VN-INSTEAD: строка текста — порция,
    /// длинная строка — по предложениям (больше 100 символов в порции, хвост до 330 — к последней)
    std::vector<std::vector<Paragraph>> portions() const;
    static std::vector<std::vector<Paragraph>> portionsOf(const std::vector<Paragraph>& text);
    /// Единственный доступный вариант и он тривиальный — индекс, иначе -1
    int singleTrivialChoice() const;
    void menuAction(int index);
    FontRef fontFor(const ScreenTheme::Window& w) const { return scaled(FontRef{w.fontFile, w.fontSize}); }
    /// Шрифт в масштабе текста (Options::textScale или выбор игрока)
    FontRef scaled(const FontRef& f) const { return FontRef{f.file, f.size * m_textScale}; }
    float m_textScale = 1.0f;
    float textW(const FontRef& f, const std::string& s) { return m_r->textWidth(f, s); }
    bool imageSize(const ImageRef& image, float& w, float& h);

    Session& m_session;
    Options m_options;
    IScreenRenderer* m_r = nullptr;   ///< рендерер текущего кадра
    ScreenTheme m_standardTheme, m_vnTheme;
    ScreenModel m_screen;

    std::vector<Hit> m_hits;
    std::string m_hoverObject;
    std::function<void(bool)> m_setFullscreen;
    std::function<bool()> m_isFullscreen;
    /// Пункты главного меню по порядку (полный экран — только если фронтенд его умеет)
    enum class MainItem { Save, Load, Restart, Sound, Choices, Fullscreen, Language, TextSize, Quit, Back };
    std::string tr(int key) const;  ///< текст интерфейса на языке книги (Localization.h, Ui)
    std::vector<MainItem> mainItems() const;
    static constexpr int NO_HOVER = -1000000;
    int m_hoverChoice = NO_HOVER;    ///< переход под курсором (номер варианта; -1 — заголовок комнаты)
    std::string m_hoverItem;         ///< предмет инвентаря под курсором
    float m_mouseX = 0, m_mouseY = 0;
    float m_scroll = 0, m_contentHeight = 0, m_lineHeight = 30, m_pageHeight = 0;
    RectF m_textWindow{};
    bool m_moreText = false;
    float m_drawTop = -1e9f, m_drawBottom = 1e9f, m_firstHiddenY = -1;
    bool m_choicesScreen = false, m_showChoices = false;  ///< m_choicesScreen — выбор игрока (меню)
    /// Режим «варианты отдельным экраном» действует только на страницах с темой VN (там его вёл
    /// paginator INSTEAD); на страницах STANDARD — всегда варианты в тексте с прокруткой
    bool choicesMode() const { return m_choicesScreen && m_screen.layout == Layout::VisualNovel; }
    size_t m_portion = 0;            ///< текущая порция текста (режим экрана вариантов)
    bool m_scrollByPages = false;    ///< листание страницами: последняя — без повтора прочитанного
    size_t m_voicedPortion = SIZE_MAX;  ///< порция, на которой уже запущена фраза озвучки
    bool m_voiceOnEnter = false;     ///< новая страница без порций: первая фраза — при отрисовке
    /// Действие привело на другую страницу (ответ «Изучив панно…» и сразу «Больно вам, больно!»):
    /// показать и ответ, и описание новой комнаты (INSTEAD показал бы только комнату)
    bool m_reactionThenRoom = false;
    bool m_morePortions = false;
    Menu m_menu = Menu::None;
    std::string m_message;
    std::vector<std::string> m_toasts;
    uint64_t m_toastUntil = 0;
    bool m_quit = false, m_soundOn = true;
    std::function<void(const std::string&)> m_externalHandler;

    std::map<std::string, Effect> m_effects;
    std::map<std::string, bool> m_shown;
    std::map<std::string, RectF> m_objectRects, m_lastRects;
    std::string m_externalNotified;  ///< внешняя игра, о которой уже сообщили обработчику
    std::function<void(const std::vector<PlayerEngine::Event>&)> m_eventListener;
    uint64_t m_seenRevision = 0;     ///< ревизия сессии, которую экран уже показал
    void pull();                     ///< забрать события и снимок из сессии
    uint64_t now() const;          ///< время экрана: стоит, пока открыто меню (пауза)
    uint64_t rawNow() const;       ///< часы фронтенда
    void updatePause();            ///< меню открылось — время экрана встаёт; закрылось — идёт дальше
    uint64_t m_pauseStart = 0;     ///< когда открылось меню (0 — не на паузе)
    uint64_t m_pausedTotal = 0;    ///< сколько всего простояло на паузе
};

}  // namespace nlb::presentation
