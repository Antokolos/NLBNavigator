#include "nlb/domain/NLBExplorer.h"

#include "nlb/api/NonLinearBook.h"

#include <algorithm>
#include <sstream>

NLBExplorer::NLBExplorer(NonLinearBook* book, const std::string& startPageId,
                         const PlayerEngine::Settings& settings, std::istream& in, std::ostream& out)
    : m_book(book), m_startPageId(startPageId), m_in(in), m_out(out) {
    m_engine = std::make_unique<PlayerEngine>(book, settings);
    m_engine->setExternalGameFilter([this](const std::string& name) { return findExternalGame(name) != nullptr; });
    registerDefaultExternalGames();
}

void NLBExplorer::explore() {
    if (m_screenReader) {
        exploreScreenReader();
        return;
    }
    try {
        m_engine->start(m_startPageId);
        while (true) {
            printEvents();
            if (m_engine->externalGame()) {
                runExternalGame();
                continue;
            }
            const PlayerEngine::PageView view = m_engine->view();
            if (view.finished) {
                // VN-экспорт на последней странице предлагает начать заново (_try_again)
                if (!m_engine->isVnPage(m_engine->currentPage())) {
                    break;
                }
                m_out << "\n1. Начать заново\n0. Выход\n\nВаш выбор: " << std::flush;
                std::string line;
                if (!readLine(line) || line != "1") {
                    break;
                }
                m_engine->restart();
                continue;
            }
            printView(view);
            m_out << "\nВаш выбор: " << std::flush;
            std::string line;
            if (!readLine(line)) {
                break;
            }
            if (!handleCommand(line, view)) {
                break;
            }
        }
    } catch (const std::exception& e) {
        m_out << "\n!!! Ошибка исполнения книги: " << e.what() << std::endl;
    }
}

bool NLBExplorer::readLine(std::string& line) {
    if (!std::getline(m_in, line)) {
        return false;
    }
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (m_inputListener) m_inputListener(line);
    return true;
}

void NLBExplorer::printEvents() {
    using Kind = PlayerEngine::Event::Kind;
    for (const auto& event : takeEvents()) {
        switch (event.kind) {
            case Kind::PageCaption: m_out << "\n" << std::string(50, '=') << "\n" << event.text << "\n"
                                          << std::string(50, '=') << std::endl; break;
            case Kind::PageText:    m_out << "\n" << event.text << std::endl; break;
            case Kind::ObjectText:  m_out << event.text << std::endl; break;
            case Kind::AltText:     m_out << event.text << std::endl; break;
            case Kind::Text:        m_out << event.text << std::endl; break;
            case Kind::Image:       m_out << "[IMAGE: " << event.text
                                          << (event.subject.empty() ? "" : " — " + event.subject)
                                          << (event.position.empty() ? "" : " @ " + event.position) << "]" << std::endl; break;
            case Kind::Animation: {
                // В INSTEAD кадры меняются по таймеру; в консоли — только описание анимации
                auto frame = [&](int n) {
                    std::string name = event.text;
                    const auto pos = name.find("%d");
                    if (pos != std::string::npos) name.replace(pos, 2, std::to_string(n));
                    return name;
                };
                m_out << "[ANIMATION: " << frame(1) << " ... " << frame(event.frames)
                      << ", кадров: " << event.frames << ", смена по таймеру"
                      << (event.subject.empty() ? "" : " — " + event.subject)
                      << (event.position.empty() ? "" : " @ " + event.position) << "]" << std::endl;
                break;
            }
            case Kind::Sound:       m_out << "[SOUND: " << event.text << "]" << std::endl; break;
            case Kind::Music:       m_out << (event.text.empty() ? std::string("[MUSIC: стоп]")
                                                                 : "[MUSIC: " + event.text + "]") << std::endl; break;
            case Kind::Achievement: m_out << "*** " << event.text << " ***" << std::endl; break;
            case Kind::Info:        m_out << event.text << std::endl; break;
            case Kind::Finish:      m_out << "\n=== КОНЕЦ ===" << std::endl; break;
            case Kind::ExternalGame: break;  // обрабатывается в runExternalGame
        }
    }
}

namespace {
/// Подпись объекта в списке: многострочную (панель HP и т.п.) сводим в одну строку
std::string oneLine(const std::string& s) {
    std::string r;
    for (char c : s) r += (c == '\n') ? ' ' : c;
    return r;
}
std::string withPosition(const PlayerEngine::ObjectView& o) {
    return o.position.empty() ? oneLine(o.disp) : oneLine(o.disp) + " [" + o.position + "]";
}
}

