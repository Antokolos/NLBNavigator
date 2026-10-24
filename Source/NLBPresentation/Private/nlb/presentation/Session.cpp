#include "nlb/presentation/Session.h"
#include <algorithm>
#include <set>
#include <sstream>
#include "nlb/domain/PageImpl.h"
#include "nlb/presentation/Localization.h"

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
    // Язык: выбор игрока (сохранён), иначе первый из предпочтений, что есть в книге, иначе английский
    {
        std::vector<std::string> preferred;
        if (!savedLanguage().empty()) preferred.push_back(savedLanguage());
        preferred.insert(preferred.end(), options.preferredLanguages.begin(), options.preferredLanguages.end());
        const std::string lang = chooseLanguage(availableLanguages(), preferred, m_book->getLanguage());
        if (!lang.empty() && lang != m_book->getLanguage()) applyLanguage(lang);
    }
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
    const std::string pageBefore = m_screen.pageId;
    m_screen = m_engine->screen();
    // Озвучка страницы (как nlb:push/nextsnd/sndout NLB, но по порядку поля звука): новые файлы
    // озвучки — новая очередь; уход со страницы — очередь прочь, звучащая фраза — стоп
    std::vector<std::string> voices;
    for (const auto& e : events) if (e.kind == PlayerEngine::Event::Kind::Voice) voices.push_back(e.text);
    if (!voices.empty() || pageBefore != m_screen.pageId) {
        if (m_voicePlayed && m_director) m_director->stopVoice();
        m_voicePlayed = false;
        m_voices.assign(voices.begin(), voices.end());
    }
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
    // при выбранном предмете щелчок по menu INSTEAD игнорирует (game.c: ссылка menu — act,
    // в режиме use она не срабатывает), выбор остаётся
    const bool ignored = !m_selected.empty() && m_engine->isMenuItem(objectId);
    if (m_log) {
        const std::string obj = labelOf(m_screen.sceneObjects, objectId);
        logAction(m_selected.empty() ? obj : labelOf(m_screen.inventory, m_selected) + " → " + obj
                                                 + (ignored ? " (не применяется)" : ""));
    }
    if (ignored) return false;
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
    // menu (кнопки боя «Фронтира» и т. п.) срабатывает с первого щелчка, как в INSTEAD
    // (iface.xref: у menu — команда act); при выбранном предмете щелчок по нему INSTEAD
    // игнорирует (game.c: в режиме use ссылка act не срабатывает), выбор остаётся
    const bool menu = m_engine->isMenuItem(itemId);
    if (menu && !m_selected.empty()) {
        if (m_log) logAction(labelOf(m_screen.inventory, m_selected) + " → " + labelOf(m_screen.inventory, itemId) + " (не применяется)");
        return false;
    }
    if (m_log) {
        const std::string item = labelOf(m_screen.inventory, itemId);
        if (menu) logAction(item);
        else if (m_selected == itemId) logAction(item + " (осмотреть)");
        else if (!m_selected.empty()) logAction(labelOf(m_screen.inventory, m_selected) + " → " + item);
        else logAction(item + " (выбран — дальше щёлкните, к чему применить)");
    }
    bool ok = true;
    if (menu || m_selected == itemId) {   // menu — действие; второй щелчок по тому же — осмотр (inv)
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

void Session::tryAgain() {
    if (!m_engine->hasSnapshot()) {
        restart();
        return;
    }
    if (m_platforms) m_platforms->reset();
    logAction("Попробовать ещё раз");
    m_logPage.clear();
    m_logRevision = static_cast<uint64_t>(-1);
    m_selected.clear();
    m_engine->tryAgain();
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

namespace {
void collectLanguages(const NonLinearBook* book, std::set<std::string>& out) {
    if (!book) return;
    if (!book->getLanguage().empty()) out.insert(book->getLanguage());
    for (const auto& [id, page] : book->getPages()) {
        for (const auto& lang : page->getTexts().keySet()) if (!page->getTexts().get(lang).empty()) out.insert(lang);
        if (page->getModule()) collectLanguages(page->getModule(), out);
    }
}

void setBookLanguage(NonLinearBookImpl* book, const std::string& lang) {
    if (!book) return;
    book->setLanguage(lang);
    // страницы книги — PageImpl, их модули — NonLinearBookImpl (у модуля свой язык текстов)
    for (const auto& [id, page] : book->getPages()) setBookLanguage(static_cast<PageImpl*>(page)->getModuleImpl(), lang);
}
}

std::vector<std::string> Session::availableLanguages() const {
    std::set<std::string> langs;
    collectLanguages(m_book.get(), langs);
    return {langs.begin(), langs.end()};
}

void Session::applyLanguage(const std::string& language) {
    setBookLanguage(m_book.get(), language);
}

std::string Session::savedLanguage() const {
    return setting("language");
}

std::string Session::setting(const std::string& key) const {
    if (m_options.savesDir.empty()) return "";
    std::string text;
    if (!FileUtils::readFile(FileUtils::combinePath(m_options.savesDir, "settings.ini"), text)) return "";
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.compare(0, key.size() + 1, key + "=") == 0) return line.substr(key.size() + 1);
    }
    return "";
}

void Session::setSetting(const std::string& key, const std::string& value) {
    if (m_options.savesDir.empty()) return;
    const std::string path = FileUtils::combinePath(m_options.savesDir, "settings.ini");
    std::string text, out;
    FileUtils::readFile(path, text);
    std::istringstream in(text);
    std::string line;
    bool done = false;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        if (line.compare(0, key.size() + 1, key + "=") == 0) {
            if (done) continue;
            line = key + "=" + value;
            done = true;
        }
        out += line + "\n";
    }
    if (!done) out += key + "=" + value + "\n";
    FileUtils::createDirectoryRecursive(m_options.savesDir);
    std::ofstream(FileUtils::nativePath(path), std::ios::trunc) << out;
}

bool Session::setLanguage(const std::string& language, bool byPlayer) {
    if (!m_engine) return false;
    const auto langs = availableLanguages();
    if (std::find(langs.begin(), langs.end(), language) == langs.end()) return false;
    if (byPlayer) setSetting("language", language);
    if (language == m_book->getLanguage()) return true;
    applyLanguage(language);
    m_engine->look();  // сцена заново — на новом языке
    afterCommand();
    return true;
}

void Session::setPlatforms(std::unique_ptr<nlb::platform::PlatformBridge> bridge) {
    m_platforms = std::move(bridge);
    // язык, выбранный в лаунчере (Steam, Epic), — если игрок не выбирал язык в меню
    if (m_platforms && m_engine && savedLanguage().empty()) {
        const std::string lang = m_platforms->language();
        if (!lang.empty()) {
            const std::string chosen = chooseLanguage(availableLanguages(), {lang}, m_book->getLanguage());
            if (chosen != m_book->getLanguage()) setLanguage(chosen, false);
        }
    }
}

bool Session::nextVoice() {
    if (m_voices.empty()) return false;
    const std::string file = m_voices.front();
    m_voices.pop_front();
    if (m_director) m_director->playVoice(file);
    m_voicePlayed = true;
    return true;
}

void Session::playUiSound(const std::string& path) {
    if (path.empty() || !m_audio || (m_director && m_director->muted())) return;
    m_audio->playEffect(MediaRef{path});
}

}  // namespace nlb::presentation
