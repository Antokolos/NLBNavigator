#pragma once

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/node_path.hpp>

#include <map>
#include <memory>
#include <string>

#include "nlb/presentation/GameScreen.h"

class NLBPlayer;

/**
 * @brief Экран книги NLB в Godot — тот же, что в nlb_sdl и Unreal: общий
 * nlb::presentation::GameScreen (тема INSTEAD, полоса текста VN, выравнивание по ширине, листание
 * порциями, экран вариантов, спрайты по правилам vn.lua с эффектами, меню). Узел рисует его команды
 * средствами CanvasItem (_draw) и передаёт ему мышь и клавиатуру. Книгу открывает узел NLBPlayer
 * (свойство player); пока экран работает, время книги ведёт он.
 */
class NLBScreen : public godot::Control, public nlb::presentation::IScreenRenderer {
    GDCLASS(NLBScreen, godot::Control)

public:
    NLBScreen();
    ~NLBScreen() override;

    void _ready() override;
    void _process(double delta) override;
    void _draw() override;
    void _gui_input(const godot::Ref<godot::InputEvent>& event) override;
    void _exit_tree() override;

    void set_player(const godot::NodePath& path) { m_playerPath = path; }
    godot::NodePath get_player() const { return m_playerPath; }
    void set_choices_screen(bool on) { m_choicesScreen = on; }
    bool get_choices_screen() const { return m_choicesScreen; }

    /// «Дальше» (как пробел): следующая часть текста, затем экран вариантов
    void next_page();
    /// Сейчас показан экран вариантов
    bool is_choices_screen_shown() const;
    /// Сколько команд рисования было на последнем кадре (для проверок без окна)
    int64_t get_draw_count() const { return m_drawCount; }

    // ---- IScreenRenderer
    bool imageSize(const std::string& path, float& w, float& h) override;
    void drawImage(const std::string& path, const nlb::presentation::RectF& dst, float alpha) override;
    void fillRect(const nlb::presentation::RectF& r, nlb::presentation::Color c) override;
    float textWidth(const nlb::presentation::FontRef& font, const std::string& text) override;
    float fontHeight(const nlb::presentation::FontRef& font) override;
    float lineSkip(const nlb::presentation::FontRef& font) override;
    void drawText(const nlb::presentation::FontRef& font, const std::string& text, nlb::presentation::Color c,
                  float x, float y, float alpha) override;
    void setClip(const nlb::presentation::RectF* clip) override;

protected:
    static void _bind_methods();

private:
    NLBPlayer* player() const;
    bool ensureScreen();
    godot::Ref<godot::Texture2D> texture(const std::string& path);
    godot::Ref<godot::Font> font(const nlb::presentation::FontRef& ref);
    /// Масштаб и сдвиг: экран книги 1920×1080 вписывается в узел с полями
    void layout(float& scale, godot::Vector2& offset) const;
    godot::Vector2 toScreen(const godot::Vector2& local) const;

    godot::NodePath m_playerPath;
    bool m_choicesScreen = false;
    std::unique_ptr<nlb::presentation::GameScreen> m_screen;
    const nlb::presentation::Session* m_screenSession = nullptr;
    std::map<std::string, godot::Ref<godot::Texture2D>> m_textures;
    std::map<nlb::presentation::FontRef, godot::Ref<godot::Font>> m_fonts;
    int64_t m_drawCount = 0;
};
