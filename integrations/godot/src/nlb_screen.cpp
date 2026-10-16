#include "nlb_screen.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/font_file.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/theme_db.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/core/class_db.hpp>

#include <cmath>

#include "nlb_player.h"
#include "nlb/presentation/Session.h"
#include "nlb/util/FileUtils.h"

using namespace godot;
using namespace nlb::presentation;

namespace {
String s(const std::string& v) { return String::utf8(v.c_str()); }
godot::Color col(nlb::presentation::Color c, float alpha) {
    return godot::Color(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, (c.a / 255.0f) * alpha);
}
}

NLBScreen::NLBScreen() = default;
NLBScreen::~NLBScreen() = default;

void NLBScreen::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_player", "path"), &NLBScreen::set_player);
    ClassDB::bind_method(D_METHOD("get_player"), &NLBScreen::get_player);
    ClassDB::bind_method(D_METHOD("set_choices_screen", "on"), &NLBScreen::set_choices_screen);
    ClassDB::bind_method(D_METHOD("get_choices_screen"), &NLBScreen::get_choices_screen);
    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "player", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "NLBPlayer"), "set_player", "get_player");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "choices_screen"), "set_choices_screen", "get_choices_screen");
    ClassDB::bind_method(D_METHOD("next_page"), &NLBScreen::next_page);
    ClassDB::bind_method(D_METHOD("is_choices_screen_shown"), &NLBScreen::is_choices_screen_shown);
    ClassDB::bind_method(D_METHOD("get_draw_count"), &NLBScreen::get_draw_count);
    ADD_SIGNAL(MethodInfo("quit_requested"));
}

NLBPlayer* NLBScreen::player() const {
    return m_playerPath.is_empty() ? nullptr : Object::cast_to<NLBPlayer>(get_node_or_null(m_playerPath));
}

void NLBScreen::_ready() {
    set_focus_mode(FOCUS_ALL);
    if (Engine::get_singleton()->is_editor_hint()) return;
    grab_focus();
}

bool NLBScreen::ensureScreen() {
    NLBPlayer* p = player();
    if (!p || !p->session()) return false;
    if (m_screen && m_screenSession == p->session()) return true;
    m_screenSession = p->session();
    GameScreen::Options o;
    o.themeDir = std::string(p->theme_dir().utf8().get_data());
    o.choicesScreen = m_choicesScreen;
    o.soundAvailable = true;
    o.clock = [] { return static_cast<uint64_t>(Time::get_singleton()->get_ticks_msec()); };
    m_screen = std::make_unique<GameScreen>(*p->session(), o);
    // события, которые забирает экран, — в сигналы NLBPlayer (игра слушает их)
    m_screen->setEventListener([p](const std::vector<PlayerEngine::Event>& events) { p->dispatch_events(events); });
    // внешнюю игру ведёт сама игра (сигнал external_game) — сообщение «её нет» не нужно
    m_screen->setExternalGameHandler([](const std::string&) {});
    p->set_screen_driven(true);
    return true;
}

void NLBScreen::_exit_tree() {
    if (NLBPlayer* p = player()) p->set_screen_driven(false);
    m_screen.reset();
}

void NLBScreen::_process(double delta) {
    if (Engine::get_singleton()->is_editor_hint() || !ensureScreen()) return;
    m_screen->update(static_cast<int>(delta * 1000.0));
    if (m_screen->quitRequested()) {
        m_screen->clearQuitRequest();
        emit_signal("quit_requested");
    }
    queue_redraw();
}

void NLBScreen::layout(float& scale, Vector2& offset) const {
    const Vector2 size = get_size();
    scale = std::min(size.x / GameScreen::SCREEN_W, size.y / GameScreen::SCREEN_H);
    offset = (size - Vector2(GameScreen::SCREEN_W, GameScreen::SCREEN_H) * scale) * 0.5f;
}

Vector2 NLBScreen::toScreen(const Vector2& local) const {
    float scale;
    Vector2 offset;
    layout(scale, offset);
    return scale > 0 ? (local - offset) / scale : Vector2();
}

void NLBScreen::_draw() {
    if (!m_screen) return;
    m_drawCount = 0;
    float scale;
    Vector2 offset;
    layout(scale, offset);
    // все команды — в координатах экрана 1920×1080
    draw_set_transform(offset, 0.0, Vector2(scale, scale));
    m_screen->render(*this);
    draw_set_transform(Vector2(), 0.0, Vector2(1, 1));
}

void NLBScreen::_gui_input(const Ref<InputEvent>& event) {
    if (!ensureScreen()) return;
    if (Ref<InputEventMouseButton> mb = event; mb.is_valid() && mb->is_pressed()) {
        const Vector2 p = toScreen(mb->get_position());
        switch (mb->get_button_index()) {
            case MOUSE_BUTTON_LEFT: m_screen->click(p.x, p.y, false); break;
            case MOUSE_BUTTON_RIGHT: m_screen->click(p.x, p.y, true); break;
            case MOUSE_BUTTON_WHEEL_UP: m_screen->wheel(1.0f); break;
            case MOUSE_BUTTON_WHEEL_DOWN: m_screen->wheel(-1.0f); break;
            default: return;
        }
        accept_event();
    } else if (Ref<InputEventMouseMotion> mm = event; mm.is_valid()) {
        const Vector2 p = toScreen(mm->get_position());
        m_screen->mouseMove(p.x, p.y);
    } else if (Ref<InputEventKey> k = event; k.is_valid() && k->is_pressed() && !k->is_echo()) {
        using Key = GameScreen::Key;
        const godot::Key code = k->get_keycode();
        if (code == KEY_ESCAPE) m_screen->key(Key::Escape);
        else if (code == KEY_SPACE) m_screen->key(Key::Space);
        else if (code == KEY_ENTER || code == KEY_KP_ENTER) m_screen->key(Key::Enter);
        else if (code == KEY_PAGEUP) m_screen->key(Key::PageUp);
        else if (code == KEY_PAGEDOWN) m_screen->key(Key::PageDown);
        else if (code == KEY_F5) m_screen->key(Key::Save);
        else if (code == KEY_F9) m_screen->key(Key::Load);
        else if (code >= KEY_1 && code <= KEY_9) m_screen->key(Key::Digit, static_cast<int>(code - KEY_1) + 1);
        else return;
        accept_event();
    }
}

