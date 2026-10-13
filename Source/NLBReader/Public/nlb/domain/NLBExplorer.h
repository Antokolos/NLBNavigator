#pragma once

#include "nlb/domain/PlayerEngine.h"
#include <functional>
#include <map>
#include <utility>
#include <vector>
#include <iostream>
#include <memory>
#include <string>

class NonLinearBook;

/**
 * @brief Консольный интерфейс проигрывателя NLB-книги.
 *
 * Вся игровая логика — в PlayerEngine; здесь только вывод событий и разбор команд:
 *   N        — перейти по ссылке N
 *   oN       — действие с объектом N на странице (переносимый объект берётся в инвентарь)
 *   iN       — щелчок по предмету N в инвентаре
 *   uN oM    — применить предмет инвентаря N к объекту страницы M (uN iM — к предмету инвентаря)
 *   w        — подождать (тик таймера страницы)
 *   l        — осмотреться: заново показать текущую страницу
 *   m        — меню: сохранить, загрузить, начать заново, выйти
 *   0        — выход (с подтверждением)
 */
class NLBExplorer {
public:
    NLBExplorer(NonLinearBook* book, const std::string& startPageId,
                const PlayerEngine::Settings& settings = PlayerEngine::Settings(),
                std::istream& in = std::cin, std::ostream& out = std::cout);

    /// Интерактивная игра до конца книги или команды выхода
    void explore();
    /// Обработчик внешней (нетекстовой) игры в консоли. Получает движок (полный доступ к
    /// переменным и инвентарю) и потоки ввода-вывода; по возврату управление уходит новелле
    using LineReader = std::function<bool(std::string& line)>;
    using ExternalGameHandler = std::function<void(PlayerEngine& engine, const LineReader& readLine, std::ostream& out)>;
    /// name — имя игры; "префикс*" — все игры с таким префиксом
    void registerExternalGame(const std::string& name, ExternalGameHandler handler);
    /// События движка (картинки, музыка, звуки...) до их вывода в консоль — например, для звука
    using EventListener = std::function<void(const std::vector<PlayerEngine::Event>&)>;
    void setEventListener(EventListener listener) { m_eventListener = std::move(listener); }
    /// Переключатель звука для меню: получает «включить?» и возвращает, включён ли звук теперь.
    /// Не задан — в меню нет пункта о звуке (плеер без звука)
    using SoundSwitch = std::function<bool(bool enable)>;
    void setSoundSwitch(SoundSwitch soundSwitch, bool enabled) { m_soundSwitch = std::move(soundSwitch); m_soundOn = enabled; }
    /// Режим для экранного диктора (NVDA, JAWS, Orca): без картинок, координат и имён файлов,
    /// списки по строке на пункт, после действия — только изменения, команды для навигации на слух
    void setScreenReaderMode(bool enabled) { m_screenReader = enabled; }
    /// Каталог файлов сохранения (UTF-8); пусто — сохранения недоступны
    void setSavesDirectory(const std::string& utf8Dir) { m_savesDir = utf8Dir; }
    /// Вызывается для каждой введённой строки (журнал сессии)
    void setInputListener(std::function<void(const std::string&)> listener) { m_inputListener = std::move(listener); }

private:
    void printEvents();
    void printView(const PlayerEngine::PageView& view);
    /// false — выход
    bool handleCommand(const std::string& line, const PlayerEngine::PageView& view);

    NonLinearBook* m_book;
    std::string m_startPageId;
    std::unique_ptr<PlayerEngine> m_engine;
    std::istream& m_in;
    std::ostream& m_out;
    std::function<void(const std::string&)> m_inputListener;
    bool readLine(std::string& line);
    /// Меню; false — выход из игры
    bool runMenu();
    bool confirm(const std::string& question);
    void saveMenu();
    void loadMenu();
    std::string slotPath(int slot) const;
    std::string m_savesDir;
    EventListener m_eventListener;
    SoundSwitch m_soundSwitch;
    bool m_soundOn = false;
    std::vector<PlayerEngine::Event> takeEvents();
    std::vector<std::pair<std::string, ExternalGameHandler>> m_externalGames;
    const ExternalGameHandler* findExternalGame(const std::string& name) const;
    void runExternalGame();
    void registerDefaultExternalGames();

    // ------------------------------------------------------------- режим экранного диктора
    bool m_screenReader = false;
    void exploreScreenReader();
    /// Печатает события; true — была выведена страница целиком (переход, «осмотреться»)
    bool srPrintEvents();
    void srAssignNumbers(const PlayerEngine::PageView& view);
    void srPrintChoices(const PlayerEngine::PageView& view);
    void srPrintScene(const PlayerEngine::PageView& view, bool withReadOnly);
    void srPrintInventory(const PlayerEngine::PageView& view);
    void srPrintChanges(const PlayerEngine::PageView& before, const PlayerEngine::PageView& after);
    /// Изменения инвентаря (показатели «Жизнь», «Энергия» и т.п.) — и при смене страницы
    void srPrintInventoryChanges(const PlayerEngine::PageView& before, const PlayerEngine::PageView& after);
    void srPrintHelp();
    bool srHandleCommand(const std::string& line, const PlayerEngine::PageView& view);
    bool srApply(const PlayerEngine::PageView& view, const std::string& args);
    std::string srObjectId(const PlayerEngine::PageView& view, bool inventory, int number) const;
    /// Номера объектов и предметов постоянны в пределах страницы: взятый предмет не сдвигает
    /// нумерацию остальных
    std::string m_srPageId;
    std::map<std::string, int> m_srSceneNumbers, m_srInventoryNumbers;
    int m_srNextScene = 1, m_srNextInventory = 1;
    static constexpr int SAVE_SLOTS = 9;
};
