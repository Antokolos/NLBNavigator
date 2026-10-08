#pragma once

#include <memory>
#include <optional>
#include <random>
#include <string>
#include <vector>

#include "nlb/domain/PlayerContext.h"
#include "nlb/domain/ModificationInterpreter.h"

class NonLinearBook;
class Page;
class Modification;

/**
 * @brief Движок проигрывания NLB-книги (этап D плана), без пользовательского интерфейса.
 *
 * Эталон — INSTEAD-экспорт (ExportManager.createPageBuildingBlocks / createLinkBuildingBlocks,
 * STEADExportManager: enter/autos/xact, nlb.lua). NonLinearBookImpl::createFilteredPage
 * не используется: служебные ссылки (Traverse, Return, AutowiredIn/Out) строятся здесь.
 *
 * Консольный или графический интерфейс вызывает start(), затем команды игрока
 * (choose/actOnObject/clickInventory/use) и после каждой забирает takeEvents() и view().
 */
class PlayerEngine {
public:
    struct Settings {
        /// _needs_action_count (config.xml: export/needs-action-count)
        int needsActionCount = 3;
        /// Тексты game.inv / game.nouse; пустые — взять по языку книги (config.xml)
        std::string gameInvText;
        std::string gameNouseText;
        /// Файл прогресса достижений (prefs INSTEAD); пусто — только в памяти
        std::string achievementsPath;
        unsigned randomSeed = std::random_device{}();
    };

    /// Элемент вывода для интерфейса, в порядке показа
    struct Event {
        enum class Kind {
            PageCaption,  ///< заголовок страницы
            PageText,     ///< текст страницы с подставленными $var$
            ObjectText,   ///< описание объекта на странице (dsc)
            AltText,      ///< альтернативный текст недоступной ссылки
            Text,         ///< текст из модификаций и действий (PRN, ACT, USE, ...)
            Image,        ///< имя файла картинки
            Sound,        ///< имя файла звука
            Achievement,  ///< "Получено достижение: ..."
            Info,         ///< служебное: URL, оформление окна, автопереходы
            Finish        ///< конец игры
        };
        Kind kind;
        std::string text;
    };

    struct Choice {
        std::string text;
    };

    struct ObjectView {
        std::string instanceId;
        std::string disp;
        bool takable = false;
    };

    struct PageView {
        std::string pageId;
        std::vector<Choice> choices;          ///< доступные неавтоматические ссылки
        std::vector<ObjectView> sceneObjects; ///< доступные объекты страницы
        std::vector<ObjectView> inventory;
        bool finished = false;                ///< ходов больше нет
    };

    PlayerEngine(NonLinearBook* rootBook, const Settings& settings);

    /// Вход на стартовую страницу книги (или на указанную страницу)
    void start(const std::string& pageId = std::string());

    /// Переход по доступной ссылке с индексом из view().choices
    bool choose(size_t choiceIndex);
    /// Клик по объекту на странице: tak для переносимых, иначе act
    bool actOnObject(const std::string& instanceId);
    /// Клик по объекту в инвентаре: use(s, s), иначе game.inv
    bool clickInventory(const std::string& instanceId);
    /// Применить объект инвентаря к объекту страницы или инвентаря
    bool use(const std::string& sourceInstanceId, const std::string& targetInstanceId);

    PageView view() const;
    std::vector<Event> takeEvents();
    bool isFinished() const;

    PlayerContext& context() { return *m_context; }
    ModificationInterpreter& interpreter() { return *m_interpreter; }
    const Page* currentPage() const { return m_page; }

private:
    enum class LinkKind { Normal, Traverse, Return, AutowiredIn, AutowiredOut };

    /// Ссылка в том виде, в каком её видит INSTEAD-экспорт (Link или LinkLw)
    struct PlayerLink {
        LinkKind kind = LinkKind::Normal;
        std::string id;
        std::string target;
        std::string text;
        std::string altText;
        std::string constrId;
        std::string varId;
        bool autoFlag = false;
        bool needsAction = false;
        bool once = false;
        bool positive = true;
        bool obeyModule = false;
        std::vector<Modification*> modifications;
        /// Для autowired: служебная переменная "W_P" и присваиваемое значение (модификация LinkLw)
        std::string autowiredVarId;
        bool autowiredValue = false;
    };

    std::vector<PlayerLink> buildLinks() const;
    bool isLinkAvailable(const PlayerLink& link) const;
    std::string moduleConstraintText(NonLinearBook* book) const;
    bool hasAction();
    /// Исполнение ссылки: модификации, переменные, переход. fromPageContext — auto-ссылка (s = страница)
    void followLink(const PlayerLink& link, bool fromPageContext);
    /// nlbwalk: вход на страницу. fromAutowired — переход совершается из autowired-страницы
    void walkTo(const std::string& pageId, bool fromAutowired);
    void enterPage(Page* page, bool fromAutowired);
    /// life()/autos(): счётчики, callback-объекты, auto-ссылки. true — был переход
    bool runAutos();
    void settle();
    void renderPage();
    void flushOutput(bool afterPage);
    void setVarTrue(NonLinearBook* book, const std::string& varId);
    std::string objDisp(const std::string& instanceId) const;
    std::string nouseText(const std::string& instanceId) const;

    NonLinearBook* m_rootBook;
    Settings m_settings;
    std::unique_ptr<PlayerContext> m_context;
    std::unique_ptr<ModificationInterpreter> m_interpreter;
    Page* m_page = nullptr;
    NonLinearBook* m_book = nullptr;
    int64_t m_actionCountDiffPrev = 0;
    int m_transitions = 0;
    std::optional<std::string> m_pendingWalk;
    bool m_pendingFromAutowired = false;
    std::vector<Event> m_events;
};
