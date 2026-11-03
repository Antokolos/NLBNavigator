#include "nlb_screen.h"

#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/font_file.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/theme_db.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/viewport_texture.hpp>
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
NLBScreen::~NLBScreen() { freeSegments(); }

void NLBScreen::freeSegments() {
    RenderingServer* rs = RenderingServer::get_singleton();
    if (rs) {
        for (const Segment& seg : m_segments) {
            rs->free_rid(seg.inner);
            rs->free_rid(seg.outer);
        }
    }
    m_segments.clear();
    m_segmentCount = 0;
    m_canvas = RID();
}

void NLBScreen::beginSegment(const RectF* clip) {
    RenderingServer* rs = RenderingServer::get_singleton();
    if (m_segmentCount == m_segments.size()) {
        Segment seg{rs->canvas_item_create(), rs->canvas_item_create()};
        rs->canvas_item_set_parent(seg.outer, get_canvas_item());
        rs->canvas_item_set_parent(seg.inner, seg.outer);
        m_segments.push_back(seg);
    }
    const Segment& seg = m_segments[m_segmentCount];
    rs->canvas_item_clear(seg.inner);
    rs->canvas_item_set_visible(seg.outer, true);
    rs->canvas_item_set_draw_index(seg.outer, static_cast<int64_t>(m_segmentCount));
    // координаты экрана 1920×1080 -> узел: масштаб и поля (layout)
    rs->canvas_item_set_transform(seg.outer, Transform2D(Vector2(m_layoutScale, 0), Vector2(0, m_layoutScale), m_layoutOffset));
    // всегда — по экрану темы (за ним — поля узла: рамка 1920×1080 туда не вылезает), с clip — и по нему
    const RectF a = area();
    Rect2 r(a.x, a.y, a.w, a.h);
    if (clip) r = r.intersection(Rect2(clip->x, clip->y, clip->w, clip->h));
    rs->canvas_item_set_custom_rect(seg.outer, true, r);
    rs->canvas_item_set_clip(seg.outer, true);
    m_canvas = seg.inner;
    ++m_segmentCount;
}

void NLBScreen::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_player", "path"), &NLBScreen::set_player);
    ClassDB::bind_method(D_METHOD("get_player"), &NLBScreen::get_player);
    ClassDB::bind_method(D_METHOD("set_choices_screen", "on"), &NLBScreen::set_choices_screen);
    ClassDB::bind_method(D_METHOD("get_choices_screen"), &NLBScreen::get_choices_screen);
    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "player", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "NLBPlayer"), "set_player", "get_player");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "choices_screen"), "set_choices_screen", "get_choices_screen");
    ClassDB::bind_method(D_METHOD("set_transitions", "on"), &NLBScreen::set_transitions);
    ClassDB::bind_method(D_METHOD("get_transitions"), &NLBScreen::get_transitions);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "transitions"), "set_transitions", "get_transitions");
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
    o.baseThemeDir = std::string(p->base_theme_dir().utf8().get_data());
    // VN — всегда экран вариантов (как paginator в VN-INSTEAD), Standard — по свойству choices_screen
    o.choicesScreen = p->session()->exportMode() == PlayerEngine::ExportMode::VN || m_choicesScreen;
    o.soundAvailable = true;
    o.transitions = m_transitions;
    o.clock = [] { return static_cast<uint64_t>(Time::get_singleton()->get_ticks_msec()); };
    m_screen = std::make_unique<GameScreen>(*p->session(), o);
    // события, которые забирает экран, — в сигналы NLBPlayer (игра слушает их)
    m_screen->setEventListener([p](const std::vector<PlayerEngine::Event>& events) { p->dispatch_events(events); });
    // внешнюю игру ведёт сама игра (сигнал external_game) — сообщение «её нет» не нужно
    m_screen->setExternalGameHandler([](const std::string&) {});
    // Полный экран — пункт меню книги (окно Godot)
    m_screen->setFullscreenHandler(
        [](bool on) {
            DisplayServer::get_singleton()->window_set_mode(on ? DisplayServer::WINDOW_MODE_FULLSCREEN : DisplayServer::WINDOW_MODE_WINDOWED);
        },
        [] {
            const auto mode = DisplayServer::get_singleton()->window_get_mode();
            return mode == DisplayServer::WINDOW_MODE_FULLSCREEN || mode == DisplayServer::WINDOW_MODE_EXCLUSIVE_FULLSCREEN;
        });
    p->set_screen_driven(true);
    return true;
}

void NLBScreen::_exit_tree() {
    if (NLBPlayer* p = player()) p->set_screen_driven(false);
    m_screen.reset();
    freeSegments();
}

void NLBScreen::_process(double delta) {
    if (Engine::get_singleton()->is_editor_hint() || !ensureScreen()) return;
    m_screen->update(static_cast<int>(delta * 1000.0));
    // переход начинается на следующем кадре: текстура окна ещё хранит прежний кадр (смена страницы —
    // по вводу в _gui_input или по времени в update выше)
    if (m_screen->needsSnapshot()) captureSnapshot();
    if (m_screen->quitRequested()) {
        m_screen->clearQuitRequest();
        emit_signal("quit_requested");
    }
    queue_redraw();
}

RectF NLBScreen::area() const {
    return m_screen ? m_screen->screenArea() : RectF{0, 0, GameScreen::SCREEN_W, GameScreen::SCREEN_H};
}

void NLBScreen::layout(float& scale, Vector2& offset) const {
    const Vector2 size = get_size();
    const RectF a = area();
    scale = std::min(size.x / a.w, size.y / a.h);
    offset = (size - Vector2(a.w, a.h) * scale) * 0.5f - Vector2(a.x, a.y) * scale;
}

