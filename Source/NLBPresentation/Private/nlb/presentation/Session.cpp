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
}

void Session::advance(int milliseconds) {
    if (m_engine && m_engine->advanceTime(milliseconds) > 0) afterCommand();
}

bool Session::choose(int choiceIndex) {
    m_selected.clear();
    const bool ok = choiceIndex >= 0 && m_engine->choose(static_cast<size_t>(choiceIndex));
    afterCommand();
    return ok;
}

bool Session::clickObject(const std::string& objectId) {
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

bool Session::finishExternalGame() {
    const bool ok = m_engine->finishExternalGame();
    afterCommand();
    return ok;
}

void Session::restart() {
    m_selected.clear();
    m_engine->restart();
    afterCommand();
}

std::string Session::slotPath(int slot) const {
    return m_options.savesDir + "/slot" + std::to_string(slot) + ".sav";
}

bool Session::save(int slot, std::string& error) {
    if (m_options.savesDir.empty()) { error = "сохранения недоступны"; return false; }
    return m_engine->saveGame(slotPath(slot), error);
}

bool Session::load(int slot, std::string& error) {
    if (m_options.savesDir.empty()) { error = "сохранения недоступны"; return false; }
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

}  // namespace nlb::presentation
