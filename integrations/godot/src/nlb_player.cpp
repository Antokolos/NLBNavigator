#include "nlb_player.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "godot_audio_backend.h"
#include "nlb/presentation/MediaSource.h"
#include "nlb/util/BookPack.h"
#include "nlb/util/FileUtils.h"

using namespace godot;
using namespace nlb::presentation;

namespace {
String s(const std::string& v) { return String::utf8(v.c_str()); }
std::string u(const String& v) { return std::string(v.utf8().get_data()); }

const char* anchorName(Anchor a) {
    static const char* N[] = {"left-top", "middle-top", "right-top", "left-middle", "middle-middle",
                              "right-middle", "left-bottom", "middle-bottom", "right-bottom"};
    return N[static_cast<int>(a)];
}
const char* effectName(AppearEffect e) {
    static const char* N[] = {"none", "movein", "moveout", "fadein", "fadeout", "zoomin", "zoomout", "overlap"};
    return N[static_cast<int>(e)];
}
const char* directionName(MoveDirection d) {
    static const char* N[] = {"none", "left", "right", "top", "bottom"};
    return N[static_cast<int>(d)];
}

Dictionary image(const ImageRef& r) {
    Dictionary d;
    d["path"] = s(r.file.path);
    d["frame_pattern"] = s(r.framePattern);
    d["frame_count"] = r.frameCount;
    d["current_frame"] = r.currentFrame;
    d["looped"] = r.looped;
    // путь к текущему кадру — то, что нужно передать в load_texture
    if (!r.framePattern.empty()) {
        char buf[1024];
        std::snprintf(buf, sizeof(buf), r.framePattern.c_str(), r.currentFrame > 0 ? r.currentFrame : 1);
        d["frame_path"] = s(buf);
    } else {
        d["frame_path"] = s(r.file.path);
    }
    return d;
}

Array paragraphs(const std::vector<Paragraph>& ps) {
    Array out;
    for (const auto& p : ps) {
        Array runs;
        for (const auto& r : p.runs) {
            Dictionary run;
            run["kind"] = r.kind == TextRun::Kind::ObjectLink ? "object" : r.kind == TextRun::Kind::ChoiceLink ? "choice" : "text";
            run["text"] = s(r.text);
            run["target"] = s(r.target);
            runs.push_back(run);
        }
        out.push_back(runs);
    }
    return out;
}

Variant toVariant(const cparse::packToken& v) {
    switch (v->type) {
        case cparse::BOOL: return v.asBool();
        case cparse::INT: return static_cast<int64_t>(v.asInt());
        case cparse::REAL: return v.asDouble();
        case cparse::STR: return s(v.asString());
        default: return Variant();
    }
}

cparse::packToken fromVariant(const Variant& v) {
    switch (v.get_type()) {
        case Variant::BOOL: return cparse::packToken(static_cast<bool>(v));
        case Variant::INT: return cparse::packToken(static_cast<int64_t>(v));
        case Variant::FLOAT: return cparse::packToken(static_cast<double>(v));
        default: return cparse::packToken(u(v.stringify()));
    }
}
}

NLBPlayer::NLBPlayer() = default;
NLBPlayer::~NLBPlayer() = default;