void NLBExplorer::printView(const PlayerEngine::PageView& view) {
    // Активные объекты получают номера oN/iN; неактивные (надписи, STAT) — только для чтения, «· »
    auto printList = [&](const char* title, const std::vector<PlayerEngine::ObjectView>& list, char prefix) {
        if (list.empty()) return;
        // Графические объекты (иконки на свитках, кнопки) выводятся деревом: по строке на объект,
        // с отступом по вложенности и положением на экране — как их потом разложит графический UI
        const bool tree = std::any_of(list.begin(), list.end(), [](const PlayerEngine::ObjectView& o) {
            return o.placement.graphical || o.depth > 0;
        });
        size_t n = 0;
        if (tree) {
            m_out << title << std::endl;
            for (const auto& o : list) {
                m_out << std::string(2 + 4 * static_cast<size_t>(o.depth), ' ');
                if (o.active) m_out << prefix << ++n << ". ";
                else m_out << "· ";
                m_out << withPosition(o) << (o.collapsed ? " (свёрнут)" : "") << (o.takable ? " (можно взять)" : "") << std::endl;
            }
            return;
        }
        m_out << title;
        for (const auto& o : list) {
            if (!o.active) continue;
            m_out << "  " << prefix << ++n << ". " << withPosition(o) << (o.takable ? " (можно взять)" : "");
        }
        m_out << std::endl;
        for (const auto& o : list) {
            if (!o.active) m_out << "   · " << withPosition(o) << std::endl;
        }
    };
    if (!view.sceneObjects.empty() || !view.inventory.empty()) {
        m_out << std::endl;
    }
    printList("Здесь:", view.sceneObjects, 'o');
    printList("Инвентарь:", view.inventory, 'i');
    m_out << std::endl;
    for (size_t i = 0; i < view.choices.size(); ++i) {
        m_out << (i + 1) << ". " << view.choices[i].text << std::endl;
    }
    if (m_engine->hasTimer()) {
        m_out << "w. Подождать" << std::endl;
    }
    m_out << "l. Осмотреться" << std::endl;
    m_out << "m. Меню" << std::endl;
}

namespace {
/// "o3" -> ('o', 2); некорректный ввод -> ('\0', 0)
std::pair<char, size_t> parseRef(const std::string& token) {
    if (token.size() < 2 || (token[0] != 'o' && token[0] != 'i')) {
        return {'\0', 0};
    }
    try {
        const long n = std::stol(token.substr(1));
        if (n < 1) return {'\0', 0};
        return {token[0], static_cast<size_t>(n - 1)};
    } catch (...) {
        return {'\0', 0};
    }
}
}

bool NLBExplorer::handleCommand(const std::string& line, const PlayerEngine::PageView& view) {
    std::istringstream tokens(line);
    std::string first;
    if (!(tokens >> first)) {
        return true;
    }
    if (first == "0") {
        // Подтверждение — чтобы не завершить игру случайно
        return !confirm("Выйти из игры?");
    }
    if (first == "m") {
        return runMenu();
    }
    if (first == "w") {
        m_engine->wait();
        return true;
    }
    if (first == "l") {
        m_engine->look();
        return true;
    }
    // Номера oN/iN — только у активных объектов
    auto activeOnly = [](const std::vector<PlayerEngine::ObjectView>& list) {
        std::vector<std::string> ids;
        for (const auto& o : list) if (o.active) ids.push_back(o.instanceId);
        return ids;
    };
    const std::vector<std::string> sceneIds = activeOnly(view.sceneObjects);
    const std::vector<std::string> invIds = activeOnly(view.inventory);
    auto refId = [&](std::pair<char, size_t> ref) -> std::string {
        const auto& list = (ref.first == 'o') ? sceneIds : invIds;
        return (ref.first != '\0' && ref.second < list.size()) ? list[ref.second] : std::string();
    };
    bool ok = false;
    if (first[0] == 'u') {
        std::string second;
        tokens >> second;
        const std::string source = refId({'i', parseRef("i" + first.substr(1)).second});
        const std::string target = refId(parseRef(second));
        ok = !source.empty() && !target.empty() && parseRef("i" + first.substr(1)).first
             && m_engine->use(source, target);
    } else if (first[0] == 'o' || first[0] == 'i') {
        const auto ref = parseRef(first);
        const std::string id = refId(ref);
        ok = !id.empty() && (ref.first == 'o' ? m_engine->actOnObject(id) : m_engine->clickInventory(id));
    } else {
        try {
            const long n = std::stol(first);
            ok = n >= 1 && m_engine->choose(static_cast<size_t>(n - 1));
        } catch (...) {
            ok = false;
        }
    }
    if (!ok) {
        m_out << "Неверный выбор. Команды: N, oN, iN, uN oM | uN iM, w — подождать, l — осмотреться, m — меню." << std::endl;
    }
    return true;
}

