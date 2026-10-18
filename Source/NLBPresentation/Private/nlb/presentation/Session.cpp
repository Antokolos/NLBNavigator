#include "nlb/presentation/Session.h"

#include "nlb/api/DummyProgressData.h"
#include "nlb/domain/NonLinearBookImpl.h"
#include "nlb/presentation/AudioDirector.h"
#include "nlb/presentation/BookMediaSource.h"
#include "nlb/util/BookPack.h"
#include "nlb/util/FileUtils.h"

namespace nlb::presentation {

Session::Session(IAudioBackend* audio) : m_audio(audio) {}

Session::~Session() = default;

bool Session::open(const Options& options, std::string& error) {
    m_options = options;
    // .nlbpack монтируется как каталог книги
    if (FileUtils::exists(options.bookPath) && !FileUtils::isDirectory(options.bookPath)) {
        auto pack = BookPack::read(options.bookPath, error);
        if (!pack) return false;
        BookPack::mount(std::shared_ptr<BookPack>(std::move(pack)), options.bookPath);
    }
    m_book = std::make_unique<NonLinearBookImpl>();
    DummyProgressData progress;
    if (!m_book->load(options.bookPath, progress)) {
        error = "не удалось загрузить книгу " + options.bookPath;
        m_book.reset();
        return false;
    }
    m_media = std::make_unique<BookMediaSource>(options.bookPath);
    if (m_audio && !m_book->isSuppressSound()) m_director = std::make_unique<AudioDirector>(*m_audio);
    PlayerEngine::Settings settings;
    settings.exportMode = options.exportMode;
    settings.realTime = true;
    settings.achievementsPath = options.achievementsPath;
    m_engine = std::make_unique<PlayerEngine>(m_book.get(), settings);
    const std::vector<std::string> games = options.externalGames;
    m_engine->setExternalGameFilter([games](const std::string& name) {
        for (const auto& g : games) {
            const bool prefix = !g.empty() && g.back() == '*';
            if (prefix ? name.compare(0, g.size() - 1, g, 0, g.size() - 1) == 0 : name == g) return true;
        }
        return false;
    });
    m_engine->start();
    afterCommand();
    return true;
}

const IMediaSource& Session::media() const { return *m_media; }
std::string Session::title() const { return m_book->getTitle(); }
std::string Session::language() const { return m_book->getLanguage(); }

void Session::afterCommand() {
    std::vector<PlayerEngine::Event> events = m_engine->takeEvents();
    if (m_director) m_director->apply(events);
    m_pending.insert(m_pending.end(), events.begin(), events.end());
    m_screen = m_engine->screen();
    if (m_platforms) m_platforms->onCommand(events, *m_engine, m_screen.finished);
    logScreen();
}

std::string Session::platformConfigPath() const {
    const std::string path = FileUtils::combinePath(m_options.bookPath, "platform.ini");
    return FileUtils::exists(path) ? path : std::string();
}

// ============================================================================ журнал прохождения

namespace {
std::string paragraphText(const Paragraph& p) {
    std::string out;
    for (const auto& r : p.runs) out += r.kind == TextRun::Kind::ObjectLink ? "[" + r.text + "]" : r.text;
    return out;
}

std::string trimmed(const std::string& s) {
    const auto a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    return s.substr(a, s.find_last_not_of(" \t\r\n") - a + 1);
}

std::string labelOf(const std::vector<SceneObject>& objects, const std::string& id) {
    for (const auto& o : objects) if (o.id == id) return trimmed(o.label);
    return id;
}

std::string labelOf(const std::vector<InventoryItem>& items, const std::string& id) {
    for (const auto& i : items) if (i.id == id) return trimmed(i.label);
    return id;
}
}

bool Session::setLogFile(const std::string& path, std::string& error) {
    m_log.reset();
    m_logPage.clear();
    if (path.empty()) return true;
    auto f = std::make_unique<std::ofstream>(FileUtils::nativePath(path), std::ios::binary | std::ios::trunc);
    if (!*f) {
        error = "не удалось открыть журнал " + path;
        return false;
    }
    m_log = std::move(f);
    m_logRevision = static_cast<uint64_t>(-1);
    if (m_engine) {
        *m_log << "Книга: " << title() << "\n";
        logScreen();
    }
    return true;
}

void Session::logAction(const std::string& action) {
    if (!m_log) return;
    *m_log << "\nВаш выбор: " << action << "\n";
    m_log->flush();
}

void Session::logScreen() {
    if (!m_log) return;
    const ScreenModel& m = m_screen;
    // экран пишется, только если он изменился (refresh без команды и тики таймера — не повод)
    if (m.revision == m_logRevision && m.pageId == m_logPage) return;
    m_logRevision = m.revision;
    std::ostream& out = *m_log;
    const bool newPage = m.pageId != m_logPage;
    if (newPage) {
        m_logPage = m.pageId;
        const std::string title = !m.caption.empty() ? m.caption : m.roomTitle;
        out << "\n==================================================\n"
            << (title.empty() ? "(" + m.pageId + ")" : title)
            << "\n==================================================\n";
    }
    out << "\n";
    // ответ на действие, затем — на новой странице или если ответа нет — её текст
    for (const auto& p : m.reactionText) out << paragraphText(p) << "\n";
    if (newPage || m.reactionText.empty()) {
        if (!m.reactionText.empty()) out << "\n";
        for (const auto& p : m.pageText) out << paragraphText(p) << "\n";
    }
    if (!m.sceneObjects.empty()) {
        out << "\nЗдесь:";
        for (const auto& o : m.sceneObjects) if (!trimmed(o.label).empty()) out << "  " << trimmed(o.label);
        out << "\n";
    }
    if (!m.inventory.empty()) {
        out << (m.sceneObjects.empty() ? "\n" : "") << "Инвентарь:";
        for (const auto& i : m.inventory) if (!trimmed(i.label).empty()) out << "  " << trimmed(i.label);
        out << "\n";
    }
    if (!m.choices.empty()) {
        out << "\n";
        int n = 0;
        for (const auto& c : m.choices) {
            ++n;
            if (c.enabled) out << n << ". " << c.text << "\n";
            else if (!c.altText.empty()) out << "   (" << c.altText << ")\n";
        }
    }
    if (m.finished) out << "\n[Конец игры]\n";
    if (m.externalGame) out << "\n[Внешняя игра: " << *m.externalGame << "]\n";
    out.flush();
}

void Session::advance(int milliseconds) {
    if (m_engine && m_engine->advanceTime(milliseconds) > 0) afterCommand();
    if (m_platforms) m_platforms->tick();  // ответы площадок (вход игрока, поиск таблиц рекордов)
}

bool Session::choose(int choiceIndex) {
    if (m_log) {
        int n = 0;
        for (const auto& c : m_screen.choices) {
            ++n;
            if (c.index == choiceIndex) { logAction(std::to_string(n) + ". " + c.text); break; }
        }
    }
    m_selected.clear();
    const bool ok = choiceIndex >= 0 && m_engine->choose(static_cast<size_t>(choiceIndex));
    afterCommand();
    return ok;
}

bool Session::clickObject(const std::string& objectId) {
    if (m_log) {
        const std::string obj = labelOf(m_screen.sceneObjects, objectId);
        logAction(m_selected.empty() ? obj : labelOf(m_screen.inventory, m_selected) + " → " + obj);
    }
    bool ok;
    if (!m_selected.empty()) {
        ok = m_engine->use(m_selected, objectId);
        m_selected.clear();
    } else {
        ok = m_engine->actOnObject(objectId);
    }
    afterCommand();
    return ok;
}

bool Session::clickItem(const std::string& itemId) {
    if (m_log) {
        const std::string item = labelOf(m_screen.inventory, itemId);
        if (m_selected == itemId) logAction(item + " (осмотреть)");
        else if (!m_selected.empty()) logAction(labelOf(m_screen.inventory, m_selected) + " → " + item);
        else logAction(item + " (выбран — дальше щёлкните, к чему применить)");
    }
    bool ok = true;
    if (m_selected == itemId) {           // второй щелчок по тому же — осмотр (inv)
        m_selected.clear();
        ok = m_engine->clickInventory(itemId);
    } else if (!m_selected.empty()) {     // предмет к предмету
        ok = m_engine->use(m_selected, itemId);
        m_selected.clear();
    } else {
        m_selected = itemId;
        ++m_localRevision;
        return true;
    }
    afterCommand();
    return ok;
}

void Session::clearSelection() {
    if (!m_selected.empty()) { m_selected.clear(); ++m_localRevision; }
}

std::vector<PlayerEngine::Event> Session::takeEvents() {
    std::vector<PlayerEngine::Event> out;
    out.swap(m_pending);
    return out;
}

void Session::look() {
    logAction("Осмотреться");
    m_engine->look();
    afterCommand();
}

bool Session::finishExternalGame() {
    logAction("[внешняя игра завершена]");
    const bool ok = m_engine->finishExternalGame();
    afterCommand();
    return ok;
}

void Session::restart() {
    if (m_platforms) m_platforms->reset();
    logAction("Начать заново");
    m_logPage.clear();
    m_logRevision = static_cast<uint64_t>(-1);
    m_selected.clear();
    m_engine->restart();
    afterCommand();
}

std::string Session::slotPath(int slot) const {
    return m_options.savesDir + "/slot" + std::to_string(slot) + ".sav";
}

bool Session::save(int slot, std::string& error) {
    if (m_options.savesDir.empty()) { error = "сохранения недоступны"; return false; }
    const bool ok = m_engine->saveGame(slotPath(slot), error);
    if (ok) logAction("Сохранить в слот " + std::to_string(slot));
    return ok;
}

bool Session::load(int slot, std::string& error) {
    if (m_options.savesDir.empty()) { error = "сохранения недоступны"; return false; }
    logAction("Загрузить слот " + std::to_string(slot));
    m_logPage.clear();
    m_logRevision = static_cast<uint64_t>(-1);
    const bool ok = m_engine->loadGame(slotPath(slot), error);
    m_selected.clear();
    afterCommand();
    return ok;
}

PlayerEngine::SaveInfo Session::saveInfo(int slot) const {
    return m_options.savesDir.empty() ? PlayerEngine::SaveInfo{} : m_engine->readSaveInfo(slotPath(slot));
}

void Session::setMuted(bool muted) {
    if (m_director) m_director->setMuted(muted);
}

void Session::playUiSound(const std::string& path) {
    if (path.empty() || !m_audio || (m_director && m_director->muted())) return;
    m_audio->playEffect(MediaRef{path});
}

}  // namespace nlb::presentation