void NLBPlayer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_book_path", "path"), &NLBPlayer::set_book_path);
    ClassDB::bind_method(D_METHOD("get_book_path"), &NLBPlayer::get_book_path);
    ClassDB::bind_method(D_METHOD("set_visual_novel", "vn"), &NLBPlayer::set_visual_novel);
    ClassDB::bind_method(D_METHOD("get_visual_novel"), &NLBPlayer::get_visual_novel);
    ClassDB::bind_method(D_METHOD("set_external_games", "games"), &NLBPlayer::set_external_games);
    ClassDB::bind_method(D_METHOD("get_external_games"), &NLBPlayer::get_external_games);
    ClassDB::bind_method(D_METHOD("set_autostart", "on"), &NLBPlayer::set_autostart);
    ClassDB::bind_method(D_METHOD("get_autostart"), &NLBPlayer::get_autostart);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "book_path", PROPERTY_HINT_FILE, "*.nlbpack"), "set_book_path", "get_book_path");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "visual_novel"), "set_visual_novel", "get_visual_novel");
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "external_games"), "set_external_games", "get_external_games");
    ClassDB::bind_method(D_METHOD("set_log_path", "path"), &NLBPlayer::set_log_path);
    ClassDB::bind_method(D_METHOD("get_log_path"), &NLBPlayer::get_log_path);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "log_path", PROPERTY_HINT_GLOBAL_SAVE_FILE, "*.log,*.txt"), "set_log_path", "get_log_path");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "autostart"), "set_autostart", "get_autostart");

    ClassDB::bind_method(D_METHOD("open"), &NLBPlayer::open);
    ClassDB::bind_method(D_METHOD("get_screen"), &NLBPlayer::get_screen);
    ClassDB::bind_method(D_METHOD("get_revision"), &NLBPlayer::get_revision);
    ClassDB::bind_method(D_METHOD("choose", "index"), &NLBPlayer::choose);
    ClassDB::bind_method(D_METHOD("click_object", "id"), &NLBPlayer::click_object);
    ClassDB::bind_method(D_METHOD("click_item", "id"), &NLBPlayer::click_item);
    ClassDB::bind_method(D_METHOD("clear_selection"), &NLBPlayer::clear_selection);
    ClassDB::bind_method(D_METHOD("get_selected_item"), &NLBPlayer::get_selected_item);
    ClassDB::bind_method(D_METHOD("finish_external_game"), &NLBPlayer::finish_external_game);
    ClassDB::bind_method(D_METHOD("restart"), &NLBPlayer::restart);
    ClassDB::bind_method(D_METHOD("save_game", "slot"), &NLBPlayer::save_game);
    ClassDB::bind_method(D_METHOD("load_game", "slot"), &NLBPlayer::load_game);
    ClassDB::bind_method(D_METHOD("set_muted", "muted"), &NLBPlayer::set_muted);
    ClassDB::bind_method(D_METHOD("get_variable", "name"), &NLBPlayer::get_variable);
    ClassDB::bind_method(D_METHOD("set_variable", "name", "value"), &NLBPlayer::set_variable);
    ClassDB::bind_method(D_METHOD("load_texture", "path"), &NLBPlayer::load_texture);

    ADD_SIGNAL(MethodInfo("screen_changed"));
    ADD_SIGNAL(MethodInfo("achievement", PropertyInfo(Variant::STRING, "name")));
    ADD_SIGNAL(MethodInfo("external_game", PropertyInfo(Variant::STRING, "name")));
    ADD_SIGNAL(MethodInfo("game_finished"));
}

void NLBPlayer::_ready() {
    if (Engine::get_singleton()->is_editor_hint()) return;
    if (m_autostart && !m_bookPath.is_empty()) open();
}

String NLBPlayer::resolveBookPath() const {
    // Книга внутри проекта (res://…): в редакторе — путь на диске; в экспортированной игре файлы
    // лежат в .pck, поэтому пакет один раз копируется в user:// и читается оттуда
    if (!m_bookPath.begins_with("res://") && !m_bookPath.begins_with("user://")) return m_bookPath;
    const String global = ProjectSettings::get_singleton()->globalize_path(m_bookPath);
    if (FileAccess::file_exists(m_bookPath) && FileAccess::open(global, FileAccess::READ).is_valid()) return global;
    const String cached = String("user://nlbnavigator/") + m_bookPath.get_file();
    if (!FileAccess::file_exists(cached)) {
        DirAccess::make_dir_recursive_absolute("user://nlbnavigator");
        const PackedByteArray data = FileAccess::get_file_as_bytes(m_bookPath);
        Ref<FileAccess> out = FileAccess::open(cached, FileAccess::WRITE);
        if (out.is_valid()) out->store_buffer(data);
    }
    return ProjectSettings::get_singleton()->globalize_path(cached);
}

void NLBPlayer::set_log_path(const String& path) {
    m_logPath = path;
    if (!m_session) return;
    std::string error;
    // res:// и user:// — в путь на диске
    const String disk = path.begins_with("res://") || path.begins_with("user://") ? ProjectSettings::get_singleton()->globalize_path(path) : path;
    if (!m_session->setLogFile(std::string(disk.utf8().get_data()), error)) UtilityFunctions::push_warning("NLBPlayer: ", s(error));
}

