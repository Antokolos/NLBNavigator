#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "nlb/presentation/GameScreen.h"

namespace nlb::presentation { class Session; }
namespace nlb::audio { class SdlAudioBackend; }

/**
 * @brief Графический проигрыватель книг NLB на SDL3 (nlb_sdl).
 *
 * Экран игры — общий nlb::presentation::GameScreen (тот же, что в Unreal и Godot); здесь только
 * окно, рисование его командами средствами SDL (SdlScreenRenderer), ввод и звук SDL3_mixer.
 * Логический экран 1920×1080 вписывается в окно с полями.
 */
class SdlPlayer : public nlb::presentation::IScreenRenderer {
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
    ~SdlPlayer() override;
    /// Код возврата процесса
    int run();

    // ---- IScreenRenderer: рисование команд GameScreen средствами SDL
    bool imageSize(const std::string& path, float& w, float& h) override;
    void drawImage(const std::string& path, const nlb::presentation::RectF& dst, float alpha) override;
    void fillRect(const nlb::presentation::RectF& r, nlb::presentation::Color c) override;
    float textWidth(const nlb::presentation::FontRef& font, const std::string& text) override;
    float fontHeight(const nlb::presentation::FontRef& font) override;
    float lineSkip(const nlb::presentation::FontRef& font) override;
    void drawText(const nlb::presentation::FontRef& font, const std::string& text, nlb::presentation::Color c,
                  float x, float y, float alpha) override;
    void setClip(const nlb::presentation::RectF* clip) override;

private:
    bool init();
    void shutdown();
    void loop();
    void handleEvent(SDL_Event& e);
    void renderFrame();

    TTF_Font* font(const nlb::presentation::FontRef& ref);
    TTF_Font* openFont(const std::string& path, float size);
    SDL_Texture* textureFor(const std::string& path);
    SDL_Texture* textTexture(const std::string& text, TTF_Font* font, SDL_Color color);

    bool runScript();
    bool scriptCommand(const std::string& line);
    bool saveScreenshot(const std::string& path);

    Options m_options;
    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    std::string m_fontPath;
    bool m_quit = false;
    std::unique_ptr<nlb::presentation::IMediaSource> m_media;
    std::unique_ptr<nlb::audio::SdlAudioBackend> m_audio;
    std::unique_ptr<nlb::presentation::Session> m_session;
    std::unique_ptr<nlb::presentation::GameScreen> m_screen;

    std::map<std::string, SDL_Texture*> m_images;
    std::map<std::string, SDL_Texture*> m_texts;
    std::map<std::string, TTF_Font*> m_fonts;                           ///< "файл|размер"
    std::map<std::string, std::shared_ptr<std::string>> m_fontData;     ///< шрифты из пакета
};
