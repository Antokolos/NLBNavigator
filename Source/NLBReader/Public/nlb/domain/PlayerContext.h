#pragma once

#include <deque>
#include <map>
#include <memory>
#include <optional>
#include <random>
#include <set>
#include <string>
#include <vector>

#include "shunting-yard.h"

class NonLinearBook;

/**
 * @brief Состояние проигрывателя NLB-книги (этап B плана).
 *
 * Хранит всё, что меняется во время игры; сама книга остаётся неизменяемой.
 * Семантика операций взята из рантайма INSTEAD-экспорта
 * (NLB/NLBW/res/vnstead/modules/nlb.lua) и STEADExportManager.java.
 *
 * Объекты адресуются "экземплярами" (instanceId). У исходного объекта instanceId == objId,
 * клон получает instanceId = "<objId>#<n>" и помнит прототип (как nlb:clonefd в nlb.lua).
 *
 * Класс не исполняет модификации (это этап C) и не рисует страницы (этап D):
 * он только хранит состояние и предоставляет примитивы.
 */
class PlayerContext {
public:
    /// Вид элемента вывода. Плеер (этап D) решает, где его показать:
    /// Image/Sound — перед текстом страницы, Achievement — после.
    /// Image/Animation — картинка; Sound — звуковой эффект (SFX); Music — фоновая музыка
    /// (пустой text — остановка музыки, VOID в NLB)
    enum class OutputKind { Text, Image, Animation, Sound, Music, Achievement, Info };

    struct OutputItem {
        OutputKind kind;
        /// Для Animation — шаблон имени кадра с %d (string.format в STEAD), кадры 1..frames
        std::string text;
        int frames = 0;
    };

    /// Типы статистики для COUNTGET — битовая маска как в nlb.lua count_get (0 = все).
    enum CountType : int { CountInv = 1, CountAct = 2, CountUse = 4, CountWalk = 8 };

    explicit PlayerContext(NonLinearBook* rootBook, unsigned randomSeed = std::random_device{}());

    // ------------------------------------------------------------------ переменные
    /// Область видимости cparse с переменными книги (дочерняя к default_global,
    /// поэтому встроенные функции cparse доступны). Имена глобальны, как в STEAD.
    cparse::TokenMap& scope() { return m_scope; }
    const cparse::TokenMap& scope() const { return m_scope; }

    /// Инициализирует отсутствующие переменные книги и всех её модулей значениями по умолчанию
    /// по типу данных, как initializeVariables() STEAD-экспорта (ExportManager.getDefaultValue):
    /// NUMBER -> 0, STRING -> "", BOOLEAN/AUTO -> false. Уже существующие не трогает.
    void ensureBookVars(const NonLinearBook* book);
    bool hasVar(const std::string& name) const;
    void setVar(const std::string& name, const cparse::packToken& value);
    /// Отсутствующая переменная -> false (как prepareEngine в Java-плеере)
    cparse::packToken getVar(const std::string& name) const;

    // ------------------------------------------------------------------ списки (nlb.lua _lists)
    /// push/pop работают с ГОЛОВОЙ списка, inject/eject — с ХВОСТОМ (nlb.lua:25-75).
    void listPush(const std::string& list, const std::string& value);
    std::optional<std::string> listPop(const std::string& list);
    void listInject(const std::string& list, const std::string& value);
    std::optional<std::string> listEject(const std::string& list);
    /// Удаляет первое вхождение (nlb.lua rmv)
    void listRemove(const std::string& list, const std::string& value);
    void listShuffle(const std::string& list);
    void listClear(const std::string& list);
    size_t listSize(const std::string& list) const;
    bool listExists(const std::string& list) const;
    /// Пустой вектор, если списка нет
    std::vector<std::string> listItems(const std::string& list) const;

    // ------------------------------------------------------------------ инвентарь
    bool inInventory(const std::string& instanceId) const;
    /// Добавляет экземпляр, если его там ещё нет
    void inventoryAdd(const std::string& instanceId);
    void inventoryRemove(const std::string& instanceId);
    void inventoryClear();
    const std::vector<std::string>& inventory() const { return m_state.inventory; }

    // ------------------------------------------------------------------ контейнеры (objs(target))
    /// Владелец — id страницы или экземпляра объекта
    const std::vector<std::string>& containerContents(const std::string& ownerId) const;
    bool containerHas(const std::string& ownerId, const std::string& instanceId) const;
    /// Кладёт экземпляр в контейнер (без дублей) и запоминает его владельца.
    /// Из прежнего места НЕ удаляет — как objs(target):add в STEAD.
    void containerAdd(const std::string& ownerId, const std::string& instanceId);
    /// Удаляет из контейнера и сбрасывает владельца (как decorateDelObj: container = nil)
    void containerRemove(const std::string& ownerId, const std::string& instanceId);
    void containerClear(const std::string& ownerId);
    /// Последний владелец экземпляра; пустая строка — нигде
    std::string containerOf(const std::string& instanceId) const;