bool NLBPlayer::open() {
    if (!m_audio) m_audio = std::make_unique<GodotAudioBackend>(this, nullptr);
    m_session = std::make_unique<Session>(m_audio.get());
    Session::Options options;
    options.bookPath = u(resolveBookPath());
    m_openedPath = options.bookPath;
    options.exportMode = m_visualNovel ? PlayerEngine::ExportMode::VN : PlayerEngine::ExportMode::Standard;
    const String data = ProjectSettings::get_singleton()->globalize_path("user://nlbnavigator");
    DirAccess::make_dir_recursive_absolute("user://nlbnavigator/saves");
    options.savesDir = u(data + String("/saves"));
    options.achievementsPath = u(data + String("/achievements.tsv"));
    for (int64_t i = 0; i < m_externalGames.size(); ++i) options.externalGames.push_back(u(m_externalGames[i]));
    std::string error;
    if (!m_session->open(options, error)) {
        UtilityFunctions::push_error("NLBPlayer: ", s(error));
        m_session.reset();
        return false;
    }
    m_audio->setMedia(&m_session->media());
    if (!m_logPath.is_empty()) set_log_path(m_logPath);  // журнал прохождения — вместе с книгой
    m_lastRevision = m_session->revision();
    emitEvents();
    emit_signal("screen_changed");
    return true;
}

void NLBPlayer::_exit_tree() {
    // Звук останавливается вместе с узлом (иначе потоки Godot переживают сцену)
    if (m_audio) {
        m_audio->stopMusic(0);
        m_audio->stopEffects();
    }
}

void NLBPlayer::_process(double delta) {
    if (!m_session || m_screenDriven) return;  // экран NLBScreen сам ведёт время и события
    m_session->advance(static_cast<int>(delta * 1000.0));
    emitEvents();
}

void NLBPlayer::dispatch_events(const std::vector<PlayerEngine::Event>& events) {
    using Kind = PlayerEngine::Event::Kind;
    for (const auto& e : events) {
        if (e.kind == Kind::Achievement) emit_signal("achievement", s(e.text));
        else if (e.kind == Kind::ExternalGame) emit_signal("external_game", s(e.text));
        else if (e.kind == Kind::Finish) emit_signal("game_finished");
    }
}

String NLBPlayer::theme_dir() const {
    if (!m_session) return String();
    const std::string dir = FileUtils::combinePath(m_openedPath, BookPack::THEME_DIR);
    return FileUtils::isDirectory(dir) ? s(dir) : String();
}

String NLBPlayer::base_theme_dir() const {
    if (!m_session) return String();
    const std::string dir = FileUtils::combinePath(m_openedPath, BookPack::BASE_THEME_DIR);
    return FileUtils::isDirectory(dir) ? s(dir) : String();
}

void NLBPlayer::emitEvents() {
    // пока книгу показывает NLBScreen, события забирает он и сам вызывает dispatch_events
    if (!m_screenDriven) dispatch_events(m_session->takeEvents());
    if (m_session->revision() != m_lastRevision) {
        m_lastRevision = m_session->revision();
        emit_signal("screen_changed");
    }
}

Dictionary NLBPlayer::get_screen() const {
    Dictionary d;
    if (!m_session) return d;
    const ScreenModel& m = m_session->screen();
    d["revision"] = static_cast<int64_t>(m.revision);
    d["layout"] = m.layout == Layout::VisualNovel ? "vn" : "standard";
    d["page_id"] = s(m.pageId);
    d["caption"] = s(m.caption);
    d["page_image"] = image(m.pageImage);
    d["image_is_background"] = m.imageIsBackground;
    d["page_text"] = paragraphs(m.pageText);
    d["reaction_text"] = paragraphs(m.reactionText);
    Array choices;
    for (const auto& c : m.choices) {
        Dictionary cd;
        cd["index"] = c.index;
        cd["text"] = s(c.text);
        cd["enabled"] = c.enabled;
        cd["alt_text"] = s(c.altText);
        cd["trivial"] = c.trivial;
        choices.push_back(cd);
    }
    d["choices"] = choices;
    Array objects;
    for (const auto& o : m.sceneObjects) {
        Dictionary od;
        od["id"] = s(o.id);
        od["parent_id"] = s(o.parentId);
        od["label"] = s(o.label);
        od["graphical"] = o.graphical;
        od["image"] = image(o.image);
        od["hover_image"] = image(o.hoverImage);
        od["anchor"] = anchorName(o.placement.anchor);
        od["has_offset"] = o.placement.hasOffset;
        od["offset"] = Vector2(o.placement.offsetX, o.placement.offsetY);
        od["arm"] = Vector2(o.placement.armX, o.placement.armY);
        od["has_inner_arm"] = o.placement.hasInnerArm;
        od["inner_arm"] = Vector2(o.placement.innerArmX, o.placement.innerArmY);
        od["effect"] = effectName(o.appearance.effect);
        od["direction"] = directionName(o.appearance.direction);
        od["max_step"] = o.appearance.maxStep;
        od["cur_step"] = o.appearance.curStep;
        od["z"] = o.zOrder;
        od["visible"] = o.visible;
        od["collapsed"] = o.collapsed;
        od["active"] = o.active;
        od["takable"] = o.takable;
        od["preserved"] = o.preserved;
        objects.push_back(od);
    }
    d["objects"] = objects;
    Array inventory;
    for (const auto& i : m.inventory) {
        Dictionary id;
        id["id"] = s(i.id);
        id["parent_id"] = s(i.parentId);
        id["label"] = s(i.label);
        id["icon"] = image(i.icon);
        id["active"] = i.active;
        inventory.push_back(id);
    }
    d["inventory"] = inventory;
    d["selected_item"] = s(m_session->selectedItem());
    d["timer_active"] = m.timerActive;
    d["finished"] = m.finished;
    d["can_save"] = m.canSave;
    d["external_game"] = m.externalGame ? s(*m.externalGame) : String();
    return d;
}

