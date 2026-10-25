#include "nlb/presentation/GameScreen.h"
#include "nlb/presentation/Localization.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <sstream>

#include "nlb/util/FileUtils.h"

namespace nlb::presentation {

namespace {
const Color TEXT = {235, 230, 220, 255};
const Color LINK = {130, 190, 255, 255};
const Color REACTION = {240, 200, 120, 255};
const Color DIM = {150, 150, 150, 255};
const Color BACKGROUND = {12, 12, 16, 255};
const Color SELECTED = {90, 70, 30, 255};
const float SCREEN_W = GameScreen::SCREEN_W, SCREEN_H = GameScreen::SCREEN_H;

bool contains(const RectF& r, float x, float y) {
    return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
}

/// Подпись объекта, которую стоит рисовать на экране: не внутреннее имя (abilitiesTrigger)
bool isHumanLabel(const std::string& s) {
    if (s.empty()) return false;
    for (unsigned char c : s) if (c == ' ' || c >= 0x80) return true;
    return false;
}

/// Слова с пробелами после них — для переноса строк
std::vector<std::string> words(const std::string& text) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : text) {
        cur += c;
        if (c == ' ') { out.push_back(cur); cur.clear(); }
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

/// Текст абзацев одной строкой — понять, что ответ на действие сменился (а не просто тикнул таймер)
std::string flatten(const std::vector<Paragraph>& text) {
    std::string out;
    for (const auto& p : text) {
        for (const auto& r : p.runs) out += r.text;
        out += '\n';
    }
    return out;
}

/// Строки подписи объекта (как texts в vn.lua: каждая строка — отдельно)
std::vector<std::string> labelLines(const std::string& label) {
    std::vector<std::string> lines;
    std::string cur;
    for (char c : label) {
        if (c == '\n') { lines.push_back(cur); cur.clear(); }
        else if (c != '\r') cur += c;
    }
    lines.push_back(cur);
    return lines;
}

/// «--» -> «—», как модуль dash STEAD (экспорт NLB всегда его подключает): между не-дефисами
/// и в начале строки; «---» не трогается
std::string dashes(const std::string& s) {
    if (s.find("--") == std::string::npos) return s;
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        const bool pair = i + 1 < s.size() && s[i] == '-' && s[i + 1] == '-';
        const bool before = i == 0 || s[i - 1] != '-';
        const bool after = i + 2 >= s.size() || s[i + 2] != '-';
        if (pair && before && after) { out += "\xE2\x80\x94"; ++i; }
        else out += s[i];
    }
    return out;
}

/// Кавычки, как модуль quotes STEAD (экспорт подключает его всегда): _" -> «, "_ -> »,
/// "текст" -> «текст», ,, -> „, '' -> ”
std::string quotes(const std::string& in) {
    if (in.find_first_of("\",'_") == std::string::npos) return in;
    auto replaceAll = [](std::string s, const std::string& a, const std::string& b) {
        for (size_t pos = 0; (pos = s.find(a, pos)) != std::string::npos; pos += b.size()) s.replace(pos, a.size(), b);
        return s;
    };
    std::string s = replaceAll(replaceAll(in, "_\"", "\xC2\xAB"), "\"_", "\xC2\xBB");
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '"') {
            const size_t close = s.find('"', i + 1);
            if (close != std::string::npos) {
                out += "\xC2\xAB" + s.substr(i + 1, close - i - 1) + "\xC2\xBB";
                i = close;
                continue;
            }
        }
        out += s[i];
    }
    return replaceAll(replaceAll(out, ",,", "\xE2\x80\x9E"), "''", "\xE2\x80\x9D");
}

std::string framePath(const ImageRef& image) {
    if (image.framePattern.empty()) return image.file.path;
    char buf[1024];
    std::snprintf(buf, sizeof(buf), image.framePattern.c_str(), std::max(1, image.currentFrame));
    return buf;
}
}

GameScreen::GameScreen(Session& session, Options options) : m_session(session), m_options(std::move(options)) {
    m_standardTheme = ScreenTheme::load(m_options.baseThemeDir, m_options.themeDir, false);
    m_vnTheme = ScreenTheme::load(m_options.baseThemeDir, m_options.themeDir, true);
    m_choicesScreen = m_options.choicesScreen;
    m_soundOn = m_options.soundAvailable;
    // размер текста: выбор игрока (меню, settings.ini), иначе — из параметров фронтенда
    m_textScale = m_options.textScale > 0 ? m_options.textScale : 1.0f;
    const std::string saved = m_session.setting("textscale");
    if (!saved.empty()) {
        const float v = std::strtof(saved.c_str(), nullptr);
        if (v >= 0.5f && v <= 3.0f) m_textScale = v;
    }
    pull();
}

void GameScreen::pull() {
    processEvents(m_session.takeEvents());
    const ScreenModel before = m_screen;
    m_screen = m_session.screen();
    updateEffects(before);
    m_seenRevision = m_session.revision();
    if (!choicesMode()) m_showChoices = false;  // страница STANDARD — экрана вариантов нет
    if (before.pageId != m_screen.pageId) {
        m_reactionThenRoom = true;  // ответ (если есть) — перед описанием новой комнаты
        m_portion = 0;  // новая страница — текст с первой порции
        m_voicedPortion = SIZE_MAX;  // озвучка — заново, с первой порции
        m_voiceOnEnter = true;
    } else if (!m_screen.reactionText.empty() && flatten(before.reactionText) != flatten(m_screen.reactionText)) {
        m_reactionThenRoom = false;  // новое действие на той же странице
        // ответ на действие на той же странице — показать его (первая порция ответа)
        m_portion = portionsOf(m_screen.pageText).size();
        m_scroll = 0;
    }
}

namespace {
size_t codepoints(const std::string& s) {
    size_t n = 0;
    for (unsigned char c : s) n += (c & 0xC0) != 0x80;
    return n;
}

/// Абзац -> порции, как экспорт в VN-INSTEAD (VNSTEADExportManager.getPreprocessedChunks):
/// предложения (до . ? ! и закрывающих кавычек) набираются в порцию, пока она не длиннее
/// PARAGRAPH_THRESHOLD символов; хвост, с которым последняя порция не длиннее
/// PARAGRAPH_THRESHOLD_WARN, присоединяется к ней. Ссылки и оформление абзаца сохраняются
std::vector<Paragraph> splitIntoPortions(const Paragraph& p) {
    const size_t THRESHOLD = 100, THRESHOLD_WARN = 330;
    std::string all;
    for (const auto& r : p.runs) all += r.text;
    // концы предложений (байтовые смещения): SENTENCE_PATTERN NLB — [^.?!]+ [.?!]+ (\")?
    std::vector<size_t> ends;
    for (size_t i = 0; i < all.size();) {
        if (all[i] == '.' || all[i] == '?' || all[i] == '!') {
            while (i < all.size() && (all[i] == '.' || all[i] == '?' || all[i] == '!')) ++i;
            // закрывающие кавычки и скобки — к предложению: « …!» », «…?”», «….)»
            for (bool more = true; more && i < all.size();) {
                more = false;
                for (const char* q : {"\"", "'", ")", "\xC2\xBB" /* » */, "\xE2\x80\x9D" /* ” */, "\xE2\x80\x9C" /* “ */}) {
                    const size_t n = std::char_traits<char>::length(q);
                    if (all.compare(i, n, q) == 0) { i += n; more = true; break; }
                }
            }
            ends.push_back(i);
        } else {
            ++i;
        }
    }
    if (ends.empty() || ends.back() != all.size()) ends.push_back(all.size());
    // порции по алгоритму экспортёра
    std::vector<std::pair<size_t, size_t>> chunks;  // [начало, конец)
    size_t start = 0, prevStart = 0, prevEnd = 0;
    bool hasPrev = false;
    for (size_t e : ends) {
        if (codepoints(all.substr(start, e - start)) > THRESHOLD) {
            if (hasPrev) chunks.push_back({prevStart, prevEnd});
            prevStart = start; prevEnd = e; hasPrev = true;
            start = e;
        }
    }
    if (hasPrev && codepoints(all.substr(prevStart, all.size() - prevStart)) <= THRESHOLD_WARN) {
        chunks.push_back({prevStart, all.size()});
    } else {
        if (hasPrev) chunks.push_back({prevStart, prevEnd});
        if (start < all.size()) chunks.push_back({start, all.size()});
    }
    if (chunks.size() <= 1) return {p};
    // разрезать фрагменты абзаца по границам порций
    std::vector<Paragraph> out;
    for (const auto& [b, e] : chunks) {
        Paragraph part = p;
        part.runs.clear();
        size_t pos = 0;
        for (const auto& r : p.runs) {
            const size_t rb = pos, re = pos + r.text.size();
            pos = re;
            const size_t from = std::max(rb, b), to = std::min(re, e);
            if (from >= to) continue;
            TextRun piece = r;
            piece.text = r.text.substr(from - rb, to - from);
            part.runs.push_back(piece);
        }
        // пробелы на стыке порций — прочь (paginator обрезает их у порции)
        while (!part.runs.empty()) {
            auto& t = part.runs.front().text;
            t.erase(0, t.find_first_not_of(" \t"));
            if (!t.empty()) break;
            part.runs.erase(part.runs.begin());
        }
        if (!part.runs.empty()) out.push_back(part);
    }
    return out;
}

bool isBlankParagraph(const Paragraph& p) {
    return p.runs.empty() || std::all_of(p.runs.begin(), p.runs.end(), [](const TextRun& r) {
        return r.text.find_first_not_of(" \t") == std::string::npos; });
}
}

std::vector<std::vector<Paragraph>> GameScreen::portionsOf(const std::vector<Paragraph>& text) {
    // Как экспорт в VN-INSTEAD: каждая строка текста — отдельная порция пагинатора (перевод
    // строки становится пустой строкой — paginator.delim), длинная строка — несколько порций по
    // предложениям (splitIntoPortions); пустые строки только разделяют
    std::vector<std::vector<Paragraph>> out;
    for (const auto& p : text) {
        if (isBlankParagraph(p)) continue;
        for (auto& part : splitIntoPortions(p)) out.push_back({part});
    }
    return out;
}

std::vector<std::vector<Paragraph>> GameScreen::portions() const {
    // ответ на действие продолжает порции страницы
    auto out = portionsOf(m_screen.pageText);
    for (auto& part : portionsOf(m_screen.reactionText)) out.push_back(std::move(part));
    return out;
}

int GameScreen::singleTrivialChoice() const {
    int found = -1, count = 0;
    for (const auto& c : m_screen.choices) {
        if (!c.enabled) continue;
        ++count;
        if (c.trivial) found = c.index;
    }
    return count == 1 ? found : -1;
}