// ============================================================================ меню

bool NLBExplorer::confirm(const std::string& question) {
    m_out << question << " 1 — да, любой другой ввод — нет: " << std::flush;
    std::string answer;
    if (!readLine(answer)) {
        return true;  // ввод закончился — считаем согласием (выход)
    }
    return answer == "1";
}

std::string NLBExplorer::slotPath(int slot) const {
    return m_savesDir + "/slot" + std::to_string(slot) + ".sav";
}

bool NLBExplorer::runMenu() {
    while (true) {
        m_out << (m_screenReader ? "\nМеню.\n" : "\n=== Меню ===\n");
        if (!m_savesDir.empty()) {
            // На странице «не сохранять» пункт виден, но недоступен — как серый пункт меню INSTEAD
            m_out << (m_engine->canSave() ? "1. Сохранить игру\n" : "1. Сохранить игру — здесь сохраняться нельзя\n")
                  << "2. Загрузить игру\n";
        }
        m_out << "3. Начать заново\n4. Выйти из игры\n";
        if (m_soundSwitch) m_out << (m_soundOn ? "5. Выключить звук\n" : "5. Включить звук\n");
        m_out << "0. Вернуться в игру\n\nВаш выбор: " << std::flush;
        std::string line;
        if (!readLine(line)) {
            return false;
        }
        if (line == "0" || line.empty()) {
            m_engine->look();  // как после закрытия меню в INSTEAD: снова видна текущая сцена
            return true;
        }
        if (line == "1" && !m_savesDir.empty()) {
            if (m_engine->canSave()) {
                saveMenu();
            } else {
                m_out << "Здесь сохраняться нельзя." << std::endl;
            }
        } else if (line == "2" && !m_savesDir.empty()) {
            loadMenu();
            return true;
        } else if (line == "3") {
            if (confirm("Начать игру заново? Несохранённый прогресс будет потерян.")) {
                m_engine->restart();
                return true;
            }
        } else if (line == "5" && m_soundSwitch) {
            m_soundOn = m_soundSwitch(!m_soundOn);
            m_out << (m_soundOn ? "Звук включён." : "Звук выключен.") << std::endl;
        } else if (line == "4") {
            if (confirm("Выйти из игры?")) {
                return false;
            }
        } else {
            m_out << "Неверный выбор." << std::endl;
        }
    }
}

void NLBExplorer::saveMenu() {
    m_out << "\nСохранить в слот:\n";
    for (int slot = 1; slot <= SAVE_SLOTS; ++slot) {
        const auto info = m_engine->readSaveInfo(slotPath(slot));
        m_out << slot << ". " << (info.exists ? info.time + " — " + info.label : "<пусто>")
              << (info.exists && !info.compatible ? " (другая книга)" : "") << "\n";
    }
    m_out << "0. Отмена\n\nВаш выбор: " << std::flush;
    std::string line;
    if (!readLine(line)) return;
    int slot = 0;
    try { slot = std::stoi(line); } catch (...) { slot = 0; }
    if (slot < 1 || slot > SAVE_SLOTS) return;
    if (m_engine->readSaveInfo(slotPath(slot)).exists && !confirm("Слот занят. Перезаписать?")) {
        return;
    }
    std::string error;
    if (m_engine->saveGame(slotPath(slot), error)) {
        m_out << "Игра сохранена в слот " << slot << "." << std::endl;
    } else {
        m_out << "Не удалось сохранить игру: " << error << std::endl;
    }
}

void NLBExplorer::loadMenu() {
    m_out << "\nЗагрузить из слота:\n";
    bool any = false;
    for (int slot = 1; slot <= SAVE_SLOTS; ++slot) {
        const auto info = m_engine->readSaveInfo(slotPath(slot));
        if (!info.exists) continue;
        any = true;
        m_out << slot << ". " << info.time << " — " << info.label
              << (info.compatible ? "" : " (другая книга)") << "\n";
    }
    if (!any) {
        m_out << "Сохранений нет." << std::endl;
        m_engine->look();
        return;
    }
    m_out << "0. Отмена\n\nВаш выбор: " << std::flush;
    std::string line;
    if (!readLine(line)) return;
    int slot = 0;
    try { slot = std::stoi(line); } catch (...) { slot = 0; }
    if (slot < 1 || slot > SAVE_SLOTS) {
        m_engine->look();
        return;
    }
    std::string error;
    if (m_engine->loadGame(slotPath(slot), error)) {
        m_out << "Игра загружена из слота " << slot << "." << std::endl;
    } else {
        m_out << "Не удалось загрузить игру: " << error << std::endl;
        m_engine->look();
    }
}