int64_t NLBPlayer::get_revision() const { return m_session ? static_cast<int64_t>(m_session->revision()) : 0; }

#define NLB_COMMAND(call) \
    if (!m_session) return false; \
    const bool ok = (call); \
    emitEvents(); \
    return ok;

bool NLBPlayer::choose(int64_t index) { NLB_COMMAND(m_session->choose(static_cast<int>(index))) }
bool NLBPlayer::click_object(const String& id) { NLB_COMMAND(m_session->clickObject(u(id))) }
bool NLBPlayer::click_item(const String& id) { NLB_COMMAND(m_session->clickItem(u(id))) }
bool NLBPlayer::finish_external_game() { NLB_COMMAND(m_session->finishExternalGame()) }

void NLBPlayer::clear_selection() {
    if (!m_session) return;
    m_session->clearSelection();
    emitEvents();
}

String NLBPlayer::get_selected_item() const { return m_session ? s(m_session->selectedItem()) : String(); }

void NLBPlayer::restart() {
    if (!m_session) return;
    m_session->restart();
    emitEvents();
}

bool NLBPlayer::save_game(int64_t slot) {
    std::string error;
    if (!m_session || !m_session->save(static_cast<int>(slot), error)) {
        UtilityFunctions::push_warning("NLBPlayer: ", s(error));
        return false;
    }
    return true;
}

bool NLBPlayer::load_game(int64_t slot) {
    std::string error;
    if (!m_session) return false;
    const bool ok = m_session->load(static_cast<int>(slot), error);
    if (!ok) UtilityFunctions::push_warning("NLBPlayer: ", s(error));
    emitEvents();
    return ok;
}

void NLBPlayer::set_muted(bool muted) {
    if (m_session) m_session->setMuted(muted);
}

Variant NLBPlayer::get_variable(const String& name) {
    return m_session ? toVariant(m_session->engine().getVariable(u(name))) : Variant();
}

void NLBPlayer::set_variable(const String& name, const Variant& value) {
    if (m_session) m_session->engine().setVariable(u(name), fromVariant(value));
}

Ref<Texture2D> NLBPlayer::load_texture(const String& path) {
    if (!m_session || path.is_empty()) return Ref<Texture2D>();
    if (m_textures.has(path)) return m_textures[path];
    std::vector<uint8_t> bytes;
    Ref<Texture2D> result;
    if (m_session->media().read(MediaRef{u(path)}, bytes) && !bytes.empty()) {
        PackedByteArray data;
        data.resize(static_cast<int64_t>(bytes.size()));
        memcpy(data.ptrw(), bytes.data(), bytes.size());
        Ref<Image> img;
        img.instantiate();
        const String lower = path.to_lower();
        Error err = lower.ends_with(".png") ? img->load_png_from_buffer(data)
                  : lower.ends_with(".webp") ? img->load_webp_from_buffer(data)
                  : img->load_jpg_from_buffer(data);
        if (err == OK) result = ImageTexture::create_from_image(img);
    }
    m_textures[path] = result;
    return result;
}
