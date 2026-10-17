#pragma once

#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "nlb/domain/PlayerEngine.h"
#include "nlb/presentation/ScreenModel.h"

class NonLinearBookImpl;

namespace nlb::presentation {

class IAudioBackend;
class IMediaSource;
class AudioDirector;

/**
 * @brief Игровая сессия для встраивания плеера в движок (Godot, Unreal) или свой фронтенд.
 *
 * Собирает вместе всё, что фронтенду нужно, кроме самой отрисовки: книгу (каталог или .nlbpack),
 * движок в режиме реального времени, звук (AudioDirector поверх IAudioBackend фронтенда),
 * снимок экрана и правила щелчков INSTEAD (выбор предмета, применение, осмотр). Фронтенд:
 *   1) раз в кадр вызывает advance(dt) и, если revision() изменился, перерисовывает screen();
 *   2) на щелчки вызывает choose / clickObject / clickItem;
 *   3) разбирает takeEvents(): достижения, внешние игры, конец игры (звук уже обработан).
 * Состояние новеллы для мини-игр и 3D-сцен — через engine() (getVariable/setVariable, ...).
 */
class Session {
public:
    struct Options {
        std::string bookPath;                   ///< каталог книги или .nlbpack (UTF-8)
        PlayerEngine::ExportMode exportMode = PlayerEngine::ExportMode::VN;
        std::string achievementsPath;           ///< пусто — достижения только в памяти
        std::string savesDir;                   ///< пусто — сохранения недоступны
        /// Внешние игры, которые есть у фронтенда: имя или «префикс*» (play_bp_*). Вызов ACT/ACTF
        /// с таким именем передаёт управление фронтенду (событие ExternalGame)
        std::vector<std::string> externalGames;
    };

    /// audio — бэкенд звука фронтенда (может быть nullptr: без звука)
    explicit Session(IAudioBackend* audio = nullptr);
    ~Session();
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    bool open(const Options& options, std::string& error);
    bool isOpen() const { return m_engine != nullptr; }
    /// Медиафайлы книги (картинки для текстур фронтенда)
    const IMediaSource& media() const;
    PlayerEngine& engine() { return *m_engine; }
    std::string title() const;
    std::string language() const;

    // ---- время и снимок
    void advance(int milliseconds);
    const ScreenModel& screen() const { return m_screen; }
    uint64_t revision() const { return m_screen.revision + m_localRevision; }

    // ---- щелчки игрока (правила INSTEAD)
    bool choose(int choiceIndex);
    /// Щелчок по объекту сцены: если выбран предмет — применить его к объекту, иначе действие
    bool clickObject(const std::string& objectId);
    /// Щелчок по предмету: выбрать; повторно — осмотр; при выбранном другом — применить к нему
    bool clickItem(const std::string& itemId);
    void clearSelection();
    const std::string& selectedItem() const { return m_selected; }

    // ---- служебное
    /// Забрать события движка и обновить снимок — после команд, отданных напрямую engine()
    void refresh() { afterCommand(); }
    /// «Осмотреться»: снова описание сцены (заголовок комнаты в INSTEAD)
    void look();
    /// Журнал прохождения, как --log у консольного плеера: что было на экране (заголовок, текст,
    /// объекты, инвентарь, переходы) и что выбрал игрок — после каждой команды. Пустой путь —
    /// закрыть журнал. Если книга уже открыта, сразу записывается текущий экран
    bool setLogFile(const std::string& path, std::string& error);
    std::vector<PlayerEngine::Event> takeEvents();
    bool finishExternalGame();
    void restart();
    bool save(int slot, std::string& error);
    bool load(int slot, std::string& error);
    PlayerEngine::SaveInfo saveInfo(int slot) const;
    /// Режим экспорта, в котором открыта книга (VN или Standard)
    PlayerEngine::ExportMode exportMode() const { return m_options.exportMode; }
    /// Сохранения доступны (задан каталог сохранений)
    bool savesAvailable() const { return !m_options.savesDir.empty(); }
    void setMuted(bool muted);

private:
    void afterCommand();
    std::string slotPath(int slot) const;
    void logAction(const std::string& action);
    void logScreen();

    Options m_options;
    IAudioBackend* m_audio = nullptr;
    std::unique_ptr<NonLinearBookImpl> m_book;
    std::unique_ptr<IMediaSource> m_media;
    std::unique_ptr<PlayerEngine> m_engine;
    std::unique_ptr<AudioDirector> m_director;
    ScreenModel m_screen;
    std::vector<PlayerEngine::Event> m_pending;
    std::string m_selected;
    uint64_t m_localRevision = 0;  ///< выбор предмета меняет вид, но не снимок движка
    std::unique_ptr<std::ofstream> m_log;  ///< журнал прохождения (setLogFile)
    std::string m_logPage;                 ///< страница, описанная в журнале последней
    uint64_t m_logRevision = 0;            ///< ревизия снимка, записанного последним
};

}  // namespace nlb::presentation