void NLBScreen::next_page() {
    if (ensureScreen()) m_screen->key(GameScreen::Key::Space);
}

bool NLBScreen::is_choices_screen_shown() const {
    return m_screen && m_screen->choicesScreenShown();
}

// ============================================================================ рендерер

Ref<Texture2D> NLBScreen::texture(const std::string& path) {
    auto it = m_textures.find(path);
    if (it != m_textures.end()) return it->second;
    Ref<Texture2D> tex;
    std::string data;
    // полный путь (картинки темы) — с диска или из пакета; иначе — медиа книги через NLBPlayer
    if (path.find('/') != std::string::npos && FileUtils::exists(path) && !FileUtils::isDirectory(path) && FileUtils::readFile(path, data)) {
        PackedByteArray bytes;
        bytes.resize(static_cast<int64_t>(data.size()));
        memcpy(bytes.ptrw(), data.data(), data.size());
        Ref<Image> img;
        img.instantiate();
        const String lower = s(path).to_lower();
        const Error err = lower.ends_with(".png") ? img->load_png_from_buffer(bytes)
                        : lower.ends_with(".webp") ? img->load_webp_from_buffer(bytes)
                        : img->load_jpg_from_buffer(bytes);
        if (err == OK) tex = ImageTexture::create_from_image(img);
    } else if (NLBPlayer* p = player()) {
        tex = p->load_texture(s(path));
    }
    if (m_textures.size() > 600) m_textures.clear();  // анимации из сотен кадров
    m_textures[path] = tex;
    return tex;
}

Ref<Font> NLBScreen::font(const FontRef& ref) {
    auto it = m_fonts.find(ref);
    if (it != m_fonts.end()) return it->second;
    Ref<Font> f;
    std::string data;
    if (!ref.file.empty() && FileUtils::readFile(ref.file, data) && !data.empty()) {
        Ref<FontFile> ff;
        ff.instantiate();
        PackedByteArray bytes;
        bytes.resize(static_cast<int64_t>(data.size()));
        memcpy(bytes.ptrw(), data.data(), data.size());
        ff->set_data(bytes);
        f = ff;
    }
    if (f.is_null()) f = ThemeDB::get_singleton()->get_fallback_font();  // шрифт Godot по умолчанию
    m_fonts[ref] = f;
    return f;
}

bool NLBScreen::imageSize(const std::string& path, float& w, float& h) {
    Ref<Texture2D> t = texture(path);
    if (t.is_null()) return false;
    w = static_cast<float>(t->get_width());
    h = static_cast<float>(t->get_height());
    return true;
}

void NLBScreen::drawImage(const std::string& path, const RectF& dst, float alpha) {
    Ref<Texture2D> t = texture(path);
    if (t.is_null()) return;
    draw_texture_rect(t, Rect2(dst.x, dst.y, dst.w, dst.h), false, godot::Color(1, 1, 1, alpha));
    ++m_drawCount;
}

void NLBScreen::fillRect(const RectF& r, nlb::presentation::Color c) {
    draw_rect(Rect2(r.x, r.y, r.w, r.h), col(c, 1.0f), true);
    ++m_drawCount;
}

float NLBScreen::textWidth(const FontRef& ref, const std::string& text) {
    if (text.empty()) return 0;
    return static_cast<float>(font(ref)->get_string_size(s(text), HORIZONTAL_ALIGNMENT_LEFT, -1, static_cast<int>(ref.size)).x);
}

float NLBScreen::fontHeight(const FontRef& ref) {
    return static_cast<float>(font(ref)->get_height(static_cast<int>(ref.size)));
}

float NLBScreen::lineSkip(const FontRef& ref) {
    return static_cast<float>(font(ref)->get_height(static_cast<int>(ref.size)));
}

void NLBScreen::drawText(const FontRef& ref, const std::string& text, nlb::presentation::Color c, float x, float y, float alpha) {
    if (text.empty()) return;
    Ref<Font> f = font(ref);
    // GameScreen задаёт верх строки, Godot рисует от базовой линии
    const float baseline = y + static_cast<float>(f->get_ascent(static_cast<int>(ref.size)));
    draw_string(f, Vector2(x, baseline), s(text), HORIZONTAL_ALIGNMENT_LEFT, -1, static_cast<int>(ref.size), col(c, alpha));
    ++m_drawCount;
}

void NLBScreen::setClip(const RectF*) {
    // GameScreen сам рисует в окне текста только целые строки, отдельное отсечение не нужно
}