Vector2 NLBScreen::toScreen(const Vector2& local) const {
    float scale;
    Vector2 offset;
    layout(scale, offset);
    return scale > 0 ? (local - offset) / scale : Vector2();
}

void NLBScreen::captureSnapshot() {
    m_snapshot.unref();
    // без окна (--headless) кадров нет — переход не нужен
    if (DisplayServer::get_singleton()->get_name() == "headless") return;
    Viewport* viewport = get_viewport();
    if (!viewport) return;
    Ref<ViewportTexture> vt = viewport->get_texture();
    if (vt.is_null()) return;
    Ref<Image> image = vt->get_image();
    if (image.is_null() || image->is_empty()) return;
    // область экрана темы (вписанная в узел с полями): из координат узла — в координаты
    // окна (с канвой и её слоями), из них — в пиксели текстуры (при растяжении окна они другого
    // размера, чем видимая область)
    float scale;
    Vector2 offset;
    layout(scale, offset);
    const Rect2 visible = viewport->get_visible_rect();
    if (scale <= 0 || visible.size.x <= 0 || visible.size.y <= 0) return;
    const Vector2 ratio(image->get_width() / visible.size.x, image->get_height() / visible.size.y);
    const Transform2D xf = get_global_transform_with_canvas();
    const RectF ar = area();
    const Vector2 a = (xf.xform(offset + Vector2(ar.x, ar.y) * scale) - visible.position) * ratio;
    const Vector2 b = (xf.xform(offset + Vector2(ar.x + ar.w, ar.y + ar.h) * scale) - visible.position) * ratio;
    const int x0 = std::max(0, static_cast<int>(std::lround(std::min(a.x, b.x))));
    const int y0 = std::max(0, static_cast<int>(std::lround(std::min(a.y, b.y))));
    const int x1 = std::min(image->get_width(), static_cast<int>(std::lround(std::max(a.x, b.x))));
    const int y1 = std::min(image->get_height(), static_cast<int>(std::lround(std::max(a.y, b.y))));
    if (x1 - x0 < 2 || y1 - y0 < 2) return;
    m_snapshot = ImageTexture::create_from_image(image->get_region(Rect2i(x0, y0, x1 - x0, y1 - y0)));
    m_snapshotFresh = m_snapshot.is_valid();
}

void NLBScreen::drawSnapshot(float alpha) {
    if (m_snapshot.is_null()) return;
    const RectF ar = area();
    RenderingServer::get_singleton()->canvas_item_add_texture_rect(m_canvas, Rect2(ar.x, ar.y, ar.w, ar.h),
                                                                   m_snapshot->get_rid(), false, godot::Color(1, 1, 1, alpha));
    ++m_drawCount;
}

void NLBScreen::_draw() {
    if (!m_screen) return;
    m_drawCount = 0;
    // все команды — в координатах экрана 1920×1080, в элементы канвы сегментов (setClip)
    layout(m_layoutScale, m_layoutOffset);
    m_segmentCount = 0;
    beginSegment(nullptr);
    m_screen->render(*this);
    m_snapshotFresh = false;  // переход начат (или нет) — дальше снимок только рисуется
    // сегменты, не понадобившиеся в этом кадре, — пустые и скрытые
    RenderingServer* rs = RenderingServer::get_singleton();
    for (size_t i = m_segmentCount; i < m_segments.size(); ++i) {
        rs->canvas_item_clear(m_segments[i].inner);
        rs->canvas_item_set_visible(m_segments[i].outer, false);
    }
}

void NLBScreen::_gui_input(const Ref<InputEvent>& event) {
    if (!ensureScreen()) return;
    if (Ref<InputEventMouseButton> mb = event; mb.is_valid() && !mb->is_pressed()
        && mb->get_button_index() == MOUSE_BUTTON_LEFT) {
        // отпускание левой кнопки: щелчок по тексту или конец прокрутки перетаскиванием
        const Vector2 p = toScreen(mb->get_position());
        m_screen->release(p.x, p.y);
        accept_event();
    } else if (Ref<InputEventMouseButton> mb = event; mb.is_valid() && mb->is_pressed()) {
        const Vector2 p = toScreen(mb->get_position());
        switch (mb->get_button_index()) {
            case MOUSE_BUTTON_LEFT: m_screen->press(p.x, p.y); break;
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
    if (path.find_first_of("/\\") != std::string::npos && FileUtils::exists(path) && !FileUtils::isDirectory(path) && FileUtils::readFile(path, data)) {
        PackedByteArray bytes;
        bytes.resize(static_cast<int64_t>(data.size()));
        memcpy(bytes.ptrw(), data.data(), data.size());
        Ref<Image> img;
        img.instantiate();
        const Error err = nlbLoadImage(img, bytes, s(path));
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
    RenderingServer::get_singleton()->canvas_item_add_texture_rect(m_canvas, Rect2(dst.x, dst.y, dst.w, dst.h), t->get_rid(), false,
                                                                   godot::Color(1, 1, 1, alpha));
    ++m_drawCount;
}

void NLBScreen::fillRect(const RectF& r, nlb::presentation::Color c) {
    RenderingServer::get_singleton()->canvas_item_add_rect(m_canvas, Rect2(r.x, r.y, r.w, r.h), col(c, 1.0f));
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
    f->draw_string(m_canvas, Vector2(x, baseline), s(text), HORIZONTAL_ALIGNMENT_LEFT, -1, static_cast<int>(ref.size), col(c, alpha));
    ++m_drawCount;
}

void NLBScreen::setClip(const RectF* clip) {
    // следующие команды — в новый сегмент: с отсечением по clip или без него
    beginSegment(clip);
}