// ============================================================================ внешние игры

void NLBExplorer::registerExternalGame(const std::string& name, ExternalGameHandler handler) {
    m_externalGames.emplace_back(name, std::move(handler));
}

const NLBExplorer::ExternalGameHandler* NLBExplorer::findExternalGame(const std::string& name) const {
    for (const auto& [pattern, handler] : m_externalGames) {
        const bool prefix = !pattern.empty() && pattern.back() == '*';
        if (prefix ? name.compare(0, pattern.size() - 1, pattern, 0, pattern.size() - 1) == 0 : name == pattern) {
            return &handler;
        }
    }
    return nullptr;
}

void NLBExplorer::runExternalGame() {
    const auto game = m_engine->externalGame();
    m_out << (m_screenReader ? "\nВнешняя игра: " + game->name + "." : "\n=== Внешняя игра: " + game->name + " ===") << std::endl;
    if (const ExternalGameHandler* handler = findExternalGame(game->name)) {
        (*handler)(*m_engine, [this](std::string& line) { return readLine(line); }, m_out);
    }
    m_out << (m_screenReader ? "Возврат в новеллу." : "=== Возврат в новеллу ===") << std::endl;
    m_engine->finishExternalGame();
}

namespace {
/// Консольная заглушка мини-игры. Настоящая игра (кости «Большая свинья», 3D-сцена в Unreal)
/// подключается так же: обработчик получает PlayerEngine и через getVariable/setVariable,
/// giveItem/takeItem и evaluate читает и меняет состояние новеллы, а вернувшись, отдаёт ход ей
void demoDiceGame(PlayerEngine& engine, const NLBExplorer::LineReader& readLine, std::ostream& out) {
    const std::string money = "money";  // в «Большой свинье» выигрыш — переменная money (_money)
    auto show = [&] {
        out << "Деньги (" << money << "): "
            << ModificationInterpreter::toDisplayString(engine.getVariable(money)) << std::endl;
    };
    out << "Здесь могла бы быть мини-игра (кости, 3D-сцена). В консольном плеере — заглушка." << std::endl;
    while (true) {
        show();
        out << "1. Бросить кубик: +100 монет и вернуться в новеллу\n"
               "2. Переменные: показать все (v) или изменить (v имя=выражение)\n"
               "0. Закончить игру и вернуться в новеллу\n\nВаш выбор: " << std::flush;
        std::string line;
        if (!readLine(line)) return;
        if (line == "1") {
            const cparse::packToken value = engine.getVariable(money);
            const double current = (value->type & cparse::NUM) ? value.asDouble() : 0.0;
            engine.setVariable(money, static_cast<int64_t>(current + 100));
            out << "Кубик брошен — вы получили 100 монет." << std::endl;
            show();
            return;
        }
        if (line == "0" || line.empty()) return;
        if (line == "2" || line == "v") {
            for (const auto& name : engine.variableNames()) {
                out << "  " << name << " = " << ModificationInterpreter::toDisplayString(engine.getVariable(name)) << "\n";
            }
            continue;
        }
        if (line.rfind("v ", 0) == 0) {
            const auto eq = line.find('=');
            if (eq == std::string::npos) { out << "Формат: v имя=выражение" << std::endl; continue; }
            const std::string name = line.substr(2, eq - 2);
            try {
                engine.setVariable(name, engine.evaluate(line.substr(eq + 1)));
                out << name << " = " << ModificationInterpreter::toDisplayString(engine.getVariable(name)) << std::endl;
            } catch (const std::exception& e) {
                out << "Ошибка: " << e.what() << std::endl;
            }
            continue;
        }
        out << "Неверный выбор." << std::endl;
    }
}
}

void NLBExplorer::registerDefaultExternalGames() {
    // Мини-игры «Большая свинья» из INSTEAD-модуля big_pig.lua (play_bp_two_ais, play_bp_one_ai, ...)
    registerExternalGame("play_bp_*", demoDiceGame);
}

std::vector<PlayerEngine::Event> NLBExplorer::takeEvents() {
    std::vector<PlayerEngine::Event> events = m_engine->takeEvents();
    if (m_eventListener) m_eventListener(events);
    return events;
}