    // ------------------------------------------------------------------ клоны
    /// Создаёт клон экземпляра (тег копируется — deepcopy в nlb.lua) и возвращает его instanceId
    std::string cloneInstance(const std::string& instanceId);
    /// id исходного объекта книги; для не-клона возвращает сам instanceId
    std::string protoOf(const std::string& instanceId) const;
    bool isClone(const std::string& instanceId) const;

    // ------------------------------------------------------------------ теги
    std::optional<std::string> tag(const std::string& instanceId) const;
    void setTag(const std::string& instanceId, const std::string& tag);

    // ------------------------------------------------------------------ достижения
    /// Достижения переживают SNAPSHOT/restore (в INSTEAD они в prefs, а не в сохранении игры).
    void setAchievementMax(const std::string& name, int max);
    /// nlb.lua setAchievement: прогресс = число РАЗНЫХ modificationId.
    /// При первом получении печатает "Получено достижение: NAME"; проверяет perfect game.
    void achieve(const std::string& name, const std::string& modificationId);
    int achievementCount(const std::string& name) const;
    bool isAchievementGranted(const std::string& name) const;
    const std::string& perfectGameAchievementName() const { return m_perfectGameName; }
    /// Достижения переживают перезапуск игры, как prefs в INSTEAD. Задаёт файл хранения:
    /// загружает из него сохранённый прогресс (без повторных объявлений) и далее
    /// сохраняет после каждого изменения. Пустой путь — только в памяти.
    /// Возвращает false, если существующий файл не удалось прочитать.
    bool setAchievementsStorage(const std::string& path);

    // ------------------------------------------------------------------ счётчики (COUNTGET/COUNTRST)
    void countIncrement(CountType type);
    int64_t countGet(int statType) const;
    void countReset();

    // ------------------------------------------------------------------ SNAPSHOT
    void makeSnapshot();
    bool hasSnapshot() const;
    /// Восстанавливает игровое состояние; достижения и очередь вывода не трогает
    bool restoreSnapshot();

    // ------------------------------------------------------------------ вывод
    void emit(OutputKind kind, const std::string& text, int frames = 0);
    /// Забирает накопленный вывод (очередь очищается)
    std::vector<OutputItem> takeOutput();

    // ------------------------------------------------------------------ навигация
    void requestGoto(const std::string& pageId);
    /// Забирает запрошенный GOTO (не более одного)
    std::optional<std::string> takeGoto();
    void recordPageVisit(const std::string& pageId);
    const std::string& currentPageId() const;
    /// Предыдущая страница ("ww" в STEAD-экспорте); пусто, если её нет
    const std::string& previousPageId() const;
    /// История посещённых страниц (для отладки и инструментов)
    const std::vector<std::string>& pageHistory() const { return m_state.pageHistory; }
    void recordLinkFollowed(const std::string& linkId);
    bool wasLinkFollowed(const std::string& linkId) const;

    // ------------------------------------------------------------------ случайные числа
    /// Равномерно на [1, max] (INSTEAD rnd); max < 1 -> 1
    int64_t random(int64_t max);

private:
    /// Игровое состояние, которое сохраняет SNAPSHOT
    struct GameState {
        std::map<std::string, std::deque<std::string>> lists;
        std::vector<std::string> inventory;
        std::map<std::string, std::vector<std::string>> containers;
        std::map<std::string, std::string> containerOf;
        std::map<std::string, std::string> cloneProto;
        std::map<std::string, int> cloneCounters;
        std::map<std::string, std::string> tags;
        std::map<std::string, int64_t> counts;
        std::map<std::string, int64_t> countsAtReset;
        std::vector<std::string> pageHistory;
        std::set<std::string> followedLinks;
        std::set<const NonLinearBook*> initializedBooks;
    };

    void initContainers(NonLinearBook* book);
    void registerAchievements(NonLinearBook* rootBook);
    void announceAchievement(const std::string& name);
    void saveAchievements() const;
    bool loadAchievements();
    static std::string countKey(CountType type);

    NonLinearBook* m_rootBook;
    cparse::TokenMap m_scope;
    GameState m_state;

    // Снимок: переменные копируются поэлементно (TokenMap имеет ссылочную семантику)
    bool m_hasSnapshot = false;
    std::map<std::string, cparse::packToken> m_snapshotVars;
    GameState m_snapshotState;

    // Достижения (вне снимка)
    std::map<std::string, std::set<std::string>> m_achievementIds;
    std::map<std::string, int> m_achievementMax;
    std::set<std::string> m_grantedAchievements;
    std::string m_perfectGameName;
    std::string m_achievementsPath;

    std::vector<OutputItem> m_output;
    std::optional<std::string> m_goto;
    std::mt19937_64 m_rng;
};
