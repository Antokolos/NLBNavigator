#pragma once

#include <map>
#include <functional>
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
    /// Какой INSTEAD-экспорт эмулировать: от него зависит, какие страницы считаются VN.
    /// VN (exportToVNSTEADFile): тема страницы DEFAULT трактуется как VN;
    /// Standard (exportToSTEADFile): VN — только страницы с явной темой VN.
    enum class ExportMode { VN, Standard };

    struct Settings {
        ExportMode exportMode = ExportMode::VN;
        /// _needs_action_count (config.xml: export/needs-action-count)
        int needsActionCount = 3;
        /// Тексты game.inv / game.nouse; пустые — взять по языку книги (config.xml)
        std::string gameInvText;
        std::string gameNouseText;
        /// Файл прогресса достижений (prefs INSTEAD); пусто — только в памяти
        std::string achievementsPath;
        unsigned randomSeed = std::random_device{}();
        /// Детерминированная случайность для сверки с INSTEAD (см. PlayerContext::setDeterministic)
        bool deterministic = false;
        /// Начальное значение счётчика детерминированной последовательности rnd
        int64_t deterministicStart = 0;
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
            Animation,    ///< анимированная картинка: text — шаблон кадра с %d, кадры 1..frames
            Sound,        ///< звуковой эффект (SFX)
            Music,        ///< фоновая музыка; пустой text — остановка музыки
            Achievement,  ///< "Получено достижение: ..."
            Info,         ///< служебное: URL, оформление окна, автопереходы
            Finish,       ///< конец игры
            ExternalGame  ///< новелла передала управление внешней игре (text — её имя)
        };
        Kind kind;
        std::string text;
        int frames = 0;
        /// Для картинок объектов — отображаемое имя объекта; пусто для картинки страницы
        std::string subject;
        /// Для картинок графических объектов — положение на экране ("right-top@-103,680")
        std::string position;
    };

    struct Choice {
        std::string text;
    };

    /// Расположение графического объекта — как его задаёт INSTEAD-экспорт (decorateObjEffect):
    /// корневой объект привязан к углу экрана (origin) со смещением (offset или arm); вложенный —
    /// к своему родителю (arm 0,0, дальше раскладывает графический движок). Экран 1920×1080
    struct Placement {
        bool graphical = false;
        std::string origin;        ///< left-top, middle-middle, right-bottom, ...
        std::string offset;        ///< смещение из книги ("1520,40"); пусто — используется arm
        int armX = 0, armY = 0;    ///< arm: координаты в редакторе, пересчитанные в экранные
        std::string parentId;      ///< экземпляр-родитель для вложенных объектов, иначе пусто
        std::string toString() const;  ///< "left-top@1520,40" / "left-top +160,153" / "+0,0"
    };

    struct ObjectView {
        std::string instanceId;
        std::string disp;
        bool takable = false;
        /// false — щелчок ничего не делает (STAT, надпись, подсказка): объект только для информации
        bool active = true;
        /// Положение графического объекта на экране (текстом; подробно — placement)
        std::string position;
        Placement placement;
        /// Уровень вложенности: 0 — лежит прямо на странице/в инвентаре, 1 — в объекте и т.д.
        int depth = 0;
        /// Свёрнут (collapsable): сам объект и его содержимое сейчас не видны
        bool collapsed = false;
    };

    struct PageView {
        std::string pageId;
        std::vector<Choice> choices;          ///< доступные неавтоматические ссылки
        std::vector<ObjectView> sceneObjects; ///< доступные объекты страницы
        std::vector<ObjectView> inventory;
        bool finished = false;                ///< ходов больше нет
    };

    PlayerEngine(NonLinearBook* rootBook, const Settings& settings);

    // ------------------------------------------------------------------ внешние игры
    /// Внешняя (нетекстовая) игра: мини-игра, 3D-сцена и т.п. Новелла передаёт ей управление
    /// командой ACT/ACTF с именем, которого нет среди объектов книги (play_bp_two_ais), и ждёт,
    /// пока интерфейс не вызовет finishExternalGame(). Пока игра идёт, обычные команды недоступны,
    /// а сама игра читает и меняет состояние новеллы через getVariable/setVariable и т.п.
    struct ExternalGame {
        std::string name;          ///< имя внешней игры (play_bp_two_ais)
        std::string returnPageId;  ///< страница, на которую вернётся новелла
    };
    /// Какие имена считаются внешними играми; по умолчанию — никакие (будет предупреждение)
    void setExternalGameFilter(std::function<bool(const std::string&)> isExternalGame);
    const std::optional<ExternalGame>& externalGame() const { return m_externalGame; }
    /// Завершить внешнюю игру: как walk(_returnto) в INSTEAD — возврат на страницу вызова
    /// с исполнением её модификаций (страница видит изменённые игрой переменные)
    bool finishExternalGame();

    // ------------------------------------------------------------------ доступ к состоянию
    /// Значение переменной; false, если такой нет
    cparse::packToken getVariable(const std::string& name) const;
    void setVariable(const std::string& name, const cparse::packToken& value);
    bool hasVariable(const std::string& name) const;
    /// Имена переменных книги и служебных (отсортированы)
    std::vector<std::string> variableNames() const;
    /// Предметы инвентаря: id экземпляров и имена объектов
    std::vector<std::pair<std::string, std::string>> inventoryItems() const;
    /// Положить объект (по имени) в инвентарь / убрать из инвентаря
    bool giveItem(const std::string& objName);
    bool takeItem(const std::string& objName);
    /// Выражение на языке условий книги (например, "money >= 100")
    cparse::packToken evaluate(const std::string& expression) const;

    /// Вход на стартовую страницу книги (или на указанную страницу)
    void start(const std::string& pageId = std::string());
    /// Новая игра с начала (_try_again в VN-экспорте); прогресс достижений сохраняется
    void restart();
    /// Страница показывается по правилам VN-экспорта
    bool isVnPage(const Page* page) const;

    /// Переход по доступной ссылке с индексом из view().choices
    bool choose(size_t choiceIndex);
    /// Клик по объекту на странице: tak для переносимых, иначе act
    bool actOnObject(const std::string& instanceId);
    /// Клик по объекту в инвентаре: use(s, s), иначе game.inv
    bool clickInventory(const std::string& instanceId);
    /// Применить объект инвентаря к объекту страницы или инвентаря
    bool use(const std::string& sourceInstanceId, const std::string& targetInstanceId);
    /// Подождать: один тик таймера страницы (в INSTEAD — раз в 200 мс) и проверка auto-ссылок
    bool wait();
    /// «Осмотреться» (look в INSTEAD): заново показать текущую страницу — картинки, текст,
    /// описания объектов — без исполнения её модификаций
    bool look();

    /// Сведения о файле сохранения (для списка слотов)
    struct SaveInfo {
        bool exists = false;
        bool compatible = false;  ///< сохранение сделано для этой книги
        std::string time;         ///< время сохранения, "ГГГГ-ММ-ДД ЧЧ:ММ"
        std::string label;        ///< где находился игрок (заголовок или начало текста страницы)
    };
    /// Сохранить игру (путь в UTF-8). Как и в INSTEAD, сохраняется состояние мира, а не история:
    /// после загрузки страница показывается заново, без исполнения её модификаций
    bool saveGame(const std::string& path, std::string& error) const;
    /// Можно ли сохраниться сейчас. Страница с флагом «не сохранять» (nosave) запрещает
    /// сохранение, как game.enable_save в nlb.lua (here().nosave -> false); загружаться можно
    bool canSave() const;
    bool loadGame(const std::string& path, std::string& error);
    SaveInfo readSaveInfo(const std::string& path) const;
    /// На странице есть таймер — ожидание имеет смысл
    bool hasTimer() const;

    PageView view() const;

    /// Описание ссылки для диагностики и сверки с экспортом
    struct LinkInfo {
        std::string kind;    ///< Normal, Traverse, Return, AutowiredIn, AutowiredOut
        std::string target;
        std::string text;
        bool autoFlag = false;
    };
    /// Все ссылки страницы (включая служебные) в том порядке, в каком их строит экспорт
    std::vector<LinkInfo> linksOf(const std::string& pageId) const;
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
        /// determineTrivialStatus: текст и alt-текст по умолчанию либо auto
        bool trivial = false;
        std::vector<Modification*> modifications;
        /// Для autowired: служебная переменная "W_P" и присваиваемое значение (модификация LinkLw)
        std::string autowiredVarId;
        bool autowiredValue = false;
    };

    std::vector<PlayerLink> buildLinks() const { return buildLinks(m_page); }
    std::vector<PlayerLink> buildLinks(Page* page) const;
    bool isLinkAvailable(const PlayerLink& link) const;
    /// Ссылки, которые игрок может выбрать на текущей странице (с учётом VN-тривиальных страниц)
    std::vector<PlayerLink> availableChoices() const;
    void createState();
    std::string moduleConstraintText(NonLinearBook* book) const;
    bool hasAction();
    /// Исполнение ссылки: модификации, переменные, переход. fromPageContext — auto-ссылка (s = страница)
    void followLink(const PlayerLink& link, bool fromPageContext);
    /// nlbwalk: вход на страницу. fromAutowired — переход совершается из autowired-страницы
    void walkTo(const std::string& pageId, bool fromAutowired);
    void enterPage(Page* page, bool fromAutowired);
    /// life()/autos(): счётчики, callback-объекты, auto-ссылки. true — был переход
    /// tick — прошёл один тик таймера страницы (переменная таймера +1)
    bool runAutos(bool tick);
    /// Сколько тиков таймера проматывать автоматически на текущей странице (0 — не ждать)
    int autoWaitTicks() const;
    static constexpr int MAX_TIMER_TICKS = 100000;
    /// ~10 с при тике 200 мс: пауза хода противника, короткие анимации
    static constexpr int SHORT_WAIT_TICKS = 50;
    /// playerTurn — ход игрока на текущей странице: сначала проходит один тик таймера
    void settle(bool playerTurn);
    void renderPage();
    /// После действия на той же странице: показать картинки, которые сменились (например,
    /// флажок «Показывать эту страницу») или появились (новые карты на столе)
    void refreshSceneImages();
    /// Действие игрока выполнено: auto-ссылки, затем обновление картинок, если страница не сменилась
    void finishAction(bool playerTurn = true);
    void emitObjectImage(const std::string& instanceId, const PlayerContext::OutputItem& image);
    static std::string imageKey(const PlayerContext::OutputItem& image);
    void flushOutput(bool afterPage);
    void appendMediaEvents(const std::vector<PlayerContext::OutputItem>& output);
    void setVarTrue(NonLinearBook* book, const std::string& varId);
    std::string objDisp(const std::string& instanceId) const;
    /// Подпись объекта на странице: текст метки {…} из dsc, иначе disp, иначе имя
    std::string sceneLabel(const std::string& instanceId) const;
    bool isVisibleInInventory(const std::string& instanceId) const;
    /// У объекта есть реакция на щелчок или он участвует в применении предметов
    bool hasBehaviour(const Obj* obj) const;
    /// "<угол>@<смещение>" для графического объекта (decorateObjEffect), иначе пусто
    std::string objPosition(const std::string& instanceId) const;
    /// ExportManager.getObjType: MENU — щелчок в инвентаре вызывает act; STAT — ничего не делает;
    /// OBJ — inv = use(s, s)
    enum class ObjKind { Obj, Menu, Stat };
    ObjKind objKind(const Obj* obj) const;
    /// Объекты, видимые на странице: содержимое комнаты и, рекурсивно, содержимое видимых
    /// объектов (INSTEAD показывает вложенные объекты после их контейнера)
    std::vector<std::string> sceneObjects() const;
    void collectSceneObjects(const std::string& ownerId, std::vector<std::string>& result, int depth) const;
    bool isOnScene(const std::string& instanceId) const;
    /// Предметы инвентаря вместе с содержимым предметов-контейнеров: INSTEAD показывает
    /// вложенные объекты инвентаря в панели (так в Frontier устроены кнопки действий боя)
    std::vector<std::string> inventoryObjects() const;
    bool isInInventoryTree(const std::string& instanceId) const;
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
    std::optional<ExternalGame> m_externalGame;
    /// Сохраняемые (preserved) графические объекты, которые VN-движок INSTEAD оставляет на экране
    /// при смене страниц: кнопки «Навыки»/«Инвентарь», свитки. Корни, в порядке появления
    std::vector<std::string> m_preservedRoots;
    /// Глубина объекта в дереве сцены, как его обходит sceneObjects(): объект может лежать и
    /// в других контейнерах (ADD не убирает из прежнего места), поэтому по containerOf её не найти
    mutable std::map<std::string, int> m_sceneDepth;
    void rememberPreservedObjects();
    /// Картинки preserved-объектов, уже показанные игроку: на новой странице они остаются на
    /// экране, поэтому повторно выводятся только при изменении
    std::map<std::string, std::string> m_preservedImagesShown;
    std::optional<std::string> m_pendingExternalGame;
    std::function<bool(const std::string&)> m_isExternalGame;
    void startPendingExternalGame();
    Placement objPlacement(const std::string& instanceId) const;
    /// Показанные картинки: страницы (ключ — id страницы) и объектов на ней
    std::map<std::string, std::string> m_shownImages;
    int m_renderCount = 0;
    /// Фоновая музыка, которая сейчас играет (последнее событие Music) — сохраняется в игре
    std::string m_currentMusic;
    /// Начало событий последней показанной страницы — чтобы убрать «пустые» промежуточные страницы
    size_t m_lastRenderStart = 0;
};
