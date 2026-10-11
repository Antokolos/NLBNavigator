#include "nlb/domain/NLBExplorer.h"

#include <algorithm>
#include <set>
#include <sstream>

// Режим для экранного диктора. Принципы:
//  - не выводить то, что диктор зачитает как шум: картинки, звуки, координаты, имена файлов,
//    рамки и значки; тексты книги — без изменений;
//  - каждый пункт списка — отдельная строка, перед списком — заголовок словами;
//  - при переходе на страницу — текст и все списки; после действия на той же странице —
//    только реакция и то, что изменилось; полные списки — по командам;
//  - номера объектов и предметов не меняются, пока игрок на странице;
//  - команды — русские буквы с латинскими синонимами (на случай английской раскладки).

namespace {
/// Первая буква команды (русская или латинская) и остаток строки
std::pair<std::string, std::string> splitCommand(const std::string& line) {
    size_t i = 0;
    while (i < line.size() && line[i] == ' ') ++i;
    if (i >= line.size()) return {"", ""};
    const unsigned char c = static_cast<unsigned char>(line[i]);
    const size_t len = c < 0x80 ? 1 : c < 0xE0 ? 2 : c < 0xF0 ? 3 : 4;
    std::string head = line.substr(i, len);
    if (len == 1) head[0] = static_cast<char>(std::tolower(c));
    std::string rest = line.substr(std::min(line.size(), i + len));
    const size_t b = rest.find_first_not_of(' ');
    rest = b == std::string::npos ? "" : rest.substr(b);
    return {head, rest};
}

bool isNumber(const std::string& s) {
    return !s.empty() && std::all_of(s.begin(), s.end(), [](char c) { return c >= '0' && c <= '9'; });
}

/// Метки взаимодействия [метка] в описаниях объектов — без скобок, чтобы диктор их не читал
std::string withoutBrackets(std::string s) {
    s.erase(std::remove(s.begin(), s.end(), '['), s.end());
    s.erase(std::remove(s.begin(), s.end(), ']'), s.end());
    return s;
}

std::string oneLine(const std::string& s) {
    std::string r;
    for (char c : s) r += (c == '\n') ? ' ' : c;
    return r;
}

/// Точка в конце фразы, если её там ещё нет
std::string sentence(const std::string& s) {
    if (s.empty()) return s;
    const char last = s.back();
    return (last == '.' || last == '!' || last == '?' || last == ':' || last == ';') ? s : s + ".";
}

/// Подпись объекта для диктора. Если у графической кнопки нет подписи и выводится её
/// внутреннее имя (skipTutorialOutObj), оно разбивается на слова без служебных хвостов:
/// «skip tutorial» — понятнее, чем имя по буквам
std::string spoken(const std::string& label) {
    const bool identifier = !label.empty() && label.find(' ') == std::string::npos
        && std::all_of(label.begin(), label.end(), [](char c) {
               return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
           });
    if (!identifier) return label;
    std::vector<std::string> words;
    std::string word;
    for (char c : label) {
        if (c == '_' || (c >= 'A' && c <= 'Z' && !word.empty())) {
            if (!word.empty()) words.push_back(word);
            word.clear();
            if (c == '_') continue;
        }
        word += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    if (!word.empty()) words.push_back(word);
    while (words.size() > 1 && (words.back() == "obj" || words.back() == "out" || words.back() == "over"
                                || words.back() == "hl")) {
        words.pop_back();
    }
    std::string result;
    for (const auto& w : words) result += (result.empty() ? "" : " ") + w;
    return result.empty() ? label : result;
}

/// Справочная подпись, которую не стоит зачитывать: внутреннее имя объекта (bgObj2)
/// или «линейка» из дефисов
bool isNoise(const std::string& label) {
    if (label.empty()) return true;
    if (std::all_of(label.begin(), label.end(), [](char c) { return c == '-' || c == '_' || c == '=' || c == ' ' || c == '*'; })) {
        return true;
    }
    return label.find(' ') == std::string::npos
        && std::all_of(label.begin(), label.end(), [](char c) {
               return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
           });
}

/// Синонимы: русская буква или латинская
bool is(const std::string& head, const char* ru, const char* lat) {
    return head == ru || head == lat;
}
}

void NLBExplorer::srPrintHelp() {
    m_out << "Команды:\n"
             "число — выбрать вариант с этим номером.\n"
             "о и номер — действие с объектом, например о2. Латинская o тоже подходит.\n"
             "и — что в инвентаре. и и номер — действие с предметом, например и1.\n"
             "п — применить предмет к объекту: программа спросит, что и к чему.\n"
             "т — повторить текст страницы.\n"
             "в — варианты. з — что здесь.\n"
             "ж — подождать.\n"
             "м — меню: сохранить, загрузить, начать заново, выйти.\n"
             "вопросительный знак — эта справка." << std::endl;
}

bool NLBExplorer::srPrintEvents() {
    using Kind = PlayerEngine::Event::Kind;
    bool fullPage = false;
    for (const auto& event : m_engine->takeEvents()) {
        switch (event.kind) {
            case Kind::PageCaption: m_out << "\n" << sentence(oneLine(event.text)) << std::endl; fullPage = true; break;
            case Kind::PageText:    m_out << "\n" << event.text << std::endl; fullPage = true; break;
            case Kind::ObjectText:
            case Kind::AltText:
            case Kind::Text:        m_out << withoutBrackets(event.text) << std::endl; break;
            case Kind::Achievement: m_out << sentence(event.text) << std::endl; break;
            case Kind::Info:
                // Служебные сообщения оформления окна (WINGEOM и т.п.) диктору не нужны
                if (event.text.rfind("[URL: ", 0) == 0) {
                    m_out << "Ссылка: " << event.text.substr(6, event.text.size() > 7 ? event.text.size() - 7 : 0) << std::endl;
                } else if (!event.text.empty() && event.text[0] != '[') {
                    m_out << event.text << std::endl;
                }
                break;
            case Kind::Finish:      m_out << "\nКонец игры." << std::endl; break;
            case Kind::Image: case Kind::Animation: case Kind::Sound: case Kind::Music:
            case Kind::ExternalGame:
                break;
        }
    }
    return fullPage;
}

void NLBExplorer::srAssignNumbers(const PlayerEngine::PageView& view) {
    const std::string pageId = view.pageId;
    if (pageId != m_srPageId) {
        m_srPageId = pageId;
        m_srSceneNumbers.clear();
        m_srInventoryNumbers.clear();
        m_srNextScene = m_srNextInventory = 1;
    }
    for (const auto& o : view.sceneObjects) {
        if (o.active && !m_srSceneNumbers.count(o.instanceId)) m_srSceneNumbers[o.instanceId] = m_srNextScene++;
    }
    for (const auto& o : view.inventory) {
        if (o.active && !m_srInventoryNumbers.count(o.instanceId)) m_srInventoryNumbers[o.instanceId] = m_srNextInventory++;
    }
}

void NLBExplorer::srPrintChoices(const PlayerEngine::PageView& view) {
    if (view.choices.empty()) {
        m_out << "Вариантов перехода нет." << std::endl;
        return;
    }
    m_out << (view.choices.size() == 1 ? "Вариант:" : "Варианты:") << std::endl;
    for (size_t i = 0; i < view.choices.size(); ++i) {
        m_out << (i + 1) << ". " << oneLine(view.choices[i].text) << std::endl;
    }
}

void NLBExplorer::srPrintScene(const PlayerEngine::PageView& view, bool withReadOnly) {
    bool any = false;
    for (const auto& o : view.sceneObjects) {
        if (!o.active) continue;
        if (!any) { m_out << "Здесь можно действовать:" << std::endl; any = true; }
        m_out << "о" << m_srSceneNumbers[o.instanceId] << ". " << spoken(oneLine(o.disp))
              << (o.takable ? ", можно взять" : "") << std::endl;
    }
    if (withReadOnly) {
        bool header = false;
        for (const auto& o : view.sceneObjects) {
            if (o.active) continue;
            // Свёрнутый свиток — одной фразой, без внутренних имён
            if (o.collapsed || isNoise(oneLine(o.disp))) continue;
            if (!header) { m_out << "Для сведения:" << std::endl; header = true; }
            m_out << oneLine(o.disp) << std::endl;
        }
    }
    if (!any && withReadOnly && std::none_of(view.sceneObjects.begin(), view.sceneObjects.end(),
                                              [](const PlayerEngine::ObjectView& o) { return !o.active && !o.collapsed && !isNoise(oneLine(o.disp)); })) {
        m_out << "Здесь ничего нет." << std::endl;
    }
}

void NLBExplorer::srPrintInventory(const PlayerEngine::PageView& view) {
    if (view.inventory.empty()) {
        m_out << "Инвентарь пуст." << std::endl;
        return;
    }
    m_out << "В инвентаре:" << std::endl;
    for (const auto& o : view.inventory) {
        if (o.active) m_out << "и" << m_srInventoryNumbers[o.instanceId] << ". " << spoken(oneLine(o.disp)) << std::endl;
        else if (!isNoise(oneLine(o.disp))) m_out << oneLine(o.disp) << std::endl;
    }
}

void NLBExplorer::srPrintChanges(const PlayerEngine::PageView& before, const PlayerEngine::PageView& after) {
    auto labels = [](const std::vector<PlayerEngine::ObjectView>& list) {
        std::map<std::string, std::string> m;
        for (const auto& o : list) if (!o.collapsed) m[o.instanceId] = oneLine(o.disp);
        return m;
    };
    const auto sceneBefore = labels(before.sceneObjects), sceneAfter = labels(after.sceneObjects);
    const auto invBefore = labels(before.inventory), invAfter = labels(after.inventory);
    // Справочные (неактивные) объекты: о появлении — «Для сведения», об исчезновении молчим
    std::set<std::string> readOnly;
    for (const auto& o : before.sceneObjects) if (!o.active) readOnly.insert(o.instanceId);
    for (const auto& o : before.inventory) if (!o.active) readOnly.insert(o.instanceId);
    // Объект с той же подписью исчез в одном месте и появился в другом (навык перенесён на
    // свиток) — одной фразой «Перемещено»
    std::set<std::string> vanished;
    for (const auto& [id, label] : sceneBefore) {
        if (!sceneAfter.count(id) && !invAfter.count(id) && !readOnly.count(id)) vanished.insert(label);
    }
    std::set<std::string> moved;
    for (const auto& o : after.sceneObjects) {
        if (o.collapsed) continue;
        const std::string label = spoken(oneLine(o.disp));
        auto was = sceneBefore.find(o.instanceId);
        if (was == sceneBefore.end()) {
            if (o.active && vanished.count(oneLine(o.disp))) {
                m_out << "Перемещено: " << label << ", теперь о" << m_srSceneNumbers[o.instanceId] << "." << std::endl;
                moved.insert(oneLine(o.disp));
            } else if (o.active) {
                m_out << "Появилось здесь: о" << m_srSceneNumbers[o.instanceId] << ", " << sentence(label) << std::endl;
            } else if (!isNoise(oneLine(o.disp))) {
                m_out << "Для сведения: " << sentence(label) << std::endl;
            }
        } else if (was->second != oneLine(o.disp) && (o.active || !isNoise(oneLine(o.disp)))) {
            m_out << "Изменилось: " << sentence(label) << std::endl;
        }
    }
    for (const auto& [id, label] : sceneBefore) {
        if (!sceneAfter.count(id) && !invAfter.count(id) && !readOnly.count(id) && !moved.count(label)) {
            m_out << "Пропало отсюда: " << sentence(spoken(label)) << std::endl;
        }
    }
    srPrintInventoryChanges(before, after);
    std::vector<std::string> choicesBefore, choicesAfter;
    for (const auto& c : before.choices) choicesBefore.push_back(c.text);
    for (const auto& c : after.choices) choicesAfter.push_back(c.text);
    if (choicesBefore != choicesAfter) {
        m_out << "Варианты изменились." << std::endl;
        srPrintChoices(after);
    }
}

void NLBExplorer::srPrintInventoryChanges(const PlayerEngine::PageView& before, const PlayerEngine::PageView& after) {
    auto labels = [](const std::vector<PlayerEngine::ObjectView>& list) {
        std::map<std::string, std::string> m;
        for (const auto& o : list) if (!o.collapsed) m[o.instanceId] = oneLine(o.disp);
        return m;
    };
    const auto invBefore = labels(before.inventory), invAfter = labels(after.inventory);
    std::set<std::string> readOnly;
    for (const auto& o : before.inventory) if (!o.active) readOnly.insert(o.instanceId);
    for (const auto& o : after.inventory) {
        const std::string label = spoken(oneLine(o.disp));
        auto was = invBefore.find(o.instanceId);
        if (was == invBefore.end()) {
            if (o.active) m_out << "В инвентаре теперь: и" << m_srInventoryNumbers[o.instanceId] << ", " << sentence(label) << std::endl;
            else if (!isNoise(oneLine(o.disp))) m_out << "Для сведения: " << sentence(label) << std::endl;
        } else if (was->second != oneLine(o.disp) && (o.active || !isNoise(oneLine(o.disp)))) {
            m_out << (o.active ? "В инвентаре изменилось: " : "Теперь: ") << sentence(label) << std::endl;
        }
    }
    for (const auto& [id, label] : invBefore) {
        if (!invAfter.count(id) && !readOnly.count(id)) m_out << "Из инвентаря пропало: " << sentence(spoken(label)) << std::endl;
    }
}

std::string NLBExplorer::srObjectId(const PlayerEngine::PageView& view, bool inventory, int number) const {
    const auto& numbers = inventory ? m_srInventoryNumbers : m_srSceneNumbers;
    const auto& list = inventory ? view.inventory : view.sceneObjects;
    for (const auto& o : list) {
        auto it = numbers.find(o.instanceId);
        if (o.active && it != numbers.end() && it->second == number) return o.instanceId;
    }
    return "";
}

bool NLBExplorer::srApply(const PlayerEngine::PageView& view, const std::string& args) {
    // «п» — пошагово; «п 1 о3» — сразу
    std::istringstream words(args);
    std::string what, target;
    words >> what >> target;
    if (what.empty()) {
        srPrintInventory(view);
        m_out << "Что применить? Номер предмета: " << std::flush;
        if (!readLine(what)) return false;
    }
    auto [whatHead, whatRest] = splitCommand(what);
    const std::string whatNumber = isNumber(what) ? what : whatRest;
    if (!isNumber(whatNumber)) { m_out << "Нужен номер предмета из инвентаря." << std::endl; return true; }
    const std::string source = srObjectId(view, true, std::stoi(whatNumber));
    if (source.empty()) { m_out << "Такого предмета нет." << std::endl; return true; }
    if (target.empty()) {
        srPrintScene(view, false);
        m_out << "К чему применить? о и номер объекта или и и номер предмета: " << std::flush;
        if (!readLine(target)) return false;
    }
    auto [head, rest] = splitCommand(target);
    const bool toInventory = is(head, "и", "i");
    const std::string number = isNumber(target) ? target : rest;
    if (!isNumber(number) || (!toInventory && !isNumber(target) && !is(head, "о", "o"))) {
        m_out << "Нужно о и номер или и и номер." << std::endl;
        return true;
    }
    const std::string targetId = srObjectId(view, toInventory, std::stoi(number));
    if (targetId.empty() || !m_engine->use(source, targetId)) m_out << "Так применить нельзя." << std::endl;
    return true;
}

bool NLBExplorer::srHandleCommand(const std::string& line, const PlayerEngine::PageView& view) {
    if (line.empty()) return true;
    if (line == "0") return !confirm("Выйти из игры?");
    if (isNumber(line)) {
        const long n = std::stol(line);
        if (n < 1 || !m_engine->choose(static_cast<size_t>(n - 1))) m_out << "Нет варианта с таким номером." << std::endl;
        return true;
    }
    if (line == "?" || line == "помощь" || line == "help") { srPrintHelp(); return true; }
    auto [head, rest] = splitCommand(line);
    if (is(head, "о", "o")) {
        const std::string id = isNumber(rest) ? srObjectId(view, false, std::stoi(rest)) : "";
        if (id.empty()) { srPrintScene(view, false); m_out << "Укажите номер, например о1." << std::endl; return true; }
        m_engine->actOnObject(id);
        return true;
    }
    if (is(head, "и", "i")) {
        if (rest.empty()) { srPrintInventory(view); return true; }
        const std::string id = isNumber(rest) ? srObjectId(view, true, std::stoi(rest)) : "";
        if (id.empty()) { m_out << "Такого предмета нет." << std::endl; return true; }
        m_engine->clickInventory(id);
        return true;
    }
    if (is(head, "п", "u") || head == "p") return srApply(view, rest);
    if (is(head, "т", "t") || head == "l") { m_engine->look(); return true; }
    if (is(head, "в", "v")) { srPrintChoices(view); return true; }
    if (is(head, "з", "z")) { srPrintScene(view, true); return true; }
    if (is(head, "ж", "w")) { m_engine->wait(); return true; }
    if (is(head, "м", "m")) return runMenu();
    m_out << "Непонятная команда. Вопросительный знак — справка." << std::endl;
    return true;
}

void NLBExplorer::exploreScreenReader() {
    try {
        m_out << "Режим для экранного диктора. Вопросительный знак — список команд." << std::endl;
        m_engine->start(m_startPageId);
        PlayerEngine::PageView previous;
        bool havePrevious = false;
        while (true) {
            const bool fullPage = srPrintEvents();
            if (m_engine->externalGame()) {
                runExternalGame();
                havePrevious = false;
                continue;
            }
            const PlayerEngine::PageView view = m_engine->view();
            const bool newPage = !havePrevious || view.pageId != m_srPageId;
            srAssignNumbers(view);
            if (view.finished) {
                if (!m_engine->isVnPage(m_engine->currentPage())) break;
                m_out << "1. Начать заново. 0. Выход.\nВаш выбор: " << std::flush;
                std::string line;
                if (!readLine(line) || line != "1") break;
                m_engine->restart();
                havePrevious = false;
                continue;
            }
            if (newPage || fullPage) {
                // Показатели в инвентаре (жизнь, энергия) могли измениться и при переходе
                if (havePrevious) {
                    srPrintInventoryChanges(previous, view);
                }
                // Новая страница или «осмотреться»: варианты и что здесь
                if (std::any_of(view.sceneObjects.begin(), view.sceneObjects.end(),
                                [](const PlayerEngine::ObjectView& o) { return !o.collapsed && (o.active || !isNoise(oneLine(o.disp))); })) {
                    srPrintScene(view, true);
                }
                // Переходов нет — значит, ход делается объектами или предметами: показываем и
                // инвентарь (в бою Frontier там кнопки «Концентрация», «Блок», «Удар» и здоровье)
                const bool sceneActions = std::any_of(view.sceneObjects.begin(), view.sceneObjects.end(),
                                                      [](const PlayerEngine::ObjectView& o) { return o.active; });
                if (!view.choices.empty()) {
                    srPrintChoices(view);
                } else if (!view.inventory.empty()) {
                    srPrintInventory(view);
                } else if (!sceneActions) {
                    srPrintChoices(view);
                }
            } else {
                srPrintChanges(previous, view);
            }
            previous = view;
            havePrevious = true;
            m_out << "Ваш выбор: " << std::flush;
            std::string line;
            if (!readLine(line)) break;
            if (!srHandleCommand(line, view)) break;
        }
    } catch (const std::exception& e) {
        m_out << "\nОшибка исполнения книги: " << e.what() << std::endl;
    }
}