uint64_t GameScreen::now() const {
    // пауза в меню: время экрана (анимации спрайтов, эффекты появления, всплывающие сообщения)
    // стоит, а после закрытия меню идёт дальше с того же места
    const uint64_t t = m_pauseStart ? m_pauseStart : rawNow();
    return t - m_pausedTotal;
}

void GameScreen::updatePause() {
    if (m_menu != Menu::None && !m_pauseStart) {
        m_pauseStart = rawNow();
        m_hoverObject.clear();  // под меню ничего не подсвечено
        m_hoverChoice = NO_HOVER;
        m_hoverItem.clear();
    } else if (m_menu == Menu::None && m_pauseStart) {
        m_pausedTotal += rawNow() - m_pauseStart;
        m_pauseStart = 0;
    }
}

uint64_t GameScreen::rawNow() const {
    if (m_options.clock) return m_options.clock();
    using namespace std::chrono;
    return static_cast<uint64_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

bool GameScreen::imageSize(const ImageRef& image, float& w, float& h) {
    return !image.empty() && m_r->imageSize(framePath(image), w, h);
}

// ============================================================================ время и события

void GameScreen::update(int milliseconds) {
    // Сессию изменили снаружи (подсистема движка, вернувшая книгу после внешней игры) — как после команды
    if (m_session.revision() != m_seenRevision) afterCommand();
    updatePause();
    if (m_menu != Menu::None) return;  // время книги идёт, только пока игрок не в меню (пауза)
    const uint64_t revision = m_session.revision();
    m_session.advance(milliseconds);
    if (m_session.revision() != revision) pull();
}

void GameScreen::processEvents(const std::vector<PlayerEngine::Event>& events) {
    using Kind = PlayerEngine::Event::Kind;
    if (m_eventListener && !events.empty()) m_eventListener(events);
    for (const auto& e : events) {
        if (e.kind == Kind::Achievement) {
            // «Получено достижение: …» — на языке интерфейса
            m_toasts.push_back(e.id.empty() ? e.text : tr(static_cast<int>(Ui::Achievement)) + e.id);
            m_toastUntil = now() + 4000;
        } else if (e.kind == Kind::PageText || e.kind == Kind::PageCaption) {
            m_scroll = 0;  // новая страница — с начала
            m_showChoices = false;
        }
    }
}

void GameScreen::afterCommand() {
    pull();
    m_showChoices = false;
    // Страница без текста: показывать нечего — сразу экран вариантов (если вариант один тривиальный —
    // ждём «дальше», чтобы игрок увидел сцену)
    if (choicesMode() && m_screen.pageText.empty() && m_screen.reactionText.empty() && singleTrivialChoice() < 0) {
        advanceToChoices();
    }
    if (m_screen.externalGame) {
        if (m_externalHandler) {
            // игру ведёт фронтенд (сцена движка, мини-игра) — сообщить один раз
            if (m_externalNotified != *m_screen.externalGame) {
                m_externalNotified = *m_screen.externalGame;
                m_externalHandler(*m_screen.externalGame);
            }
        } else {
            m_message = tr(static_cast<int>(Ui::ExternalGameMissing));
            const auto at = m_message.find("%s");
            if (at != std::string::npos) m_message.replace(at, 2, *m_screen.externalGame);
            m_menu = Menu::Message;
        }
    } else {
        m_externalNotified.clear();
        // Конец игры: сообщение «начать заново» — не сразу, а когда текст последней страницы
        // дочитан, и только в режиме экрана вариантов (VN, текст порциями: advanceText). В
        // режиме ссылок в тексте его нет вовсе, как в INSTEAD: игрок читает последнюю страницу,
        // а начать заново можно из меню. Страница без текста — сразу
        if (m_screen.finished && choicesMode() && m_screen.pageText.empty() && m_screen.reactionText.empty()) {
            m_message = tr(static_cast<int>(Ui::GameOver));
            m_menu = Menu::Message;
        }
    }
}

void GameScreen::updateEffects(const ScreenModel& before) {
    const uint64_t t = now();
    // Объект исчез со страницы (взят, убран, переход): эффект в обратную сторону на прежнем месте.
    // При смене страницы не-preserved объекты в vn.lua уходят вместе со сценой — без эффекта
    const bool samePage = before.pageId == m_screen.pageId;
    for (const auto& old : before.sceneObjects) {
        if (!samePage || !old.graphical || !old.visible || old.collapsed) continue;
        const bool stillHere = std::any_of(m_screen.sceneObjects.begin(), m_screen.sceneObjects.end(),
                                           [&](const SceneObject& o) { return o.id == old.id; });
        const AppearEffect e = old.appearance.effect;
        if (!stillHere && e != AppearEffect::None && e != AppearEffect::Overlap) {
            const int steps = std::max(1, old.appearance.maxStep > old.appearance.curStep
                                              ? old.appearance.maxStep - old.appearance.curStep : 25);
            m_effects[old.id] = Effect{t, steps, true, old};
        }
    }
    std::map<std::string, bool> shown;
    for (const auto& o : m_screen.sceneObjects) {
        if (!o.graphical) continue;
        const bool visible = o.visible && !o.collapsed;
        shown[o.id] = visible;
        const bool wasVisible = m_shown.count(o.id) && m_shown[o.id];
        const int steps = std::max(1, (o.appearance.maxStep > o.appearance.curStep ? o.appearance.maxStep - o.appearance.curStep : 25));
        const AppearEffect e = o.appearance.effect;
        if (visible && !wasVisible && (e == AppearEffect::FadeIn || e == AppearEffect::MoveIn || e == AppearEffect::ZoomIn)) {
            m_effects[o.id] = Effect{t, steps, false, o};
        } else if (!visible && wasVisible && e != AppearEffect::None && e != AppearEffect::Overlap) {
            // свиток свернулся: эффект в обратную сторону (vn:set_step(v, nil, not v.forward))
            m_effects[o.id] = Effect{t, steps, true, o};
        }
    }
    m_shown = shown;
}

GameScreen::EffectState GameScreen::effectState(const std::string& id, const RectF& r) {
    EffectState st;
    auto it = m_effects.find(id);
    if (it == m_effects.end()) return st;
    const Effect& e = it->second;
    float p = static_cast<float>(now() - e.start) / (static_cast<float>(e.steps) * 20.0f);
    if (p >= 1.0f) {
        if (e.hiding) { st.alpha = 0; }
        m_effects.erase(it);
        return st;
    }
    st.done = false;
    const float k = e.hiding ? 1.0f - p : p;  // 0 — не виден, 1 — на месте
    switch (e.object.appearance.effect) {
        case AppearEffect::FadeIn: case AppearEffect::FadeOut: st.alpha = k; break;
        case AppearEffect::ZoomIn: case AppearEffect::ZoomOut: st.scale = std::max(0.01f, k); break;
        case AppearEffect::MoveIn: case AppearEffect::MoveOut: {
            // выезжает из-за края экрана с указанной стороны (по умолчанию — слева)
            const float t = 1.0f - k;
            switch (e.object.appearance.direction) {
                case MoveDirection::Right: st.dx = (SCREEN_W - r.x) * t; break;
                case MoveDirection::Top: st.dy = -(r.y + r.h) * t; break;
                case MoveDirection::Bottom: st.dy = (SCREEN_H - r.y) * t; break;
                default: st.dx = -(r.x + r.w) * t; break;
            }
            break;
        }
        default: break;
    }
    return st;
}

// ============================================================================ ввод

void GameScreen::mouseMove(float x, float y) {
    m_mouseX = x;
    m_mouseY = y;
    if (m_menu != Menu::None) return;  // пауза: книга под меню на мышь не отзывается
    // что под курсором — подсвечивается цветом alink (объекты, переходы и заголовок, предметы)
    m_hoverObject.clear();
    m_hoverChoice = NO_HOVER;
    m_hoverItem.clear();
    for (auto it = m_hits.rbegin(); it != m_hits.rend(); ++it) {
        if (!contains(it->rect, x, y)) continue;
        if (it->kind == Hit::Kind::Object) m_hoverObject = it->id;
        else if (it->kind == Hit::Kind::Choice) m_hoverChoice = it->index;
        else if (it->kind == Hit::Kind::Inventory) m_hoverItem = it->id;
        else continue;
        break;
    }
}

void GameScreen::wheel(float lines) {
    // колесо: по 3 строки за щелчок (предел прокрутки — в renderTextWindow)
    if (m_menu == Menu::None && !m_showChoices) {
        m_scroll = std::max(0.0f, m_scroll - lines * 3 * m_lineHeight);
        m_scrollByPages = false;  // колесо — плавно, в пределах текста
    }
}

void GameScreen::click(float x, float y, bool rightButton) {
    if (m_menu == Menu::Message) {
        m_menu = Menu::None;
        if (m_screen.externalGame) m_session.finishExternalGame();
        else if (m_screen.finished) m_session.tryAgain();
        afterCommand();
        return;
    }
    if (m_menu != Menu::None) {
        // пауза: щёлкать можно только по пунктам меню, книга под ним не отзывается
        if (rightButton) return;
        for (auto it = m_hits.rbegin(); it != m_hits.rend(); ++it) {
            if (it->kind != Hit::Kind::Menu || !contains(it->rect, x, y)) continue;
            const ScreenTheme& t = m_screen.layout == Layout::VisualNovel ? m_vnTheme : m_standardTheme;
            m_session.playUiSound(t.clickSound);
            menuAction(it->index);
            return;
        }
        return;
    }
    if (rightButton) {
        m_session.clearSelection();
        m_seenRevision = m_session.revision();
        return;
    }
    bool hitSomething = false;
    for (const auto& h : m_hits) hitSomething = hitSomething || contains(h.rect, x, y);
    if (m_showChoices) {
        // экран вариантов: щелчок мимо вариантов — назад к тексту
        bool onChoice = false;
        for (const auto& h : m_hits) onChoice = onChoice || (h.kind == Hit::Kind::Choice && contains(h.rect, x, y));
        if (!onChoice) { m_showChoices = false; return; }
    } else if (choicesMode() && !hitSomething && m_menu == Menu::None) {
        // как paginator: щелчок мимо спрайтов — дальше по тексту
        advanceText();
        return;
    } else if (!hitSomething && m_menu == Menu::None && m_moreText && contains(m_textWindow, x, y)) {
        m_scroll += m_pageHeight; m_scrollByPages = true;
        return;
    } else if (!hitSomething && m_menu == Menu::None && advanceToChoices()) {
        return;
    }
    for (auto it = m_hits.rbegin(); it != m_hits.rend(); ++it) {
        if (!contains(it->rect, x, y)) continue;
        const Hit hit = *it;
        // щелчок по ссылке, предмету, пункту меню — звук темы (snd.click), как в INSTEAD
        if (hit.kind != Hit::Kind::ScrollUp && hit.kind != Hit::Kind::ScrollDown
            && (m_menu == Menu::None || hit.kind == Hit::Kind::Menu)) {
            const ScreenTheme& t = m_screen.layout == Layout::VisualNovel ? m_vnTheme : m_standardTheme;
            m_session.playUiSound(t.clickSound);
        }
        if (m_menu != Menu::None) {
            if (hit.kind == Hit::Kind::Menu) menuAction(hit.index);
            return;
        }
        if (m_showChoices && hit.kind != Hit::Kind::Choice && hit.kind != Hit::Kind::MenuButton) {
            continue;
        }
        switch (hit.kind) {
            case Hit::Kind::ScrollUp: m_scroll = std::max(0.0f, m_scroll - m_pageHeight); return;
            case Hit::Kind::ScrollDown: m_scroll += m_pageHeight; m_scrollByPages = true; return;
            case Hit::Kind::MenuButton: m_menu = Menu::Main; return;
            case Hit::Kind::Choice:
                if (hit.index == -1) {
                    m_session.look();  // заголовок комнаты: снова описание сцены
                } else {
                    m_session.choose(hit.index);
                }
                break;
            case Hit::Kind::Object:
                // выбран предмет — применить его к объекту (как в INSTEAD), иначе действие с объектом
                m_session.clickObject(hit.id);
                break;
            case Hit::Kind::Inventory: {
                // первый щелчок выбирает предмет, повторный — осмотр, к другому предмету — применение
                const bool wasEmpty = m_session.selectedItem().empty();
                m_session.clickItem(hit.id);
                if (wasEmpty && !m_session.selectedItem().empty()) {
                    m_seenRevision = m_session.revision();  // только выбор предмета — перерисовки хватит
                    return;
                }
                break;
            }
            case Hit::Kind::Menu: break;
        }
        afterCommand();
        return;
    }
}

void GameScreen::key(Key k, int digit) {
    if (k == Key::Escape) {
        if (m_showChoices && m_menu == Menu::None) { m_showChoices = false; return; }
        if (m_menu == Menu::None) m_menu = Menu::Main;
        else if (m_menu == Menu::ConfirmQuit || m_menu == Menu::Language) m_menu = Menu::Main;
        else if (m_menu != Menu::Message) m_menu = Menu::None;
        return;
    }
    if (m_menu != Menu::None) return;
    if (k == Key::Digit && digit >= 1 && digit <= 9) {
        // цифры — варианты по порядку
        int n = digit - 1, i = 0;
        for (const auto& c : m_screen.choices) {
            if (!c.enabled) continue;
            if (i++ == n) { m_session.choose(c.index); afterCommand(); return; }
        }
    } else if (k == Key::Space || k == Key::Enter) {
        // пробел: следующая часть текста, а когда он дочитан — экран вариантов
        if (choicesMode() && !m_showChoices) advanceText();
        else if (m_moreText) { m_scroll += m_pageHeight; m_scrollByPages = true; }
        else if (m_showChoices) m_showChoices = false;
        else advanceToChoices();
    } else if (k == Key::PageDown) {
        m_scroll += m_pageHeight; m_scrollByPages = true;
    } else if (k == Key::PageUp) {
        m_scroll = std::max(0.0f, m_scroll - m_pageHeight);
    } else if (k == Key::Save) {
        m_menu = Menu::Save;
    } else if (k == Key::Load) {
        m_menu = Menu::Load;
    }
}

bool GameScreen::hitCenter(const std::string& id, bool inventory, float& x, float& y) const {
    for (const auto& h : m_hits) {
        if (h.id == id && h.kind == (inventory ? Hit::Kind::Inventory : Hit::Kind::Object)) {
            x = h.rect.x + h.rect.w / 2;
            y = h.rect.y + h.rect.h / 2;
            return true;
        }
    }
    return false;
}

bool GameScreen::advanceText() {
    if (m_menu != Menu::None || m_showChoices) return false;
    if (m_moreText) {                       // не уместившиеся строки текущей порции
        m_scroll += m_pageHeight; m_scrollByPages = true;
        return true;
    }
    if (m_morePortions) {                   // следующая порция
        ++m_portion;
        m_scroll = 0;
        return true;
    }
    // текст дочитан: конец игры — сообщение «начать заново»
    if (m_screen.finished && std::none_of(m_screen.choices.begin(), m_screen.choices.end(), [](const Choice& c) { return c.enabled; })) {
        m_message = tr(static_cast<int>(Ui::GameOver));
        m_menu = Menu::Message;
        return true;
    }
    // единственный тривиальный вариант («Далее») — сразу переход, иначе варианты
    const int trivial = singleTrivialChoice();
    if (trivial >= 0) {
        m_session.choose(trivial);
        afterCommand();
        return true;
    }
    return advanceToChoices();
}

std::string GameScreen::describeThemes() const {
    auto orNone = [](const std::string& s) { return s.empty() ? std::string("нет") : s; };
    const ScreenTheme& t = m_standardTheme;
    return "тема игры: " + orNone(m_options.themeDir) + "; базовая тема: " + orNone(m_options.baseThemeDir)
        + "; фон страниц STANDARD (gfx/bg.jpg): " + orNone(t.pageBackground)
        + "; scr.gfx.bg: " + orNone(t.backgroundImage) + "; шрифт текста: " + orNone(t.text.fontFile)
        + "; щелчок (snd.click): " + orNone(t.clickSound);
}

std::string GameScreen::tr(int key) const {
    return uiText(static_cast<Ui>(key), m_session.language());
}

std::vector<GameScreen::MainItem> GameScreen::mainItems() const {
    std::vector<MainItem> items{MainItem::Save, MainItem::Load, MainItem::Restart, MainItem::Sound, MainItem::Choices};
    if (m_setFullscreen) items.push_back(MainItem::Fullscreen);
    if (m_session.availableLanguages().size() > 1) items.push_back(MainItem::Language);  // язык — если есть выбор
    items.push_back(MainItem::TextSize);
    items.push_back(MainItem::Quit);
    items.push_back(MainItem::Back);
    return items;
}

bool GameScreen::choiceCenter(int index, float& x, float& y) const {
    for (const auto& h : m_hits) {
        if (h.kind == Hit::Kind::Choice && h.index == index) {
            x = h.rect.x + h.rect.w / 2;
            y = h.rect.y + h.rect.h / 2;
            return true;
        }
    }
    return false;
}

bool GameScreen::menuItemCenter(int index, float& x, float& y) const {
    for (const auto& h : m_hits) {
        if (h.kind == Hit::Kind::Menu && h.index == index) {
            x = h.rect.x + h.rect.w / 2;
            y = h.rect.y + h.rect.h / 2;
            return true;
        }
    }
    return false;
}

bool GameScreen::advanceToChoices() {
    if (!choicesMode() || m_moreText || m_showChoices || m_menu != Menu::None) return false;
    if (std::none_of(m_screen.choices.begin(), m_screen.choices.end(), [](const Choice& c) { return c.enabled; })) {
        return false;
    }
    m_showChoices = true;
    return true;
}

void GameScreen::menuAction(int index) {
    std::string error;
    const bool saves = m_session.savesAvailable();
    if (m_menu == Menu::Main) {
        const auto items = mainItems();
        switch (index >= 0 && index < static_cast<int>(items.size()) ? items[index] : MainItem::Back) {
            case MainItem::Save: if (saves && m_screen.canSave) m_menu = Menu::Save; break;
            case MainItem::Load: if (saves) m_menu = Menu::Load; break;
            case MainItem::Restart: m_session.restart(); m_menu = Menu::None; afterCommand(); break;
            case MainItem::Sound:
                if (m_options.soundAvailable) { m_soundOn = !m_soundOn; m_session.setMuted(!m_soundOn); }
                break;
            case MainItem::Choices:
                m_choicesScreen = !m_choicesScreen;
                m_showChoices = false;
                break;
            case MainItem::Fullscreen:
                if (m_setFullscreen) m_setFullscreen(!(m_isFullscreen && m_isFullscreen()));
                break;
            case MainItem::Language: m_menu = Menu::Language; break;
            case MainItem::TextSize: {
                // 100 % -> 125 % -> … -> 200 % -> 100 %; запоминается для следующих запусков
                static const float STEPS[] = {1.0f, 1.25f, 1.5f, 1.75f, 2.0f};
                float next = STEPS[0];
                for (float st : STEPS) if (st > m_textScale + 0.01f) { next = st; break; }
                m_textScale = next;
                char buf[16];
                std::snprintf(buf, sizeof buf, "%.2f", next);
                m_session.setSetting("textscale", buf);
                m_scroll = 0;  // строки текста переразбиваются заново
                return;        // меню остаётся открытым: видно, как меняется размер
            }
            case MainItem::Quit: m_menu = Menu::ConfirmQuit; break;  // сначала спросить
            case MainItem::Back: m_menu = Menu::None; break;
        }
    } else if (m_menu == Menu::Language) {
        // 0 — «Назад», дальше языки книги по порядку
        const auto langs = m_session.availableLanguages();
        if (index >= 1 && index <= static_cast<int>(langs.size())) {
            m_session.setLanguage(langs[static_cast<size_t>(index - 1)], true);
            m_menu = Menu::None;
            afterCommand();
        } else {
            m_menu = Menu::Main;
        }
    } else if (m_menu == Menu::ConfirmQuit) {
        if (index == 0) { m_quit = true; m_menu = Menu::None; }  // «Да, выйти»
        else m_menu = Menu::Main;                                 // «Нет» — обратно в меню
    } else if (m_menu == Menu::Save || m_menu == Menu::Load) {
        if (index < 1 || index > 9) { m_menu = Menu::Main; return; }
        const bool ok = m_menu == Menu::Save ? m_session.save(index, error) : m_session.load(index, error);
        m_menu = Menu::None;
        if (!ok) { m_message = tr(static_cast<int>(Ui::Failed)) + error; m_menu = Menu::Message; return; }
        afterCommand();
    }
}

// ============================================================================ рисование

void GameScreen::render(IScreenRenderer& r) {
    m_r = &r;
    updatePause();
    m_r->fillRect({0, 0, SCREEN_W, SCREEN_H}, BACKGROUND);
    m_hits.clear();
    if (m_screen.layout == Layout::VisualNovel) renderVisualNovel();
    else renderStandard();
    // текст без порций (стандартная тема, VN с вариантами в окне текста): озвучка — только первая
    // фраза при входе на страницу (snd -> nextsnd NLB без пагинатора)
    if (m_voiceOnEnter) {
        m_voiceOnEnter = false;
        m_session.nextVoice();
    }
    renderOverlayMessages();
    if (m_menu != Menu::None) renderMenu();
    m_r = nullptr;
}

void GameScreen::drawObject(const SceneObject& o, const RectF& r, const FontRef& labelFont, Color labelColor, bool spriteLabel) {
    // Эффект родителя действует и на детей (в vn.lua дети получают шаги родителя)
    EffectState st = effectState(o.id, r);
    for (std::string p = o.parentId; st.done && !p.empty();) {
        auto pr = m_objectRects.find(p);
        if (pr == m_objectRects.end()) break;
        st = effectState(p, pr->second);
        const SceneObject* parent = nullptr;
        for (const auto& x : m_screen.sceneObjects) if (x.id == p) { parent = &x; break; }
        p = parent ? parent->parentId : "";
    }
    RectF dst{r.x + st.dx, r.y + st.dy, r.w, r.h};
    if (st.scale != 1.0f) {
        dst = {r.x + r.w * (1 - st.scale) / 2 + st.dx, r.y + r.h * (1 - st.scale) / 2 + st.dy, r.w * st.scale, r.h * st.scale};
    }
    const ImageRef& img = (m_hoverObject == o.id && !o.hoverImage.empty()) ? o.hoverImage : o.image;
    float iw = 0, ih = 0;
    const bool hasImage = imageSize(img, iw, ih);
    if (hasImage) m_r->drawImage(framePath(img), dst, st.alpha);
    if (isHumanLabel(o.label) && st.alpha > 0.05f) {
        if (!hasImage) {
            // Объект без картинки — только подпись, как draw_hud в vn.lua (в обоих режимах):
            // строки по одной (пустая строка — отступ hud_indent_spacing), блок справа от точки
            // объекта на default_label_extent + default_tooltip_offset (4 + 5 px). Прямоугольник
            // объекта (objectRect) уже охватывает блок подписи
            const float lh = m_r->fontHeight(labelFont);
            float ty = dst.y;
            for (const auto& line : labelLines(o.label)) {
                if (line.empty()) { ty += 25; continue; }
                m_r->drawText(labelFont, line, labelColor, dst.x + 4 + 5, ty, st.alpha);
                ty += lh;
            }
            return;
        }
        if (!spriteLabel) return;
        // Подпись спрайта — как draw_hud в vn.lua: справа от картинки (+4 +5 px), по вертикали
        // по углу привязки (top — верх, middle — середина, bottom — низ; у вложенного — середина)
        const float lh = m_r->lineSkip(labelFont);
        std::vector<std::string> lines;
        std::istringstream in(o.label);
        for (std::string line; std::getline(in, line);) lines.push_back(line);
        const float total = lh * static_cast<float>(lines.size());
        const int row = static_cast<int>(o.placement.anchor) / 3;
        const float coef = !o.parentId.empty() ? 0.5f : row == 0 ? 0.0f : row == 1 ? 0.5f : 1.0f;
        const float tx = dst.x + dst.w + 4 + 5;
        float ty = dst.y + dst.h * coef - total * coef;
        for (const auto& line : lines) {
            m_r->drawText(labelFont, line, labelColor, tx, ty, st.alpha);
            ty += lh;
        }
    }
}

float GameScreen::renderParagraphs(const std::vector<Paragraph>& text, float x, float y, float width,
                                   const FontRef& font, const ScreenTheme::Window& colors, bool draw, float blankLine) {
    // Как txt_layout INSTEAD (graphics.c): высота строки — целая часть высоты шрифта × fnt.height,
    // слово — по середине строки: (высота строки − высота шрифта) / 2
    const float px = m_r->pixelScale();  // округления — в пикселях окна, как у INSTEAD
    const float lineHeight = std::floor(m_r->fontHeight(font) * px * colors.lineHeight) / px;
    const float textOffset = std::floor((lineHeight - m_r->fontHeight(font)) * px / 2) / px;
    const ScreenTheme& theme = m_screen.layout == Layout::VisualNovel ? m_vnTheme : m_standardTheme;
    // glued — слово продолжает предыдущее без пробела («{кран}, испачканный»: запятая сразу за
    // ссылкой) — между ними не добавляется промежуток выравнивания
    // Картинка объекта в тексте (img() в {..} экспорта STEAD) — «слово» шириной в картинку; строка
    // с ней — высотой в самую высокую картинку, текст и картинки — по её середине, как в INSTEAD
    struct Word {
        std::string text; float w; TextRun::Kind kind; std::string target; bool glued = false;
        ImageRef image; float imageW = 0, imageH = 0;
    };
    // Строка выводится целиком: с выравниванием (win.align; по умолчанию INSTEAD — по ширине;
    // у абзаца «по центру» — по центру, как заголовок и пути в INSTEAD)
    auto flush = [&](std::vector<Word>& line, bool lastInParagraph, const Paragraph& p, const FontRef& pf) {
        if (line.empty()) return;
        // хвостовой пробел последнего слова в ширину строки не входит
        float total = 0, lineH = lineHeight;
        for (auto& w : line) {
            total += w.w;
            lineH = std::max(lineH, w.imageH);
        }
        const float trailing = textW(pf, " ") * (!line.back().text.empty() && line.back().text.back() == ' ' ? 1 : 0);
        total -= trailing;
        float cx = x, gap = 0;
        if (p.centered || theme.align == "center") cx = x + (width - total) / 2;
        else if (theme.align == "right") cx = x + width - total;
        else if (theme.align == "justify" && !lastInParagraph && line.size() > 1) {
            size_t gaps = 0;
            for (size_t i = 1; i < line.size(); ++i) gaps += line[i].glued ? 0 : 1;
            if (gaps) gap = (width - total) / static_cast<float>(gaps);
        }
        const bool visible = y >= m_drawTop - 0.5f && y + lineH <= m_drawBottom + 0.5f;
        if (draw && !visible && y + lineH > m_drawBottom + 0.5f && m_firstHiddenY < 0) m_firstHiddenY = y;
        const float ty = y + std::floor((lineH - lineHeight) * px / 2) / px;  // строка текста — по середине
        for (auto& w : line) {
            if (draw && visible) {
                Color c = colors.fg;
                if (w.kind == TextRun::Kind::ObjectLink) c = m_hoverObject == w.target ? colors.alink : colors.link;
                else if (w.kind == TextRun::Kind::ChoiceLink || p.style == Paragraph::Style::Title) {
                    int index = NO_HOVER;
                    try { index = std::stoi(w.target); } catch (...) {}
                    c = w.kind == TextRun::Kind::ChoiceLink && index == m_hoverChoice ? colors.alink : colors.link;
                }
                if (w.imageH > 0) {
                    m_r->drawImage(framePath(w.image), {cx, y + std::floor((lineH - w.imageH) * px / 2) / px, w.imageW, w.imageH}, 1.0f);
                } else {
                    m_r->drawText(pf, w.text, c, cx, ty + textOffset, 1.0f);
                }
                if (w.kind == TextRun::Kind::ObjectLink) {
                    m_hits.push_back({{cx, y, w.w, lineH}, Hit::Kind::Object, w.target});
                } else if (w.kind == TextRun::Kind::ChoiceLink) {
                    int index = -1;
                    try { index = std::stoi(w.target); } catch (...) {}
                    m_hits.push_back({{cx, y, w.w, lineH}, Hit::Kind::Choice, "", index});
                }
            }
            const size_t i = static_cast<size_t>(&w - &line[0]);
            cx += w.w + (i + 1 < line.size() && line[i + 1].glued ? 0 : gap);
        }
        line.clear();
        y += lineH;
    };
    for (const auto& p : text) {
        // курсив (txtem) — третьим начертанием шрифта темы, заголовок комнаты — вторым (полужирным),
        // как в INSTEAD; если начертания нет — обычное
        FontRef pf = font;
        if (p.style == Paragraph::Style::Emphasis && !colors.fontItalicFile.empty()) pf = FontRef{colors.fontItalicFile, font.size};
        if (p.style == Paragraph::Style::Title && !colors.fontBoldFile.empty()) pf = FontRef{colors.fontBoldFile, font.size};
        std::vector<Word> line;
        float lineW = 0;
        bool any = false;
        for (const auto& run : p.runs) {
            float iw = 0, ih = 0;
            if (imageSize(run.image, iw, ih)) {
                iw *= theme.scale;
                ih *= theme.scale;
                if (lineW + iw > width && !line.empty()) {
                    flush(line, false, p, pf);
                    lineW = 0;
                }
                const bool glued = !line.empty() && (line.back().text.empty() || line.back().text.back() != ' ');
                line.push_back({"", iw, run.kind, run.target, glued, run.image, iw, ih});
                lineW += iw;
                any = true;
            }
            for (const auto& w : words(quotes(dashes(run.text)))) {
                const float ww = textW(pf, w);
                // одни пробелы (на стыке фрагментов: «{кран} с водой») — к предыдущему слову, а в
                // начале строки — не нужны: иначе выравнивание по ширине удваивает промежутки
                if (w.find_first_not_of(' ') == std::string::npos) {
                    if (!line.empty()) { line.back().text += w; line.back().w += ww; lineW += ww; }
                    continue;
                }
                // перенос — как txt_layout INSTEAD: слово (без пробела после него) не помещается
                const std::string bare = w.substr(0, w.find_last_not_of(' ') + 1);
                const float bareW = bare.size() == w.size() ? ww : textW(pf, bare);
                if (lineW + bareW > width && !line.empty()) {
                    flush(line, false, p, pf);
                    lineW = 0;
                }
                // без пробела после предыдущего слова (стык фрагментов) — продолжение того же слова
                const bool glued = !line.empty() && (line.back().text.empty() || line.back().text.back() != ' ');
                line.push_back({w, ww, run.kind, run.target, glued});
                lineW += ww;
                any = true;
            }
        }
        if (any) flush(line, true, p, pf);
        else y += p.spacing > 0 ? p.spacing : lineHeight * blankLine;
    }
    return y;
}

float GameScreen::renderChoices(float x, float y, float width, const FontRef& font, const ScreenTheme::Window& colors, bool draw) {
    const float lineHeight = m_r->lineSkip(font) * std::max(1.15f, colors.lineHeight);
    y += lineHeight * 0.3f;
    int number = 0;
    for (const auto& c : m_screen.choices) {
        const std::string text = c.enabled ? std::to_string(++number) + ". " + c.text : c.altText;
        const float w = textW(font, text);
        if (draw && y >= m_drawTop - 0.5f && y + lineHeight <= m_drawBottom + 0.5f) {
            m_r->drawText(font, text, !c.enabled ? colors.fg : c.index == m_hoverChoice ? colors.alink : colors.link, x, y, 1.0f);
            if (c.enabled) m_hits.push_back({{x, y, std::min(w, width), lineHeight}, Hit::Kind::Choice, "", c.index});
        } else if (draw && y + lineHeight > m_drawBottom + 0.5f && m_firstHiddenY < 0) {
            m_firstHiddenY = y;
        }
        y += lineHeight;
    }
    return y;
}

RectF GameScreen::objectRect(size_t index) {
    const SceneObject& o = m_screen.sceneObjects[index];
    float w = 0, h = 0;
    // Объект без картинки, но с подписью: в vn.lua его спрайт пустой (0×0), а подпись — блок
    // строк справа от точки объекта, по вертикали — по углу привязки (get_pos_coef: top — вниз
    // от точки, middle — по центру, bottom — вверх; у вложенного — по центру)
    float labelW = 0, labelH = 0, labelCoef = 0;
    const bool textOnly = !imageSize(o.image, w, h) && isHumanLabel(o.label);
    if (textOnly) {
        const ScreenTheme& theme = m_screen.layout == Layout::VisualNovel ? m_vnTheme : m_standardTheme;
        const FontRef font = fontFor(theme.text);
        for (const auto& line : labelLines(o.label)) {
            if (line.empty()) { labelH += 25; continue; }  // hud_indent_spacing
            labelW = std::max(labelW, textW(font, line) + 4 + 5);
            labelH += m_r->fontHeight(font);
        }
        const int row = static_cast<int>(o.placement.anchor) / 3;
        labelCoef = !o.parentId.empty() ? 0.5f : row == 0 ? 0.0f : row == 1 ? 0.5f : 1.0f;
        w = h = 0;
    }
    auto finish = [&](float x, float y) -> RectF {
        if (textOnly) return {x, y - labelH * labelCoef, labelW, labelH};
        return {x, y, w, h};
    };
    const Placement& p = o.placement;
    auto parent = m_objectRects.find(o.parentId);
    if (!o.parentId.empty() && parent != m_objectRects.end()) {
        // Вложенный: точка родителя + iarm, иначе столбиком под предыдущими детьми (vn.lua)
        float ax = 0, ay = 0;
        if (p.hasInnerArm) {
            ax = static_cast<float>(p.innerArmX);
            ay = static_cast<float>(p.innerArmY);
        } else {
            float stacked = 0;
            for (size_t i = 0; i < index; ++i) {
                const SceneObject& prev = m_screen.sceneObjects[i];
                if (prev.parentId != o.parentId || prev.placement.hasInnerArm) continue;
                auto r = m_objectRects.find(prev.id);
                if (r != m_objectRects.end()) stacked += r->second.h;
            }
            ay = stacked;
        }
        return finish(parent->second.x + ax, parent->second.y + ay);
    }
    // Корневой: vn.lua postoxy
    const int a = static_cast<int>(p.anchor);
    const int col = a % 3, row = a / 3;   // 0 — left/top, 1 — middle, 2 — right/bottom
    float x = col == 0 ? static_cast<float>(p.armX) : col == 2 ? SCREEN_W - w : std::floor((SCREEN_W - w) / 2);
    float y = row == 0 ? static_cast<float>(p.armY) : row == 2 ? SCREEN_H - h : std::floor((SCREEN_H - h) / 2);
    if (p.hasOffset) { x += static_cast<float>(p.offsetX); y += static_cast<float>(p.offsetY); }
    return finish(x, y);
}

void GameScreen::renderTextWindow(const ScreenTheme::Window& window) {
    const FontRef font = fontFor(window);
    RectF r = window.rect;
    // Режим экрана вариантов: текст порциями (как paginator), без стрелок прокрутки
    std::vector<Paragraph> portionText;
    if (choicesMode()) {
        const auto parts = portions();
        if (m_portion >= parts.size()) m_portion = parts.empty() ? 0 : parts.size() - 1;
        if (!parts.empty()) portionText = parts[m_portion];
        m_morePortions = m_portion + 1 < parts.size();
        // озвучка: каждая новая порция — следующая фраза (paginator text_page -> nextsnd)
        if (!parts.empty() && m_portion != m_voicedPortion) {
            m_voicedPortion = m_portion;
            m_session.nextVoice();
        }
        m_voiceOnEnter = false;
    } else {
        m_morePortions = false;
    }
    // Страница STANDARD без экрана вариантов — как INSTEAD (game.c, stead2): заголовок по центру
    // цветом ссылки (щелчок — «осмотреться»), под ним пути через « | » по центру (или под
    // текстом: win.ways.mode = bottom), затем сцена: описание комнаты, со следующей строки
    // описания объектов через пробел; после действия — курсивом ответ, со следующей строки
    // описания объектов (описание комнаты — только при входе, а с game.forcedsc — и перед ответом)
    const bool instead = m_screen.layout == Layout::Standard && !choicesMode();
    std::pair<float, float> pictureSize{0, 0};  // картинка в окне текста (scr.gfx.mode fixed/embedded)
    size_t pictureAt = SIZE_MAX;                // её место: абзац заголовка (fixed) или сцены (embedded)
    bool pictureEmbedded = false;
    std::vector<Paragraph> composed;
    std::vector<Paragraph> header;  // заголовок и пути — над сценой, не прокручиваются (el_title, el_ways)
    if (instead) {
        const ScreenTheme& st = m_standardTheme;
        if (!m_screen.roomTitle.empty()) {
            // название комнаты: заголовок страницы, а без него — название модуля (nam в экспорте)
            Paragraph title;
            title.runs.push_back({TextRun::Kind::ChoiceLink, m_screen.roomTitle, "-1"});
            title.style = Paragraph::Style::Title;
            title.centered = true;
            header.push_back(title);
            Paragraph spacer;  // game.c: win_spacing = font_size * font_height / 2 (целое)
            spacer.spacing = std::floor(window.fontSize * m_r->pixelScale() * window.lineHeight / 2) / m_r->pixelScale();
            header.push_back(spacer);
        }
        Paragraph ways;
        ways.centered = true;
        for (const auto& c : m_screen.choices) {
            if (!c.enabled || c.inlineLink) continue;  // реплики и «Далее» — внизу текста (xdsc)
            if (!ways.runs.empty()) ways.runs.push_back({TextRun::Kind::Plain, " | ", ""});
            ways.runs.push_back({TextRun::Kind::ChoiceLink, c.text, std::to_string(c.index)});
        }
        if (!ways.runs.empty() && !st.waysBottom) header.push_back(ways);
        // Картинка страницы в окне текста (scr.gfx.mode, game.c INSTEAD): fixed — под заголовком,
        // не прокручивается, ниже — пути (а без них — отступ win_spacing), затем текст; embedded —
        // в начале текста сцены, за ней пути и сцена, всё прокручивается. Размер — с масштабом
        // темы, вписан в ширину окна и наибольшую высоту scr.gfx.h (game_pict_scale)
        float iw = 0, ih = 0;
        if (st.gfxMode != ScreenTheme::GfxMode::Float && !m_screen.imageIsBackground && imageSize(m_screen.pageImage, iw, ih)) {
            float pw = iw * st.scale, ph = ih * st.scale;
            const float maxH = st.gfxMaxHeight >= 0 ? st.gfxMaxHeight : ph;
            const float k = std::min({1.0f, r.w / pw, maxH / ph});
            pictureSize = {std::floor(pw * k), std::floor(ph * k)};
            const float spacing = std::floor(font.size * m_r->pixelScale() * window.lineHeight / 2) / m_r->pixelScale();  // font — с размером текста
            const bool waysAbove = !ways.runs.empty() && !st.waysBottom;  // пути — последний абзац header
            Paragraph place;  // место под картинку: пустой абзац её высоты
            place.spacing = pictureSize.second + (waysAbove ? 0 : spacing);
            if (st.gfxMode == ScreenTheme::GfxMode::Fixed) {
                // между заголовком (с его отступом) и путями
                pictureAt = waysAbove ? header.size() - 1 : header.size();
                header.insert(header.begin() + static_cast<std::ptrdiff_t>(pictureAt), place);
            } else {
                // в начале сцены, а пути — за ней
                if (waysAbove) header.pop_back();
                composed.push_back(place);
                pictureAt = 0;
                pictureEmbedded = true;
                if (waysAbove) composed.push_back(ways);
            }
        }
        if (!m_screen.reactionText.empty()) {
            // game.forcedsc: после действия на той же странице STEAD2 (iface.fmt) выводит сначала
            // me():look() — описание комнаты без вывода enter, — затем ответ
            if (!m_reactionThenRoom && m_screen.forceDsc) {
                composed.insert(composed.end(), m_screen.roomText.begin(), m_screen.roomText.end());
            }
            // ответ курсивом: строки ответа — с новой строки, а следующий ответ (другой p() в
            // экспорте) продолжает предыдущий через пробел, как stead.fmt. Пустой абзац — пустая
            // строка («^^» внутри ответа, «^» в конце перед следующей частью сцены); после «^» в
            // конце следующий ответ начинается с новой строки — продолжает этот пустой абзац
            for (const auto& p : m_screen.reactionText) {
                if (p.joinPrevious && !composed.empty() && composed.back().style == Paragraph::Style::Emphasis) {
                    if (!composed.back().runs.empty()) composed.back().runs.push_back({TextRun::Kind::Plain, " ", ""});
                    composed.back().runs.insert(composed.back().runs.end(), p.runs.begin(), p.runs.end());
                } else {
                    Paragraph em = p;
                    em.style = Paragraph::Style::Emphasis;
                    composed.push_back(em);
                }
            }
            // действие привело на новую страницу: после ответа — и её описание, чтобы не потерять
            // ни то, ни другое (INSTEAD показал бы только комнату, а ответ пропал бы)
            if (m_reactionThenRoom) {
                composed.insert(composed.end(), m_screen.modText.begin(), m_screen.modText.end());
                composed.insert(composed.end(), m_screen.roomText.begin(), m_screen.roomText.end());
            }
        } else {
            // текст модификаций страницы — вывод enter: stead.go выводит его перед сценой. После
            // действия на той же странице его уже нет — только ответ и сцена
            composed.insert(composed.end(), m_screen.modText.begin(), m_screen.modText.end());
            composed.insert(composed.end(), m_screen.roomText.begin(), m_screen.roomText.end());
        }
        if (!m_screen.objectsText.empty()) {
            composed.insert(composed.end(), m_screen.objectsText.begin(), m_screen.objectsText.end());
        }
        // xdsc экспорта: после описаний объектов — с новой строки alt-тексты недоступных ссылок,
        // затем ссылки в тексте (реплики диалога, «Далее»), каждая отдельной строкой
        for (const auto& c : m_screen.choices) {
            if (c.enabled || c.altText.empty()) continue;
            Paragraph alt;
            alt.runs.push_back({TextRun::Kind::Plain, c.altText, ""});
            composed.push_back(alt);
        }
        for (const auto& c : m_screen.choices) {
            if (!c.enabled || !c.inlineLink) continue;
            Paragraph link;
            link.runs.push_back({TextRun::Kind::ChoiceLink, c.text, std::to_string(c.index)});
            composed.push_back(link);
        }
        // переводы строк в конце всей сцены не видны (INSTEAD)
        while (!composed.empty() && composed.back().runs.empty() && composed.back().spacing <= 0) composed.pop_back();
        if (!ways.runs.empty() && st.waysBottom) composed.push_back(ways);
    }
    const std::vector<Paragraph>& firstText = instead ? composed : choicesMode() ? portionText : m_screen.pageText;
    static const std::vector<Paragraph> NONE;
    const std::vector<Paragraph>& secondText = instead || choicesMode() ? NONE : m_screen.reactionText;
    const bool showCaption = !instead && !m_screen.caption.empty();
    const bool showChoices = !instead && !choicesMode();

    // Заголовок и пути (INSTEAD: el_title, el_ways) — над прокручиваемой сценой, на месте; при
    // scr.gfx.mode = fixed между ними — картинка страницы
    if (!header.empty()) {
        m_drawTop = -1e9f;
        m_drawBottom = 1e9f;
        float hy = r.y;
        if (pictureAt != SIZE_MAX && !pictureEmbedded) {
            const std::vector<Paragraph> above(header.begin(), header.begin() + static_cast<std::ptrdiff_t>(pictureAt));
            const std::vector<Paragraph> below(header.begin() + static_cast<std::ptrdiff_t>(pictureAt) + 1, header.end());
            hy = renderParagraphs(above, r.x, hy, r.w, font, window, true);
            m_r->drawImage(framePath(m_screen.pageImage),
                           {r.x + (r.w - pictureSize.first) / 2, hy, pictureSize.first, pictureSize.second}, 1.0f);
            hy = renderParagraphs({header[pictureAt]}, r.x, hy, r.w, font, window, true);
            hy = renderParagraphs(below, r.x, hy, r.w, font, window, true);
        } else {
            hy = renderParagraphs(header, r.x, hy, r.w, font, window, true);
        }
        const float headerH = hy - r.y;
        r.y += headerH;
        r.h = std::max(0.0f, r.h - headerH);
    }

    // Сколько места занимает текст (для прокрутки)
    float need = 0;
    if (showCaption) need += m_r->lineSkip(font) * 1.4f;
    const float blankLine = instead ? 1.0f : 0.5f;
    need = renderParagraphs(firstText, r.x, need, r.w, font, window, false, blankLine);
    need = renderParagraphs(secondText, r.x, need, r.w, font, window, false, blankLine);
    if (showChoices) need = renderChoices(r.x, need, r.w, font, window, false);
    m_contentHeight = need;
    // Окно показывает целое число строк; не уместившийся текст листается страницами, как
    // paginator в INSTEAD: щелчок по окну (или колесо, PgDn) — следующая часть
    const float lh = std::floor(m_r->fontHeight(font) * m_r->pixelScale() * window.lineHeight) / m_r->pixelScale();
    m_lineHeight = lh;
    const float pageH = std::max(lh, std::floor(r.h / lh) * lh);
    // Листание страницами начинает каждую с первой не уместившейся строки и на последней не
    // возвращается назад: её окно может быть заполнено не целиком — прочитанное не повторяется
    const float maxScroll = std::max(0.0f, m_scrollByPages ? need - lh : need - pageH);
    m_scroll = std::min(m_scroll, maxScroll);
    m_textWindow = r;
    m_pageHeight = pageH;
    m_moreText = m_scroll < maxScroll;
    const RectF clip{std::trunc(r.x - 8), std::trunc(r.y), std::trunc(r.w + 16), std::trunc(r.h)};
    m_r->setClip(&clip);
    const size_t firstHit = m_hits.size();
    float y = r.y - m_scroll;
    m_drawTop = r.y;
    m_drawBottom = r.y + r.h;
    m_firstHiddenY = -1;
    if (showCaption) {
        m_r->drawText(font, m_screen.caption, window.fg, r.x, y, 1.0f);
        y += m_r->lineSkip(font) * 1.4f;
    }
    // scr.gfx.mode = embedded: картинка — в начале сцены, прокручивается с текстом (место под неё —
    // первый абзац composed)
    if (pictureEmbedded) {
        m_r->drawImage(framePath(m_screen.pageImage),
                       {r.x + (r.w - pictureSize.first) / 2, y, pictureSize.first, pictureSize.second}, 1.0f);
    }
    y = renderParagraphs(firstText, r.x, y, r.w, font, window, true, blankLine);
    y = renderParagraphs(secondText, r.x, y, r.w, font, window, true, blankLine);
    if (showChoices) renderChoices(r.x, y, r.w, font, window, true);
    m_r->setClip(nullptr);
    m_drawTop = -1e9f;
    m_drawBottom = 1e9f;
    // Следующая страница начинается с первой строки, которая не уместилась
    m_moreText = m_firstHiddenY >= 0;
    if (m_moreText) m_pageHeight = std::max(lh, m_firstHiddenY - r.y);
    // Ссылки за краем окна не кликабельны
    m_hits.erase(std::remove_if(m_hits.begin() + static_cast<long>(firstHit), m_hits.end(), [&](const Hit& h) {
        return h.rect.y + h.rect.h <= r.y || h.rect.y >= r.y + r.h;
    }), m_hits.end());
    // Стрелки прокрутки: картинки темы (win.gfx.up/down), без них — символы; по ним можно щёлкать
    auto arrow = [&](bool up, float ay) {
        const std::string& path = up ? window.upImage : window.downImage;
        float aw = lh * 0.8f, ah = lh * 0.8f;
        const bool img = !path.empty() && m_r->imageSize(path, aw, ah);
        if (img) {  // картинки темы — в масштабе её экрана (scr.w×scr.h -> 1920×1080)
            const float k = (m_screen.layout == Layout::VisualNovel ? m_vnTheme : m_standardTheme).scale;
            aw *= k;
            ah *= k;
        }
        // места стрелок — win.up.x/y, win.down.x/y темы (левый верхний угол; у нижней по x — как у
        // верхней, как в INSTEAD), иначе справа от окна: вверху и внизу
        const float tx = up ? window.upX : (window.downX >= 0 ? window.downX : window.upX);
        const float ty = up ? window.upY : window.downY;
        const RectF ar{tx >= 0 ? tx : r.x + r.w + 6, ty >= 0 ? ty : (up ? ay : ay - ah), aw, ah};
        if (img) m_r->drawImage(path, ar, 1.0f);
        else m_r->drawText(font, up ? "▲" : "▼", window.link, ar.x, ar.y, 1.0f);
        m_hits.push_back({{ar.x - 6, ar.y - 6, ar.w + 12, ar.h + 12}, up ? Hit::Kind::ScrollUp : Hit::Kind::ScrollDown, ""});
    };
    if (choicesMode()) return;  // как paginator: без прокрутки, «дальше» — щелчок или пробел
    if (m_scroll > 0) arrow(true, r.y);
    if (m_firstHiddenY >= 0) arrow(false, r.y + r.h);
}

void GameScreen::renderSceneObjects(const ScreenTheme& t, bool spriteLabels, bool hidden) {
    // Графические объекты в порядке обхода сцены (родитель раньше детей)
    m_objectRects.clear();
    const FontRef labelFont = fontFor(t.text);
    for (size_t i = 0; i < (hidden ? 0 : m_screen.sceneObjects.size()); ++i) {
        const SceneObject& o = m_screen.sceneObjects[i];
        if (!o.graphical || !o.visible) continue;
        if (o.collapsed) {
            // свёрнутый свиток виден, пока идёт обратный эффект; щелчков не принимает
            auto e = m_effects.find(o.id);
            if (e != m_effects.end() && e->second.hiding) {
                const RectF r = objectRect(i);
                m_objectRects[o.id] = r;
                const ScreenTheme& lt = m_screen.layout == Layout::VisualNovel ? m_vnTheme : m_standardTheme;
                drawObject(o, r, fontFor(lt.text), lt.text.fg, m_screen.layout == Layout::VisualNovel);
            }
            continue;
        }
        const RectF r = objectRect(i);
        m_objectRects[o.id] = r;
        drawObject(o, r, labelFont, t.text.fg, spriteLabels);
        if (o.active) m_hits.push_back({r, Hit::Kind::Object, o.id});
    }
    renderVanishing(labelFont, t.text.fg, spriteLabels);
}

void GameScreen::renderStandard() {
    const ScreenTheme& t = m_standardTheme;
    float w = 0, h = 0;
    // Своя картинка-фон страницы (theme.gfx.bg экспорта) заменяет фон темы — как фон INSTEAD
    // (themes.c): в своём размере с масштабом темы, по центру экрана темы, вокруг — цвет рамки
    const bool pageBackground = m_screen.imageIsBackground && imageSize(m_screen.pageImage, w, h);
    // вне экрана темы (scr.w×scr.h, вписан в 1920×1080) — цвет рамки scr.col.brd
    m_r->fillRect({0, 0, SCREEN_W, SCREEN_H}, t.border);
    m_r->fillRect(t.screenArea, pageBackground ? t.border : t.background);
    if (pageBackground) {
        const RectF a = t.screenArea;
        const float bw = w * t.scale, bh = h * t.scale;
        m_r->drawImage(framePath(m_screen.pageImage), {a.x + (a.w - bw) / 2, a.y + (a.h - bh) / 2, bw, bh}, 1.0f);
    } else {
        // Фон: у страницы STANDARD без своего фона экспорт ставит bgimg = nlb:std_bg() (gfx/bg.jpg
        // темы); если его нет — scr.gfx.bg из theme.ini
        const std::string& bg = !t.pageBackground.empty() ? t.pageBackground : t.backgroundImage;
        if (!bg.empty() && m_r->imageSize(bg, w, h)) {
            m_r->drawImage(bg, t.screenArea, 1.0f);  // фон темы — на её экран (scr.w×scr.h)
        }
        // scr.gfx.mode = float: картинка страницы в своей области scr.gfx, вписана и по центру;
        // fixed и embedded — в окне текста (renderTextWindow)
        if (t.gfxMode == ScreenTheme::GfxMode::Float && imageSize(m_screen.pageImage, w, h)) {
            const RectF a = t.pictureArea;
            const float k = std::min(a.w / w, a.h / h);
            m_r->drawImage(framePath(m_screen.pageImage), {a.x + (a.w - w * k) / 2, a.y + (a.h - h * k) / 2, w * k, h * k}, 1.0f);
        }
    }
    renderSceneObjects(t, false, false);
    if (m_showChoices) {
        renderChoicesScreen();
        renderMenuButton(t);
        return;
    }
    // картинка-фон страницы — как в INSTEAD: под окном текста без подложки
    renderTextWindow(t.text);
    renderInventory(t.inventory);
    renderMenuButton(t);
}

void GameScreen::renderVisualNovel() {
    const ScreenTheme& t = m_vnTheme;
    m_r->fillRect({0, 0, SCREEN_W, SCREEN_H}, t.border);
    m_r->fillRect(t.screenArea, t.background);
    float w = 0, h = 0;
    if (imageSize(m_screen.pageImage, w, h)) m_r->drawImage(framePath(m_screen.pageImage), {0, 0, SCREEN_W, SCREEN_H}, 1.0f);
    // На экране вариантов объектов нет — как в INSTEAD, где на время выбора остаются только фон и кнопка меню
    renderSceneObjects(t, true, m_showChoices);
    // Полоса текста — только если в ней есть что показать: текст страницы или ответ, а в режиме
    // ссылок в тексте — и переходы. На странице без текста в режиме экрана вариантов (переходы —
    // отдельным экраном) полупрозрачной подложки нет, картинка видна целиком
    const bool anyText = std::any_of(m_screen.pageText.begin(), m_screen.pageText.end(), [](const Paragraph& p) { return !p.runs.empty(); })
                         || std::any_of(m_screen.reactionText.begin(), m_screen.reactionText.end(), [](const Paragraph& p) { return !p.runs.empty(); });
    const bool hasText = anyText || (!choicesMode() && !m_screen.choices.empty());
    if (m_showChoices) {
        renderChoicesScreen();
    } else if (hasText) {
        // Каждая VN-страница экспорта вызывает vn:geom(8, 864, 1904, 184, 'dissolve', 240): окно
        // текста — полоса почти во всю ширину, а текст — внутри неё с полями по 240 px.
        // Полоса — полупрозрачная, как на экране INSTEAD (рамки fl/fr там не видны)
        // Крупный текст (размер текста > 100 %): полоса выше в той же пропорции (строк в ней
        // столько же) и с меньшими полями, низ — на месте, но не ниже строки инвентаря
        float bottom = 864 + 184;
        if (m_textScale > 1.0f && !m_screen.inventory.empty()) {  // при 100 % — ровно как vn:geom
            const FontRef invFont = fontFor(t.inventory);
            const float invLine = m_r->fontHeight(invFont) * t.inventory.lineHeight;
            bottom = std::min(bottom, std::min(t.inventory.rect.y, SCREEN_H - invLine) - 2);
        }
        const float bandH = std::min(184 * m_textScale, SCREEN_H * 0.5f);
        const RectF band{8, bottom - bandH, 1904, bandH};
        const float wf = 240 / std::max(1.0f, m_textScale), pad = 8;  // vn.lua: textpad = 8
        m_r->fillRect({0, band.y, SCREEN_W, band.h + 16}, {0, 0, 0, 110});
        ScreenTheme::Window win = t.text;
        win.rect = {band.x + wf, band.y + pad, band.w - 2 * wf, band.h - 2 * pad};
        renderTextWindow(win);
    }
    renderMenuButton(t);
    renderInventory(t.inventory);
}

void GameScreen::renderInventory(const ScreenTheme::Window& window) {
    if (m_screen.inventory.empty()) return;
    const FontRef font = fontFor(window);
    RectF r = window.rect;
    // высота строки — как у текста (inv.fnt.height), слово — по середине строки
    const float px = m_r->pixelScale();
    const float lineHeight = std::floor(m_r->fontHeight(font) * px * window.lineHeight) / px;
    const float textOffset = std::floor((lineHeight - m_r->fontHeight(font)) * px / 2) / px;
    // Предметы в строку разделяются « | », как hinv_delim в INSTEAD (game.gui)
    const bool horizontal = m_standardTheme.inventoryHorizontal;
    const std::string delim = " | ";
    const float gap = textW(font, horizontal ? delim : "    ");
    const std::string& selected = m_session.selectedItem();
    // Подпись предмета из нескольких строк («^» в disp: «Слава: 0^Боеспособность: 45») — как в
    // INSTEAD, каждая строка с новой строки инвентаря; кусок — одна строка подписи (картинка —
    // у первой, разделитель « | » — после последней)
    struct Item { const InventoryItem* item; std::string label; float w; bool first; bool last; };
    std::vector<Item> items;
    for (const auto& item : m_screen.inventory) {
        std::vector<std::string> lines;
        std::string line;
        for (char c : item.label) {
            if (c == '\n') { lines.push_back(line); line.clear(); } else line += c;
        }
        lines.push_back(line);
        float iw = 0, ih = 0;
        const float icon = imageSize(item.icon, iw, ih) ? lineHeight + 6 : 0;
        for (size_t i = 0; i < lines.size(); ++i) {
            items.push_back({&item, lines[i], textW(font, lines[i]) + (i == 0 ? icon : 0), i == 0, i + 1 == lines.size()});
        }
    }
    // inv.mode = horizontal-center: строки предметов по центру области; vertical — столбиком
    std::vector<std::vector<Item>> rows(1);
    float rowW = 0;
    for (const auto& it : items) {
        if (!it.first && !rows.back().empty()) { rows.emplace_back(); rowW = 0; }
        if (!it.first) {
            rowW += it.w;
            rows.back().push_back(it);
            continue;
        }
        if (m_standardTheme.inventoryHorizontal && !rows.back().empty() && rowW + gap + it.w > r.w) { rows.emplace_back(); rowW = 0; }
        if (!m_standardTheme.inventoryHorizontal && !rows.back().empty()) { rows.emplace_back(); rowW = 0; }
        rowW += (rows.back().empty() ? 0 : gap) + it.w;
        rows.back().push_back(it);
    }
    // VN: под инвентарём — сплошная чёрная полоса до низа экрана, иначе подписи предметов
    // теряются на картинке сцены. Крупный текст (размер текста) не уходит за низ экрана: строки
    // предметов поднимаются, и полоса вместе с ними
    if (m_screen.layout == Layout::VisualNovel) {
        const float need = lineHeight * static_cast<float>(rows.size());
        if (r.y + need > SCREEN_H) r.y = std::max(0.0f, SCREEN_H - need);
        m_r->fillRect({0, r.y, SCREEN_W, SCREEN_H - r.y}, {0, 0, 0, 255});
    }
    float y = r.y;
    for (const auto& row : rows) {
        float total = 0;
        for (size_t i = 0; i < row.size(); ++i) total += row[i].w + (i ? gap : 0);
        // inv.mode = horizontal-left / -center / -right
        const std::string& ia = m_standardTheme.inventoryAlign;
        float x = !m_standardTheme.inventoryHorizontal || ia == "left" ? r.x
                : ia == "right" ? r.x + r.w - total : r.x + (r.w - total) / 2;
        for (const auto& it : row) {
            const RectF cell{x, y, it.w, lineHeight};
            // выбранный предмет — на тёмной плашке: светлая на светлой подложке темы была не видна
            if (it.item->id == selected) m_r->fillRect({cell.x - 6, cell.y, cell.w + 12, cell.h}, {0, 0, 0, 150});
            float tx = x, iw = 0, ih = 0;
            if (it.first && imageSize(it.item->icon, iw, ih)) {
                m_r->drawImage(framePath(it.item->icon), {x, y, lineHeight, lineHeight}, 1.0f);
                tx += lineHeight + 6;
            }
            const bool isSelected = it.item->id == selected;
            const bool hovered = it.item->id == m_hoverItem;
            m_r->drawText(font, it.label, !it.item->active ? window.fg : isSelected || hovered ? window.alink : window.link, tx, y + textOffset, 1.0f);
            if (it.item->active) m_hits.push_back({cell, Hit::Kind::Inventory, it.item->id});
            if (horizontal && it.last && &it != &row.back()) m_r->drawText(font, delim, window.fg, x + it.w, y + textOffset, 1.0f);
            x += it.w + gap;
        }
        y += lineHeight;
        if (y > r.y + r.h + lineHeight) break;
    }
}

void GameScreen::renderVanishing(const FontRef& labelFont, Color labelColor, bool spriteLabel) {
    // Объекты, которых уже нет в снимке, но их эффект исчезновения ещё идёт
    for (auto it = m_effects.begin(); it != m_effects.end();) {
        const Effect& e = it->second;
        const bool inSnapshot = std::any_of(m_screen.sceneObjects.begin(), m_screen.sceneObjects.end(),
                                            [&](const SceneObject& o) { return o.id == it->first; });
        auto rect = m_lastRects.find(it->first);
        if (!e.hiding || inSnapshot || rect == m_lastRects.end()) { ++it; continue; }
        const SceneObject object = e.object;
        const RectF r = rect->second;
        ++it;  // drawObject может удалить закончившийся эффект
        drawObject(object, r, labelFont, labelColor, spriteLabel);
    }
    for (const auto& [id, r] : m_objectRects) m_lastRects[id] = r;
}

void GameScreen::renderMenuButton(const ScreenTheme& theme) {
    float w = 0, h = 0;
    // Игра NLB при запуске ставит кнопку сама (nlb.lua, _syscall_showmenubtn): gfx/menubtn_ru.png
    // (для русской книги) или gfx/menubtn.png активной темы, в правый верхний угол — x = scr.w −
    // ширина картинки, y = 0. Она заменяет menu.gfx.button / menu.button.x/y из theme.ini
    const std::string nlbButton = m_session.language() == "ru" && !theme.nlbMenuButtonRu.empty()
                                      ? theme.nlbMenuButtonRu : theme.nlbMenuButton;
    if (!nlbButton.empty() && m_r->imageSize(nlbButton, w, h)) {
        const RectF r{theme.screenArea.x + theme.screenArea.w - w * theme.scale, theme.screenArea.y, w * theme.scale, h * theme.scale};
        m_r->drawImage(nlbButton, r, 1.0f);
        m_hits.push_back({r, Hit::Kind::MenuButton, ""});
        return;
    }
    // Без неё — menu.gfx.button в menu.button.x/y из theme.ini, как в INSTEAD.
    // Языковой вариант картинки, если есть: menubtn.png -> menubtn_ru.png для русской книги
    std::string path = theme.menuButton;
    if (path.empty()) return;
    const auto dot = path.find_last_of('.');
    const std::string localized = dot == std::string::npos ? "" : path.substr(0, dot) + "_" + m_session.language() + path.substr(dot);
    if (!localized.empty() && FileUtils::exists(localized)) path = localized;
    if (!m_r->imageSize(path, w, h)) return;
    const RectF r{theme.menuButtonX, theme.menuButtonY, w * theme.scale, h * theme.scale};
    m_r->drawImage(path, r, 1.0f);
    m_hits.push_back({r, Hit::Kind::MenuButton, ""});
}

void GameScreen::renderChoicesScreen() {
    // Варианты посередине экрана на полупрозрачной панели; объекты сцены в это время не кликабельны
    m_hits.erase(std::remove_if(m_hits.begin(), m_hits.end(), [](const Hit& h) {
        return h.kind == Hit::Kind::Object || h.kind == Hit::Kind::Inventory;
    }), m_hits.end());
    const ScreenTheme::Window& window = m_screen.layout == Layout::VisualNovel ? m_vnTheme.text : m_standardTheme.text;
    const FontRef font = scaled(FontRef{window.fontFile, window.fontSize * 1.25f});
    const float lh = m_r->lineSkip(font);
    const float step = lh * 1.9f;  // от начала варианта до начала следующего (однострочного)
    const float panelW = SCREEN_W * std::min(0.95f, 0.675f * std::max(1.0f, m_textScale * 0.8f));  // крупный текст — подложка шире
    const float textWidth = panelW - 20;
    // Вариант, не умещающийся в ширину подложки, переносится по словам; продолжение — с отступом
    // под текстом после номера («2. »)
    struct Entry { std::vector<std::string> lines; float indent = 0; const Choice* choice; };
    std::vector<Entry> entries;
    int number = 0;
    float total = 0;
    for (const auto& c : m_screen.choices) {
        const std::string prefix = c.enabled ? std::to_string(++number) + ". " : "";
        const std::string body = c.enabled ? c.text : c.altText;
        Entry e{{}, c.enabled ? textW(font, prefix) : 0.0f, &c};
        std::string line = prefix;
        float lineW = textW(font, line);
        size_t pos = 0;
        while (pos < body.size()) {
            size_t end = body.find(' ', pos);
            if (end == std::string::npos) end = body.size();
            const std::string word = body.substr(pos, end - pos);
            const std::string sep = (line.empty() || line == prefix) ? "" : " ";
            const float limit = e.lines.empty() ? textWidth : textWidth - e.indent;
            const float ww = textW(font, sep + word);
            if (lineW + ww > limit && !(line.empty() || line == prefix)) {
                e.lines.push_back(line);
                line = word;
                lineW = textW(font, word);
            } else {
                line += sep + word;
                lineW += ww;
            }
            pos = end + 1;
        }
        if (!line.empty()) e.lines.push_back(line);
        if (e.lines.empty()) e.lines.push_back(prefix);
        total += step + lh * static_cast<float>(e.lines.size() - 1);
        entries.push_back(std::move(e));
    }
    const float panelH = std::max(SCREEN_H * 0.46f, total + lh);
    const RectF panel{(SCREEN_W - panelW) / 2, std::max(20.0f, (SCREEN_H - panelH) / 2 - 80), panelW, panelH};
    m_r->fillRect(panel, {0, 0, 0, 120});
    float y = panel.y + lh * 0.5f;
    for (const auto& e : entries) {
        const Choice& c = *e.choice;
        float w = 0;
        for (size_t i = 0; i < e.lines.size(); ++i) w = std::max(w, (i ? e.indent : 0) + textW(font, e.lines[i]));
        const RectF r{panel.x + 10, y, std::min(w, textWidth), lh * static_cast<float>(e.lines.size())};
        const bool hover = c.enabled && m_menu == Menu::None && contains(r, m_mouseX, m_mouseY);
        const Color color = !c.enabled ? window.fg : hover ? window.alink : window.link;
        for (size_t i = 0; i < e.lines.size(); ++i) {
            m_r->drawText(font, e.lines[i], color, r.x + (i ? e.indent : 0), y + lh * static_cast<float>(i), 1.0f);
        }
        if (c.enabled) m_hits.push_back({r, Hit::Kind::Choice, "", c.index});
        y += step + lh * static_cast<float>(e.lines.size() - 1);
    }
}

void GameScreen::renderOverlayMessages() {
    if (!m_toasts.empty() && now() < m_toastUntil) {
        float y = 30;
        for (const auto& t : m_toasts) {
            const float w = textW(scaled(m_options.uiFont), t), h = m_r->fontHeight(scaled(m_options.uiFont));
            m_r->fillRect({SCREEN_W - w - 70, y - 10, w + 40, h + 20}, SELECTED);
            m_r->drawText(scaled(m_options.uiFont), t, TEXT, SCREEN_W - w - 50, y, 1.0f);
            y += h + 30;
        }
    } else {
        m_toasts.clear();
    }
    if (!m_session.selectedItem().empty() && m_menu == Menu::None) {
        // подсказка — на тёмной плашке, чтобы читалась на любом фоне (светлая подложка темы, картинка)
        const std::string hint = tr(static_cast<int>(Ui::ItemSelected));
        const float w = textW(scaled(m_options.smallFont), hint), h = m_r->fontHeight(scaled(m_options.smallFont));
        const float x = 60, y = SCREEN_H - h - 12;
        m_r->fillRect({x - 14, y - 6, w + 28, h + 12}, {0, 0, 0, 190});
        m_r->drawText(scaled(m_options.smallFont), hint, REACTION, x, y, 1.0f);
    }
}

void GameScreen::renderMenu() {
    m_r->fillRect({0, 0, SCREEN_W, SCREEN_H}, {0, 0, 0, 160});
    std::vector<std::pair<std::string, bool>> items;  // текст, доступен
    std::string title;
    const bool saves = m_session.savesAvailable();
    auto T = [this](Ui k) { return tr(static_cast<int>(k)); };
    if (m_menu == Menu::Main) {
        title = T(Ui::Menu);
        for (MainItem item : mainItems()) {
            switch (item) {
                case MainItem::Save:
                    items.push_back({m_screen.canSave ? T(Ui::SaveGame) : T(Ui::SaveForbidden), saves && m_screen.canSave});
                    break;
                case MainItem::Load: items.push_back({T(Ui::LoadGame), saves}); break;
                case MainItem::Restart: items.push_back({T(Ui::Restart), true}); break;
                case MainItem::Sound:
                    items.push_back({m_options.soundAvailable ? (m_soundOn ? T(Ui::SoundOff) : T(Ui::SoundOn)) : T(Ui::SoundUnavailable),
                                     m_options.soundAvailable});
                    break;
                case MainItem::Choices:
                    items.push_back({m_choicesScreen ? (m_screen.layout == Layout::VisualNovel ? T(Ui::ChoicesScreen) : T(Ui::ChoicesScreenVnOnly))
                                                     : T(Ui::ChoicesInText), true});
                    break;
                case MainItem::Fullscreen:
                    items.push_back({m_isFullscreen && m_isFullscreen() ? T(Ui::FullscreenOff) : T(Ui::FullscreenOn), true});
                    break;
                case MainItem::Language:
                    items.push_back({T(Ui::Language) + ": " + languageName(m_session.language()), true});
                    break;
                case MainItem::TextSize:
                    items.push_back({T(Ui::TextSize) + ": " + std::to_string(static_cast<int>(std::lround(m_textScale * 100))) + "%", true});
                    break;
                case MainItem::Quit: items.push_back({T(Ui::Quit), true}); break;
                case MainItem::Back: items.push_back({T(Ui::BackToGame), true}); break;
            }
        }
    } else if (m_menu == Menu::Language) {
        // языки книги — каждый своим названием; текущий отмечен
        title = T(Ui::Language);
        items.push_back({T(Ui::Back), true});
        for (const auto& lang : m_session.availableLanguages()) {
            items.push_back({(lang == m_session.language() ? "• " : "") + languageName(lang), true});
        }
    } else if (m_menu == Menu::ConfirmQuit) {
        title = T(Ui::QuitQuestion);
        items = {{T(Ui::QuitYes), true}, {T(Ui::QuitNo), true}};
    } else if (m_menu == Menu::Save || m_menu == Menu::Load) {
        title = m_menu == Menu::Save ? T(Ui::SaveToSlot) : T(Ui::LoadFromSlot);
        items.push_back({T(Ui::Back), true});
        for (int slot = 1; slot <= 9; ++slot) {
            const auto info = m_session.saveInfo(slot);
            const std::string text = std::to_string(slot) + ". " + (info.exists ? info.time + " — " + info.label : T(Ui::EmptySlot));
            items.push_back({text, m_menu == Menu::Save || (info.exists && info.compatible)});
        }
    } else {
        const float lh = m_r->lineSkip(scaled(m_options.titleFont));
        std::istringstream lines(m_message);
        std::string line;
        float y = SCREEN_H / 2 - lh;
        while (std::getline(lines, line)) {
            const float w = textW(scaled(m_options.titleFont), line);
            m_r->drawText(scaled(m_options.titleFont), line, TEXT, (SCREEN_W - w) / 2, y, 1.0f);
            y += lh * 1.2f;
        }
        return;
    }
    // крупный текст: меню уменьшается ровно настолько, чтобы целиком поместиться на экране
    FontRef uiFont = scaled(m_options.uiFont), titleFont = scaled(m_options.titleFont);
    float lh = m_r->lineSkip(uiFont) * 1.6f;
    const float rows = static_cast<float>(items.size()) + 2;
    if (lh * rows > SCREEN_H * 0.95f) {
        const float k = SCREEN_H * 0.95f / (lh * rows);
        uiFont.size *= k;
        titleFont.size *= k;
        lh = m_r->lineSkip(uiFont) * 1.6f;
    }
    float y = (SCREEN_H - lh * rows) / 2;
    const float tw = textW(titleFont, title);
    m_r->drawText(titleFont, title, TEXT, (SCREEN_W - tw) / 2, y, 1.0f);
    y += lh * 1.5f;
    for (size_t i = 0; i < items.size(); ++i) {
        const float w = textW(uiFont, items[i].first);
        const float x = (SCREEN_W - w) / 2;
        m_r->drawText(uiFont, items[i].first, items[i].second ? LINK : DIM, x, y, 1.0f);
        // индексы: в главном меню — по порядку; в слотах «Назад» — 0, слоты — 1..9
        if (items[i].second) m_hits.push_back({{x - 20, y, w + 40, lh}, Hit::Kind::Menu, "", static_cast<int>(i)});
        y += lh;
    }
}

}  // namespace nlb::presentation
