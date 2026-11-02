#pragma once

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/node_path.hpp>

#include <godot_cpp/variant/rid.hpp>

#include <map>
#include <memory>
#include <string>
#include <vector>

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
    /// Экран вариантов для книги в режиме Standard; в режиме VN он включён всегда
    void set_choices_screen(bool on) { m_choicesScreen = on; }
    bool get_choices_screen() const { return m_choicesScreen; }
    /// Переходы между сценами (dissolve, как в INSTEAD): прежний кадр проступает в новый за ~0,4 с
    void set_transitions(bool on) { m_transitions = on; }
    bool get_transitions() const { return m_transitions; }

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
    bool hasSnapshot() const override { return m_snapshotFresh && m_snapshot.is_valid(); }
    void drawSnapshot(float alpha) override;

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
    /// Прежний кадр для перехода: область экрана книги из текстуры окна (там ещё последний
    /// показанный кадр). Без изображения (--headless, рендерер-заглушка) — без перехода
    void captureSnapshot();
    /// Отсечение (setClip): команды рисования идут не в сам узел, а в цепочку дочерних элементов
    /// канвы RenderingServer по порядку. Каждый setClip начинает следующий элемент: внешний — со
    /// сдвигом и масштабом экрана книги и (если задано) отсечением по прямоугольнику, внутренний —
    /// с командами (отсечение потомков в Godot работает всегда). Порядок наложения — draw_index
    struct Segment { godot::RID outer, inner; };
    std::vector<Segment> m_segments;
    size_t m_segmentCount = 0;
    godot::RID m_canvas;  ///< внутренний элемент текущего сегмента — сюда рисуем
    float m_layoutScale = 1;
    godot::Vector2 m_layoutOffset;
    void beginSegment(const nlb::presentation::RectF* clip);
    void freeSegments();

    godot::NodePath m_playerPath;
    bool m_choicesScreen = false;
    bool m_transitions = true;
    godot::Ref<godot::ImageTexture> m_snapshot;
    bool m_snapshotFresh = false;  ///< снят перед этим кадром (иначе — от прошлого перехода)
    std::unique_ptr<nlb::presentation::GameScreen> m_screen;
    const nlb::presentation::Session* m_screenSession = nullptr;
    std::map<std::string, godot::Ref<godot::Texture2D>> m_textures;
    std::map<nlb::presentation::FontRef, godot::Ref<godot::Font>> m_fonts;
    int64_t m_drawCount = 0;
};
