#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "nlb/domain/PlayerEngine.h"
#include "nlb/presentation/ScreenModel.h"
#include "ScreenTheme.h"

class NonLinearBookImpl;
namespace nlb::presentation { class BookMediaSource; class AudioDirector; }
namespace nlb::audio { class SdlAudioBackend; }

/**
 * @brief Графический проигрыватель книг NLB на SDL3 (nlb_sdl).
 *
 * Рисует снимок экрана движка (PlayerEngine::screen()) в логическом экране 1920×1080, который
 * вписывается в окно с полями. Звук — SdlAudioBackend через AudioDirector. Время идёт по-настоящему
 * (Settings::realTime): таймеры страниц и анимация — через advanceTime().
 *
 * Тема STANDARD: картинка, текст со ссылками на объекты, варианты, инвентарь справа.
 * Тема VN: фон на весь экран, графические объекты на своих местах (по правилам vn.lua),
 * текстовое окно внизу.
 */
class SdlPlayer {
public:
    struct Options {
        std::string bookPath;                    ///< каталог книги или .nlbpack (UTF-8)
        PlayerEngine::ExportMode exportMode = PlayerEngine::ExportMode::VN;
        std::string fontPath;                    ///< шрифт проигрывателя (меню; запасной для темы)
        std::string themeDir;                    ///< каталог темы INSTEAD; пусто — тема по умолчанию
        bool sound = true;
        std::string savesDir;                    ///< пусто — сохранения недоступны
        std::string achievementsPath;
        std::string scriptPath;                  ///< для тестов: команды и снимки экрана
        int windowWidth = 1280, windowHeight = 720;
        /// Варианты отдельным экраном: окно показывает только текст, а дочитав его (пробел или
        /// щелчок), игрок видит список вариантов посередине экрана. Иначе — варианты в окне текста
        bool choicesScreen = false;
    };

    explicit SdlPlayer(Options options);
    ~SdlPlayer();
    /// Код возврата процесса
    int run();

private:
    // ---- клики: область экрана и что она делает
    struct Hit {
        enum class Kind { Choice, Object, Inventory, Menu };
        SDL_FRect rect;
        Kind kind;
        std::string id;   ///< объект/предмет
        int index = -1;   ///< вариант или пункт меню
    };
    enum class Menu { None, Main, Save, Load, Message };

    bool init();
    void shutdown();
    void loop();
    void refresh();                      ///< новый снимок, события -> звук и сообщения
    void processEvents(const std::vector<PlayerEngine::Event>& events);
    void render();
    void renderStandard();
    void renderVisualNovel();
    void renderInventory(const ScreenTheme::Window& window, bool panel);
    void renderMenu();
    void renderOverlayMessages();
    float renderParagraphs(const std::vector<nlb::presentation::Paragraph>& text, float x, float y, float width,
                           TTF_Font* font, const ScreenTheme::Window& colors, bool draw);
    float renderChoices(float x, float y, float width, TTF_Font* font, const ScreenTheme::Window& colors, bool draw);
    /// Текстовое окно темы: заголовок, текст, ответ, варианты; с прокруткой
    void renderTextWindow(const ScreenTheme::Window& window, bool panel);
    /// Шрифт окна темы (файл темы или шрифт проигрывателя нужного размера)
    TTF_Font* fontFor(const ScreenTheme::Window& window);
    void drawText(const std::string& text, TTF_Font* font, SDL_Color color, float x, float y);
    SDL_FPoint textSize(const std::string& text, TTF_Font* font);
    void fillRect(const SDL_FRect& r, SDL_Color c);

    // ---- изображения
    SDL_Texture* texture(const nlb::presentation::ImageRef& image);
    SDL_Texture* textureFor(const std::string& path);
    SDL_Texture* textTexture(const std::string& text, TTF_Font* font, SDL_Color color);
    /// Положение графического объекта (vn.lua: postoxy); размер — по его картинке
    SDL_FRect objectRect(size_t index);

