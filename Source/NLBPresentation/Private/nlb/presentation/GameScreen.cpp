#include "nlb/presentation/GameScreen.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
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
        m_portion = 0;  // новая страница — текст с первой порции
    } else if (!m_screen.reactionText.empty() && flatten(before.reactionText) != flatten(m_screen.reactionText)) {
        // ответ на действие на той же странице — показать его (первая порция ответа)
        size_t pagePortions = 0;
        bool any = false;
        for (const auto& p : m_screen.pageText) {
            const bool blank = p.runs.empty() || std::all_of(p.runs.begin(), p.runs.end(), [](const TextRun& r) {
                return r.text.find_first_not_of(" \t") == std::string::npos; });
            if (blank) { if (any) { ++pagePortions; any = false; } } else any = true;
        }
        if (any) ++pagePortions;
        m_portion = pagePortions;
        m_scroll = 0;
    }
}

std::vector<std::vector<Paragraph>> GameScreen::portions() const {
    // paginator.delim = '\n[ \t]*\n': порция — абзацы до пустой строки; ответ на действие
    // продолжает порции страницы
    std::vector<std::vector<Paragraph>> out;
    std::vector<Paragraph> cur;
    auto add = [&](const std::vector<Paragraph>& text) {
        for (const auto& p : text) {
            const bool blank = p.runs.empty() || std::all_of(p.runs.begin(), p.runs.end(), [](const TextRun& r) {
                return r.text.find_first_not_of(" \t") == std::string::npos; });
            if (blank) {
                if (!cur.empty()) { out.push_back(cur); cur.clear(); }
            } else {
                cur.push_back(p);
            }
        }
        if (!cur.empty()) { out.push_back(cur); cur.clear(); }
    };
    add(m_screen.pageText);
    add(m_screen.reactionText);
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
    if (m_menu != Menu::None) return;  // время идёт, только пока игрок не в меню (как пауза в INSTEAD)
    const uint64_t revision = m_session.revision();
    m_session.advance(milliseconds);
    if (m_session.revision() != revision) pull();
}

