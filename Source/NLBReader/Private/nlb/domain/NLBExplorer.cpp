#include "nlb/domain/NLBExplorer.h"

#include "nlb/api/NonLinearBook.h"

#include <sstream>

NLBExplorer::NLBExplorer(NonLinearBook* book, const std::string& startPageId,
                         const PlayerEngine::Settings& settings, std::istream& in, std::ostream& out)
    : m_book(book), m_startPageId(startPageId), m_in(in), m_out(out) {
    m_engine = std::make_unique<PlayerEngine>(book, settings);
}

void NLBExplorer::explore() {
    try {
        m_engine->start(m_startPageId);
        while (true) {
            printEvents();
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
    for (const auto& event : m_engine->takeEvents()) {
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
        m_out << title;
        size_t n = 0;
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
    m_out << "0. Выход" << std::endl;
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
        m_out << "Выйти из игры? 1 — да, любой другой ввод — продолжить: " << std::flush;
        std::string answer;
        if (!readLine(answer)) {
            return false;
        }
        return answer != "1";
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
        m_out << "Неверный выбор. Команды: N, oN, iN, uN oM | uN iM, w — подождать, l — осмотреться, 0 — выход." << std::endl;
    }
    return true;
}