    // ---- действия игрока
    void click(float x, float y, bool rightButton);
    void key(SDL_Keycode keycode);
    void afterCommand();
    void openMenu(Menu menu);
    void menuAction(int index);
    std::string slotPath(int slot) const;

    // ---- сценарий для тестов
    bool runScript();
    bool scriptCommand(const std::string& line);
    bool saveScreenshot(const std::string& path);

    Options m_options;
    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    TTF_Font* m_font = nullptr;
    TTF_Font* m_titleFont = nullptr;
    TTF_Font* m_smallFont = nullptr;
    std::string m_fontPath;
    ScreenTheme m_standardTheme, m_vnTheme;
    std::map<std::string, TTF_Font*> m_fonts;  ///< "файл|размер" -> шрифт
    std::map<std::string, std::shared_ptr<std::string>> m_fontData;  ///< шрифты из пакета: данные живут, пока открыт шрифт
    /// Открыть шрифт с диска или из смонтированного пакета
    TTF_Font* openFont(const std::string& path, float size);

    std::unique_ptr<NonLinearBookImpl> m_book;
    std::unique_ptr<PlayerEngine> m_engine;
    std::unique_ptr<nlb::presentation::BookMediaSource> m_media;
    std::unique_ptr<nlb::audio::SdlAudioBackend> m_audio;
    std::unique_ptr<nlb::presentation::AudioDirector> m_director;

    nlb::presentation::ScreenModel m_screen;
    std::vector<Hit> m_hits;
    std::string m_selectedItem;          ///< предмет, выбранный для применения
    std::string m_hoverObject;           ///< объект под курсором (morphover)
    float m_scroll = 0;                  ///< прокрутка текста (тема STANDARD)
    float m_contentHeight = 0;
    SDL_FRect m_textWindow{};            ///< окно текста на последнем кадре
    float m_pageHeight = 0;              ///< высота «страницы» текста — целое число строк
    bool m_moreText = false;             ///< текст не уместился: щелчок по окну — дальше
    bool m_choicesScreen = false;        ///< режим «варианты отдельным экраном» (переключается в меню)
    bool m_showChoices = false;          ///< сейчас показан экран вариантов
    std::string m_choicesForPage;        ///< страница, для которой экран вариантов уже открывали
    void renderChoicesScreen();
    /// Дочитан текст: в режиме экрана вариантов показать варианты; true — показали
    bool advanceToChoices();
    float m_drawTop = -1e9f, m_drawBottom = 1e9f;  ///< рисуются только строки целиком внутри
    float m_firstHiddenY = -1;           ///< верх первой не уместившейся строки (начало следующей страницы)
    Menu m_menu = Menu::None;
    std::string m_message;               ///< сообщение поверх (конец игры, ошибка сохранения)
    std::vector<std::string> m_toasts;   ///< достижения
    Uint64 m_toastUntil = 0;
    bool m_quit = false;
    bool m_soundOn = true;

    std::map<std::string, SDL_Texture*> m_images;
    std::map<std::string, SDL_Texture*> m_texts;
    std::map<std::string, SDL_FRect> m_objectRects;  ///< вычисленные места объектов (для детей)

    // ---- эффекты появления спрайтов (vn.lua: шаг 20 мс, шагов maxStep или 500 мс / 20 мс)
    struct Effect {
        Uint64 start = 0;
        int steps = 25;
        bool hiding = false;        ///< обратный ход: свиток сворачивается
        nlb::presentation::SceneObject object;  ///< последний вид объекта (для обратного хода)
    };
    std::map<std::string, Effect> m_effects;
    std::map<std::string, bool> m_shown;      ///< объект был виден на прошлом снимке
    void updateEffects(const nlb::presentation::ScreenModel& before);
    /// Прозрачность, сдвиг и масштаб объекта на текущем кадре
    struct EffectState { float alpha = 1, dx = 0, dy = 0, scale = 1; bool done = true; };
    EffectState effectState(const std::string& id, const SDL_FRect& rect);
    void drawObject(size_t index, const nlb::presentation::SceneObject& o, const SDL_FRect& r, TTF_Font* labelFont,
                    SDL_Color labelColor, bool spriteLabel);
};