void GameScreen::processEvents(const std::vector<PlayerEngine::Event>& events) {
    using Kind = PlayerEngine::Event::Kind;
    if (m_eventListener && !events.empty()) m_eventListener(events);
    for (const auto& e : events) {
        if (e.kind == Kind::Achievement) {
            m_toasts.push_back(e.text);
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
            m_message = "Внешняя игра «" + *m_screen.externalGame + "»: в этом проигрывателе её нет.\nЩелчок — вернуться в книгу.";
            m_menu = Menu::Message;
        }
    } else {
        m_externalNotified.clear();
        if (m_screen.finished) {
            m_message = "Конец игры.\nЩелчок — начать заново.";
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
    m_hoverObject.clear();
    for (auto it = m_hits.rbegin(); it != m_hits.rend(); ++it) {
        if (it->kind == Hit::Kind::Object && contains(it->rect, x, y)) {
            m_hoverObject = it->id;
            break;
        }
    }
}

void GameScreen::wheel(float lines) {
    // колесо: по 3 строки за щелчок (предел прокрутки — в renderTextWindow)
    if (m_menu == Menu::None && !m_showChoices) m_scroll = std::max(0.0f, m_scroll - lines * 3 * m_lineHeight);
}

void GameScreen::click(float x, float y, bool rightButton) {
    if (m_menu == Menu::Message) {
        m_menu = Menu::None;
        if (m_screen.externalGame) m_session.finishExternalGame();
        else if (m_screen.finished) m_session.restart();
        afterCommand();
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
        m_scroll += m_pageHeight;
        return;
    } else if (!hitSomething && m_menu == Menu::None && advanceToChoices()) {
        return;
    }
    for (auto it = m_hits.rbegin(); it != m_hits.rend(); ++it) {
        if (!contains(it->rect, x, y)) continue;
        const Hit hit = *it;
        if (m_menu != Menu::None) {
            if (hit.kind == Hit::Kind::Menu) menuAction(hit.index);
            return;
        }
        if (m_showChoices && hit.kind != Hit::Kind::Choice && hit.kind != Hit::Kind::MenuButton) {
            continue;
        }
        switch (hit.kind) {
            case Hit::Kind::ScrollUp: m_scroll = std::max(0.0f, m_scroll - m_pageHeight); return;
            case Hit::Kind::ScrollDown: m_scroll += m_pageHeight; return;
            case Hit::Kind::MenuButton: m_menu = Menu::Main; return;
            case Hit::Kind::Choice:
                m_session.choose(hit.index);
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
        else if (m_menu == Menu::ConfirmQuit) m_menu = Menu::Main;
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
        else if (m_moreText) m_scroll += m_pageHeight;
        else if (m_showChoices) m_showChoices = false;
        else advanceToChoices();
    } else if (k == Key::PageDown) {
        m_scroll += m_pageHeight;
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
        m_scroll += m_pageHeight;
        return true;
    }
    if (m_morePortions) {                   // следующая порция
        ++m_portion;
        m_scroll = 0;
        return true;
    }
    // текст дочитан: единственный тривиальный вариант («Далее») — сразу переход, иначе варианты
    const int trivial = singleTrivialChoice();
    if (trivial >= 0) {
        m_session.choose(trivial);
        afterCommand();
        return true;
    }
    return advanceToChoices();
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
        switch (index) {
            case 0: if (saves && m_screen.canSave) m_menu = Menu::Save; break;
            case 1: if (saves) m_menu = Menu::Load; break;
            case 2: m_session.restart(); m_menu = Menu::None; afterCommand(); break;
            case 3:
                if (m_options.soundAvailable) { m_soundOn = !m_soundOn; m_session.setMuted(!m_soundOn); }
                break;
            case 4:
                m_choicesScreen = !m_choicesScreen;
                m_showChoices = false;
                break;
            case 5: m_menu = Menu::ConfirmQuit; break;  // сначала спросить
            default: m_menu = Menu::None; break;
        }
    } else if (m_menu == Menu::ConfirmQuit) {
        if (index == 0) { m_quit = true; m_menu = Menu::None; }  // «Да, выйти»
        else m_menu = Menu::Main;                                 // «Нет» — обратно в меню
    } else if (m_menu == Menu::Save || m_menu == Menu::Load) {
        if (index < 1 || index > 9) { m_menu = Menu::Main; return; }
        const bool ok = m_menu == Menu::Save ? m_session.save(index, error) : m_session.load(index, error);
        m_menu = Menu::None;
        if (!ok) { m_message = "Не удалось: " + error; m_menu = Menu::Message; return; }
        afterCommand();
    }
}

// ============================================================================ рисование

void GameScreen::render(IScreenRenderer& r) {
    m_r = &r;
    m_r->fillRect({0, 0, SCREEN_W, SCREEN_H}, BACKGROUND);
    m_hits.clear();
    if (m_screen.layout == Layout::VisualNovel) renderVisualNovel();
    else renderStandard();
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
                                   const FontRef& font, const ScreenTheme::Window& colors, bool draw) {
    const float lineHeight = m_r->lineSkip(font) * colors.lineHeight;
    const ScreenTheme& theme = m_screen.layout == Layout::VisualNovel ? m_vnTheme : m_standardTheme;
    struct Word { std::string text; float w; bool link; std::string target; };
    // Строка выводится целиком: с выравниванием (win.align; по умолчанию INSTEAD — по ширине)
    auto flush = [&](std::vector<Word>& line, bool lastInParagraph) {
        if (line.empty()) return;
        // хвостовой пробел последнего слова в ширину строки не входит
        float total = 0;
        for (auto& w : line) total += w.w;
        const float trailing = textW(font, " ") * (line.back().text.back() == ' ' ? 1 : 0);
        total -= trailing;
        float cx = x, gap = 0;
        if (theme.align == "center") cx = x + (width - total) / 2;
        else if (theme.align == "right") cx = x + width - total;
        else if (theme.align == "justify" && !lastInParagraph && line.size() > 1) gap = (width - total) / static_cast<float>(line.size() - 1);
        const bool visible = y >= m_drawTop - 0.5f && y + lineHeight <= m_drawBottom + 0.5f;
        if (draw && !visible && y + lineHeight > m_drawBottom + 0.5f && m_firstHiddenY < 0) m_firstHiddenY = y;
        for (auto& w : line) {
            if (draw && visible) {
                const Color c = w.link ? (m_hoverObject == w.target ? colors.alink : colors.link) : colors.fg;
                m_r->drawText(font, w.text, c, cx, y, 1.0f);
                if (w.link) m_hits.push_back({{cx, y, w.w, lineHeight}, Hit::Kind::Object, w.target});
            }
            cx += w.w + gap;
        }
        line.clear();
        y += lineHeight;
    };
    for (const auto& p : text) {
        std::vector<Word> line;
        float lineW = 0;
        bool any = false;
        for (const auto& run : p.runs) {
            const bool link = run.kind == TextRun::Kind::ObjectLink;
            for (const auto& w : words(run.text)) {
                const float ww = textW(font, w);
                if (lineW + ww > width && !line.empty()) {  // перенос
                    flush(line, false);
                    lineW = 0;
                }
                line.push_back({w, ww, link, run.target});
                lineW += ww;
                any = true;
            }
        }
        if (any) flush(line, true);
        else y += lineHeight * 0.5f;
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
            m_r->drawText(font, text, c.enabled ? colors.link : colors.fg, x, y, 1.0f);
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

void GameScreen::renderTextWindow(const ScreenTheme::Window& window, bool panel) {
    const FontRef font = fontFor(window);
    const RectF r = window.rect;
    // Режим экрана вариантов: текст порциями (как paginator), без стрелок прокрутки
    std::vector<Paragraph> portionText;
    if (choicesMode()) {
        const auto parts = portions();
        if (m_portion >= parts.size()) m_portion = parts.empty() ? 0 : parts.size() - 1;
        if (!parts.empty()) portionText = parts[m_portion];
        m_morePortions = m_portion + 1 < parts.size();
    } else {
        m_morePortions = false;
    }
    const std::vector<Paragraph>& firstText = choicesMode() ? portionText : m_screen.pageText;
    static const std::vector<Paragraph> NONE;
    const std::vector<Paragraph>& secondText = choicesMode() ? NONE : m_screen.reactionText;
    // Сколько места занимает текст (для прокрутки)
    float need = 0;
    if (!m_screen.caption.empty()) need += m_r->lineSkip(font) * 1.4f;
    need = renderParagraphs(firstText, r.x, need, r.w, font, window, false);
    need = renderParagraphs(secondText, r.x, need, r.w, font, window, false);
    if (!choicesMode()) need = renderChoices(r.x, need, r.w, font, window, false);
    m_contentHeight = need;
    // Окно показывает целое число строк; не уместившийся текст листается страницами, как
    // paginator в INSTEAD: щелчок по окну (или колесо, PgDn) — следующая часть
    const float lh = m_r->lineSkip(font) * window.lineHeight;
    m_lineHeight = lh;
    const float pageH = std::max(lh, std::floor(r.h / lh) * lh);
    const float maxScroll = std::max(0.0f, need - pageH);
    m_scroll = std::min(m_scroll, maxScroll);
    m_textWindow = r;
    m_pageHeight = pageH;
    m_moreText = m_scroll < maxScroll;
    if (panel) m_r->fillRect({r.x - 16, r.y - 10, r.w + 32, r.h + 20}, {0, 0, 0, 140});
    const RectF clip{std::trunc(r.x - 8), std::trunc(r.y), std::trunc(r.w + 16), std::trunc(r.h)};
    m_r->setClip(&clip);
    const size_t firstHit = m_hits.size();
    float y = r.y - m_scroll;
    m_drawTop = r.y;
    m_drawBottom = r.y + r.h;
    m_firstHiddenY = -1;
    if (!m_screen.caption.empty()) {
        m_r->drawText(font, m_screen.caption, window.fg, r.x, y, 1.0f);
        y += m_r->lineSkip(font) * 1.4f;
    }
    y = renderParagraphs(firstText, r.x, y, r.w, font, window, true);
    y = renderParagraphs(secondText, r.x, y, r.w, font, window, true);
    if (!choicesMode()) renderChoices(r.x, y, r.w, font, window, true);
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
        const RectF ar{r.x + r.w + 6, up ? ay : ay - ah, aw, ah};
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
    m_r->fillRect({0, 0, SCREEN_W, SCREEN_H}, t.background);
    float w = 0, h = 0;
    if (!t.backgroundImage.empty() && m_r->imageSize(t.backgroundImage, w, h)) {
        m_r->drawImage(t.backgroundImage, t.screenArea, 1.0f);  // фон темы — на её экран (scr.w×scr.h)
    }
    if (imageSize(m_screen.pageImage, w, h)) {
        RectF dst;
        if (m_screen.imageIsBackground) {
            dst = {0, 0, SCREEN_W, SCREEN_H};
        } else {
            // scr.gfx: картинка страницы в своей области, вписана и по центру (gfx.mode = float)
            const RectF a = t.pictureArea;
            const float k = std::min(a.w / w, a.h / h);
            dst = {a.x + (a.w - w * k) / 2, a.y + (a.h - h * k) / 2, w * k, h * k};
        }
        m_r->drawImage(framePath(m_screen.pageImage), dst, 1.0f);
    }
    renderSceneObjects(t, false, false);
    if (m_showChoices) {
        renderChoicesScreen();
        renderMenuButton(t);
        return;
    }
    renderTextWindow(t.text, m_screen.imageIsBackground);
    renderInventory(t.inventory, false);
    renderMenuButton(t);
}

void GameScreen::renderVisualNovel() {
    const ScreenTheme& t = m_vnTheme;
    m_r->fillRect({0, 0, SCREEN_W, SCREEN_H}, t.background);
    float w = 0, h = 0;
    if (imageSize(m_screen.pageImage, w, h)) m_r->drawImage(framePath(m_screen.pageImage), {0, 0, SCREEN_W, SCREEN_H}, 1.0f);
    // На экране вариантов объектов нет — как в INSTEAD, где на время выбора остаются только фон и кнопка меню
    renderSceneObjects(t, true, m_showChoices);
    const bool hasText = !m_screen.pageText.empty() || !m_screen.reactionText.empty() || !m_screen.choices.empty();
    if (m_showChoices) {
        renderChoicesScreen();
    } else if (hasText) {
        // Каждая VN-страница экспорта вызывает vn:geom(8, 864, 1904, 184, 'dissolve', 240): окно
        // текста — полоса почти во всю ширину, а текст — внутри неё с полями по 240 px.
        // Полоса — полупрозрачная, как на экране INSTEAD (рамки fl/fr там не видны)
        const RectF band{8, 864, 1904, 184};
        const float wf = 240, pad = 8;  // vn.lua: textpad = 8
        m_r->fillRect({0, band.y, SCREEN_W, band.h + 16}, {0, 0, 0, 110});
        ScreenTheme::Window win = t.text;
        win.rect = {band.x + wf, band.y + pad, band.w - 2 * wf, band.h - 2 * pad};
        renderTextWindow(win, false);
    }
    renderMenuButton(t);
    renderInventory(t.inventory, false);
}

void GameScreen::renderInventory(const ScreenTheme::Window& window, bool panel) {
    if (m_screen.inventory.empty()) return;
    const FontRef font = fontFor(window);
    const RectF r = window.rect;
    if (panel) m_r->fillRect({r.x, r.y, r.w, r.h}, {0, 0, 0, 140});
    const float lineHeight = m_r->lineSkip(font) * 1.15f;
    const float gap = textW(font, "    ");
    const std::string& selected = m_session.selectedItem();
    struct Item { const InventoryItem* item; std::string label; float w; };
    std::vector<Item> items;
    for (const auto& item : m_screen.inventory) {
        std::string label = item.label;
        std::replace(label.begin(), label.end(), '\n', ' ');
        float iw = 0, ih = 0;
        float w = textW(font, label) + (imageSize(item.icon, iw, ih) ? lineHeight + 6 : 0);
        items.push_back({&item, label, w});
    }
    // inv.mode = horizontal-center: строки предметов по центру области; vertical — столбиком
    std::vector<std::vector<Item>> rows(1);
    float rowW = 0;
    for (const auto& it : items) {
        if (m_standardTheme.inventoryHorizontal && !rows.back().empty() && rowW + gap + it.w > r.w) { rows.emplace_back(); rowW = 0; }
        if (!m_standardTheme.inventoryHorizontal && !rows.back().empty()) { rows.emplace_back(); rowW = 0; }
        rowW += (rows.back().empty() ? 0 : gap) + it.w;
        rows.back().push_back(it);
    }
    float y = r.y;
    for (const auto& row : rows) {
        float total = 0;
        for (size_t i = 0; i < row.size(); ++i) total += row[i].w + (i ? gap : 0);
        float x = m_standardTheme.inventoryHorizontal ? r.x + (r.w - total) / 2 : r.x;
        for (const auto& it : row) {
            const RectF cell{x, y, it.w, lineHeight};
            if (it.item->id == selected) m_r->fillRect({cell.x - 6, cell.y, cell.w + 12, cell.h}, {255, 255, 255, 60});
            float tx = x, iw = 0, ih = 0;
            if (imageSize(it.item->icon, iw, ih)) {
                m_r->drawImage(framePath(it.item->icon), {x, y, lineHeight, lineHeight}, 1.0f);
                tx += lineHeight + 6;
            }
            const bool isSelected = it.item->id == selected;
            m_r->drawText(font, it.label, !it.item->active ? window.fg : isSelected ? window.alink : window.link, tx, y, 1.0f);
            if (it.item->active) m_hits.push_back({cell, Hit::Kind::Inventory, it.item->id});
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
    // menu.gfx.button в menu.button.x/y — как кнопка «Меню» в INSTEAD.
    // Языковой вариант картинки, если есть: menubtn.png -> menubtn_ru.png для русской книги
    std::string path = theme.menuButton;
    if (path.empty()) return;
    const auto dot = path.find_last_of('.');
    const std::string localized = dot == std::string::npos ? "" : path.substr(0, dot) + "_" + m_session.language() + path.substr(dot);
    if (!localized.empty() && FileUtils::exists(localized)) path = localized;
    float w = 0, h = 0;
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
    const FontRef font{window.fontFile, window.fontSize * 1.25f};
    const float lh = m_r->lineSkip(font);
    const float step = lh * 1.9f;
    const float panelW = SCREEN_W * 0.675f;
    const float panelH = std::max(SCREEN_H * 0.46f, step * static_cast<float>(m_screen.choices.size()) + lh);
    const RectF panel{(SCREEN_W - panelW) / 2, std::max(20.0f, (SCREEN_H - panelH) / 2 - 80), panelW, panelH};
    m_r->fillRect(panel, {0, 0, 0, 120});
    float y = panel.y + lh * 0.5f;
    int number = 0;
    for (const auto& c : m_screen.choices) {
        const std::string text = c.enabled ? std::to_string(++number) + ". " + c.text : c.altText;
        const RectF r{panel.x + 10, y, std::min(textW(font, text), panel.w - 20), lh};
        const bool hover = c.enabled && contains(r, m_mouseX, m_mouseY);
        m_r->drawText(font, text, !c.enabled ? window.fg : hover ? window.alink : window.link, r.x, r.y, 1.0f);
        if (c.enabled) m_hits.push_back({r, Hit::Kind::Choice, "", c.index});
        y += step;
    }
}

void GameScreen::renderOverlayMessages() {
    if (!m_toasts.empty() && now() < m_toastUntil) {
        float y = 30;
        for (const auto& t : m_toasts) {
            const float w = textW(m_options.uiFont, t), h = m_r->fontHeight(m_options.uiFont);
            m_r->fillRect({SCREEN_W - w - 70, y - 10, w + 40, h + 20}, SELECTED);
            m_r->drawText(m_options.uiFont, t, TEXT, SCREEN_W - w - 50, y, 1.0f);
            y += h + 30;
        }
    } else {
        m_toasts.clear();
    }
    if (!m_session.selectedItem().empty() && m_menu == Menu::None) {
        m_r->drawText(m_options.smallFont, "Выбран предмет: щёлкните, к чему применить (правая кнопка — отмена)", REACTION, 60, SCREEN_H - 36, 1.0f);
    }
}

void GameScreen::renderMenu() {
    m_r->fillRect({0, 0, SCREEN_W, SCREEN_H}, {0, 0, 0, 160});
    std::vector<std::pair<std::string, bool>> items;  // текст, доступен
    std::string title;
    const bool saves = m_session.savesAvailable();
    if (m_menu == Menu::Main) {
        title = "Меню";
        items = {{m_screen.canSave ? "Сохранить игру (F5)" : "Сохранить игру — здесь нельзя", saves && m_screen.canSave},
                 {"Загрузить игру (F9)", saves},
                 {"Начать заново", true},
                 {m_options.soundAvailable ? (m_soundOn ? "Выключить звук" : "Включить звук") : "Звук недоступен", m_options.soundAvailable},
                 {m_choicesScreen ? (m_screen.layout == Layout::VisualNovel ? "Варианты: отдельным экраном"
                                                                             : "Варианты: отдельным экраном (только в VN)")
                                  : "Варианты: в окне текста", true},
                 {"Выйти", true},
                 {"Вернуться в игру (Esc)", true}};
    } else if (m_menu == Menu::ConfirmQuit) {
        title = "Выйти из игры?";
        items = {{"Да, выйти", true}, {"Нет, вернуться (Esc)", true}};
    } else if (m_menu == Menu::Save || m_menu == Menu::Load) {
        title = m_menu == Menu::Save ? "Сохранить в слот" : "Загрузить из слота";
        items.push_back({"Назад", true});
        for (int slot = 1; slot <= 9; ++slot) {
            const auto info = m_session.saveInfo(slot);
            const std::string text = std::to_string(slot) + ". " + (info.exists ? info.time + " — " + info.label : "пусто");
            items.push_back({text, m_menu == Menu::Save || (info.exists && info.compatible)});
        }
    } else {
        const float lh = m_r->lineSkip(m_options.titleFont);
        std::istringstream lines(m_message);
        std::string line;
        float y = SCREEN_H / 2 - lh;
        while (std::getline(lines, line)) {
            const float w = textW(m_options.titleFont, line);
            m_r->drawText(m_options.titleFont, line, TEXT, (SCREEN_W - w) / 2, y, 1.0f);
            y += lh * 1.2f;
        }
        return;
    }
    const float lh = m_r->lineSkip(m_options.uiFont) * 1.6f;
    float y = (SCREEN_H - lh * (static_cast<float>(items.size()) + 2)) / 2;
    const float tw = textW(m_options.titleFont, title);
    m_r->drawText(m_options.titleFont, title, TEXT, (SCREEN_W - tw) / 2, y, 1.0f);
    y += lh * 1.5f;
    for (size_t i = 0; i < items.size(); ++i) {
        const float w = textW(m_options.uiFont, items[i].first);
        const float x = (SCREEN_W - w) / 2;
        m_r->drawText(m_options.uiFont, items[i].first, items[i].second ? LINK : DIM, x, y, 1.0f);
        // индексы: в главном меню — по порядку; в слотах «Назад» — 0, слоты — 1..9
        if (items[i].second) m_hits.push_back({{x - 20, y, w + 40, lh}, Hit::Kind::Menu, "", static_cast<int>(i)});
        y += lh;
    }
}

}  // namespace nlb::presentation
