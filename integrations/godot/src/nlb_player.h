#pragma once

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

#include <memory>

#include "nlb/presentation/Session.h"

class GodotAudioBackend;

/**
 * @brief Узел Godot с плеером книг NLB.
 *
 * Сцена Godot сама решает, как рисовать: узел отдаёт снимок экрана (get_screen() — Dictionary),
 * сигналы screen_changed / achievement / external_game / game_finished и принимает щелчки.
 * Звук — средствами Godot (дочерние AudioStreamPlayer), время идёт в _process.
 */
class NLBPlayer : public godot::Node {
    GDCLASS(NLBPlayer, godot::Node)

public:
    NLBPlayer();
    ~NLBPlayer() override;

    void _ready() override;
    void _process(double delta) override;
    void _exit_tree() override;

    // ---- свойства
    void set_book_path(const godot::String& path) { m_bookPath = path; }
    godot::String get_book_path() const { return m_bookPath; }
    void set_visual_novel(bool vn) { m_visualNovel = vn; }
    bool get_visual_novel() const { return m_visualNovel; }
    void set_external_games(const godot::PackedStringArray& games) { m_externalGames = games; }
    godot::PackedStringArray get_external_games() const { return m_externalGames; }
    /// Журнал прохождения, как --log у консольного плеера (путь на диске; пусто — без журнала)
    void set_log_path(const godot::String& path);
    /// Игровые площадки: «steam», «epic», «log» или «log:<файл>» (достижения, статистика и
    /// рекорды книги по её platform.ini). Steam и Epic — если модуль собран с их SDK
    void set_platforms(const godot::PackedStringArray& platforms) { m_platforms = platforms; }
    /// Язык книги по умолчанию для этой игры («ru», «en»…), главнее языка системы; пусто —
    /// язык лаунчера/системы, иначе английский. Выбор игрока в меню всё равно главнее
    void set_language(const godot::String& lang) { m_language = lang; }
    godot::String get_language() const { return m_language; }
    godot::PackedStringArray get_platforms() const { return m_platforms; }
    godot::String get_log_path() const { return m_logPath; }
    void set_autostart(bool on) { m_autostart = on; }
    bool get_autostart() const { return m_autostart; }

    // ---- управление
    bool open();
    godot::Dictionary get_screen() const;
    int64_t get_revision() const;
    bool choose(int64_t index);
    bool click_object(const godot::String& id);
    bool click_item(const godot::String& id);
    void clear_selection();
    godot::String get_selected_item() const;
    bool finish_external_game();
    void restart();
    /// «Попробовать ещё раз» в конце игры: к снимку SNAPSHOT, если он есть, иначе restart()
    void try_again();
    bool save_game(int64_t slot);
    bool load_game(int64_t slot);
    void set_muted(bool muted);
    /// Переменные новеллы (для мини-игр и сцен, которым передано управление)
    godot::Variant get_variable(const godot::String& name);
    void set_variable(const godot::String& name, const godot::Variant& value);
    /// Картинка книги (путь из снимка экрана) как текстура; кэшируется
    godot::Ref<godot::Texture2D> load_texture(const godot::String& path);

    // ---- для экрана NLBScreen (C++)
    nlb::presentation::Session* session() const { return m_session.get(); }
    /// Время книги и события ведёт экран (GameScreen), а не _process узла
    void set_screen_driven(bool driven) { m_screenDriven = driven; }
    /// Сигналы (достижения, внешние игры, конец) по событиям, которые забрал экран
    void dispatch_events(const std::vector<PlayerEngine::Event>& events);
    /// Тема игры, зашитая в пакет (pack --theme); пусто — тема по умолчанию
    godot::String theme_dir() const;
    /// Базовая тема, зашитая в пакет (pack --base-theme); пусто — нет
    godot::String base_theme_dir() const;

protected:
    static void _bind_methods();

private:
    void emitEvents();
    godot::String resolveBookPath() const;

    godot::String m_bookPath;
    bool m_visualNovel = true;
    bool m_autostart = true;
    godot::PackedStringArray m_externalGames;
    godot::String m_logPath;
    godot::PackedStringArray m_platforms;
    godot::String m_language;
    std::unique_ptr<GodotAudioBackend> m_audio;
    std::unique_ptr<nlb::presentation::Session> m_session;
    uint64_t m_lastRevision = 0;
    godot::Dictionary m_textures;
    bool m_screenDriven = false;
    std::string m_openedPath;
};

/// Картинка из байтов: формат по содержимому (PNG, JPG, WEBP), иначе по расширению
godot::Error nlbLoadImage(const godot::Ref<godot::Image>& img, const godot::PackedByteArray& data, const godot::String& path);
