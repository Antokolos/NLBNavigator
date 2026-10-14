#include "SdlPlayer.h"

#include <SDL3_image/SDL_image.h>

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>

#include "nlb/api/DummyProgressData.h"
#include "nlb/api/Page.h"
#include "nlb/audio/SdlAudioBackend.h"
#include "nlb/domain/NonLinearBookImpl.h"
#include "nlb/presentation/AudioDirector.h"
#include "nlb/presentation/BookMediaSource.h"
#include "nlb/util/FileUtils.h"

using namespace nlb::presentation;

namespace {
const float SCREEN_W = 1920.0f, SCREEN_H = 1080.0f;
const SDL_Color TEXT = {235, 230, 220, 255};
const SDL_Color LINK = {130, 190, 255, 255};
const SDL_Color LINK_HOVER = {190, 225, 255, 255};
const SDL_Color REACTION = {240, 200, 120, 255};
const SDL_Color DIM = {150, 150, 150, 255};
const SDL_Color PANEL = {20, 20, 28, 210};
const SDL_Color BACKGROUND = {12, 12, 16, 255};
const SDL_Color SELECTED = {90, 70, 30, 255};

bool contains(const SDL_FRect& r, float x, float y) {
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

std::string framePath(const ImageRef& image) {
    if (image.framePattern.empty()) return image.file.path;
    char buf[1024];
    std::snprintf(buf, sizeof(buf), image.framePattern.c_str(), std::max(1, image.currentFrame));
    return buf;
}

/// Шрифт по умолчанию: свой каталог fonts/, затем системные шрифты с кириллицей
std::string findFont(const std::string& bookPath) {
    std::vector<std::string> candidates;
    const char* base = SDL_GetBasePath();
    if (base) {
        candidates.push_back(std::string(base) + "fonts/DejaVuSans.ttf");
        candidates.push_back(std::string(base) + "fonts/default.ttf");
    }
    candidates.push_back(FileUtils::combinePath(bookPath, "fonts/default.ttf"));
#ifdef _WIN32
    candidates.push_back("C:/Windows/Fonts/segoeui.ttf");
    candidates.push_back("C:/Windows/Fonts/arial.ttf");
#endif
    candidates.push_back("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
    candidates.push_back("/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf");
    candidates.push_back("/System/Library/Fonts/Supplemental/Arial.ttf");
    for (const auto& c : candidates) if (FileUtils::exists(c)) return c;
    return "";
}
}

SdlPlayer::SdlPlayer(Options options) : m_options(std::move(options)) {}

SdlPlayer::~SdlPlayer() {
    shutdown();
}

bool SdlPlayer::init() {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        std::cerr << "SDL_Init: " << SDL_GetError() << std::endl;
        return false;
    }
    if (!TTF_Init()) {
        std::cerr << "TTF_Init: " << SDL_GetError() << std::endl;
        return false;
    }
    m_book = std::make_unique<NonLinearBookImpl>();
    DummyProgressData progress;
    if (!m_book->load(m_options.bookPath, progress)) {
        std::cerr << "Cannot load book: " << m_options.bookPath << std::endl;
        return false;
    }
    const std::string title = "NLBNavigator — " + m_book->getTitle();
    if (!SDL_CreateWindowAndRenderer(title.c_str(), m_options.windowWidth, m_options.windowHeight,
                                     SDL_WINDOW_RESIZABLE, &m_window, &m_renderer)) {
        std::cerr << "SDL_CreateWindowAndRenderer: " << SDL_GetError() << std::endl;
        return false;
    }
    // Экран экспорта — 1920×1080; в окно он вписывается с полями (letterbox), как в теме INSTEAD
    SDL_SetRenderLogicalPresentation(m_renderer, static_cast<int>(SCREEN_W), static_cast<int>(SCREEN_H),
                                     SDL_LOGICAL_PRESENTATION_LETTERBOX);
    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderVSync(m_renderer, 1);

    const std::string fontPath = m_options.fontPath.empty() ? findFont(m_options.bookPath) : m_options.fontPath;
    m_fontPath = fontPath;
    m_standardTheme = ScreenTheme::load(m_options.themeDir, false);
    m_vnTheme = ScreenTheme::load(m_options.themeDir, true);
    m_font = fontPath.empty() ? nullptr : TTF_OpenFont(fontPath.c_str(), 32);
    m_titleFont = fontPath.empty() ? nullptr : TTF_OpenFont(fontPath.c_str(), 46);
    m_smallFont = fontPath.empty() ? nullptr : TTF_OpenFont(fontPath.c_str(), 26);
    if (!m_font) {
        std::cerr << "Нет шрифта: укажите --font <файл .ttf>" << std::endl;
        return false;
    }

    m_media = std::make_unique<BookMediaSource>(m_options.bookPath);
    if (m_options.sound && !m_book->isSuppressSound()) {
        m_audio = std::make_unique<nlb::audio::SdlAudioBackend>(*m_media);
        if (m_audio->isAvailable()) {
            m_director = std::make_unique<AudioDirector>(*m_audio);
        } else {
            std::cerr << "Warning: sound is not available: " << m_audio->lastError() << std::endl;
        }
    }
    m_soundOn = m_director != nullptr;

    PlayerEngine::Settings settings;
    settings.exportMode = m_options.exportMode;
    settings.realTime = true;
    settings.achievementsPath = m_options.achievementsPath;
    m_engine = std::make_unique<PlayerEngine>(m_book.get(), settings);
    m_engine->start();
    refresh();
    return true;
}

void SdlPlayer::shutdown() {
    for (auto& [k, t] : m_images) if (t) SDL_DestroyTexture(t);
    for (auto& [k, t] : m_texts) if (t) SDL_DestroyTexture(t);
    m_images.clear();
    m_texts.clear();
    m_director.reset();
    m_audio.reset();
    for (auto& [k, f] : m_fonts) if (f) TTF_CloseFont(f);
    m_fonts.clear();
    if (m_font) TTF_CloseFont(m_font);
    if (m_titleFont) TTF_CloseFont(m_titleFont);
    if (m_smallFont) TTF_CloseFont(m_smallFont);
    m_font = m_titleFont = m_smallFont = nullptr;
    if (m_renderer) SDL_DestroyRenderer(m_renderer);
    if (m_window) SDL_DestroyWindow(m_window);
    m_renderer = nullptr;
    m_window = nullptr;
    if (TTF_WasInit()) TTF_Quit();
    SDL_Quit();
}

int SdlPlayer::run() {
    if (!init()) return 1;
    if (!m_options.scriptPath.empty()) {
        return runScript() ? 0 : 2;
    }
    loop();
    return 0;
}

// ============================================================================ цикл

void SdlPlayer::loop() {
    Uint64 last = SDL_GetTicks();
    while (!m_quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            SDL_ConvertEventToRenderCoordinates(m_renderer, &e);
            switch (e.type) {
                case SDL_EVENT_QUIT: m_quit = true; break;
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    click(e.button.x, e.button.y, e.button.button == SDL_BUTTON_RIGHT);
                    break;
                case SDL_EVENT_MOUSE_MOTION: {
                    m_hoverObject.clear();
                    for (auto it = m_hits.rbegin(); it != m_hits.rend(); ++it) {
                        if (it->kind == Hit::Kind::Object && contains(it->rect, e.motion.x, e.motion.y)) {
                            m_hoverObject = it->id;
                            break;
                        }
                    }
                    break;
                }
                case SDL_EVENT_MOUSE_WHEEL:
                    m_scroll = std::max(0.0f, m_scroll - e.wheel.y * 60.0f);  // предел — в renderTextWindow
                    break;
                case SDL_EVENT_KEY_DOWN: key(e.key.key); break;
                default: break;
            }
        }
        const Uint64 now = SDL_GetTicks();
        const int dt = static_cast<int>(now - last);
        last = now;
        // Время идёт, только пока игрок не в меню (как пауза в INSTEAD)
        if (m_menu == Menu::None && m_engine->advanceTime(dt) > 0) refresh();
        render();
        SDL_RenderPresent(m_renderer);
        if (SDL_GetWindowFlags(m_window) & SDL_WINDOW_MINIMIZED) SDL_Delay(50);
    }
}

void SdlPlayer::refresh() {
    processEvents(m_engine->takeEvents());
    m_screen = m_engine->screen();
}

void SdlPlayer::processEvents(const std::vector<PlayerEngine::Event>& events) {
    using Kind = PlayerEngine::Event::Kind;
    if (m_director) m_director->apply(events);
    for (const auto& e : events) {
        if (e.kind == Kind::Achievement) {
            m_toasts.push_back(e.text);
            m_toastUntil = SDL_GetTicks() + 4000;
        } else if (e.kind == Kind::PageText || e.kind == Kind::PageCaption) {
            m_scroll = 0;  // новая страница — с начала
        }
    }
}

void SdlPlayer::afterCommand() {
    refresh();
    if (m_screen.externalGame) {
        m_message = "Внешняя игра «" + *m_screen.externalGame + "»: в этом проигрывателе её нет.\nЩелчок — вернуться в книгу.";
        m_menu = Menu::Message;
    } else if (m_screen.finished) {
        m_message = "Конец игры.\nЩелчок — начать заново.";
        m_menu = Menu::Message;
    }
}

// ============================================================================ ввод

void SdlPlayer::click(float x, float y, bool rightButton) {
    if (m_menu == Menu::Message) {
        m_menu = Menu::None;
        if (m_screen.externalGame) m_engine->finishExternalGame();
        else if (m_screen.finished) m_engine->restart();
        afterCommand();
        return;
    }
    if (rightButton) {
        m_selectedItem.clear();
        return;
    }
    bool hitSomething = false;
    for (const auto& h : m_hits) hitSomething = hitSomething || contains(h.rect, x, y);
    if (!hitSomething && m_menu == Menu::None && m_moreText && contains(m_textWindow, x, y)) {
        m_scroll += m_pageHeight;
        return;
    }
    for (auto it = m_hits.rbegin(); it != m_hits.rend(); ++it) {
        if (!contains(it->rect, x, y)) continue;
        const Hit hit = *it;
        if (m_menu != Menu::None) {
            if (hit.kind == Hit::Kind::Menu) menuAction(hit.index);
            return;
        }
        switch (hit.kind) {
            case Hit::Kind::Choice:
                m_selectedItem.clear();
                m_engine->choose(static_cast<size_t>(hit.index));
                break;
            case Hit::Kind::Object:
                // Выбран предмет — применить его к объекту (как в INSTEAD), иначе действие с объектом
                if (!m_selectedItem.empty()) {
                    m_engine->use(m_selectedItem, hit.id);
                    m_selectedItem.clear();
                } else {
                    m_engine->actOnObject(hit.id);
                }
                break;
            case Hit::Kind::Inventory:
                if (m_selectedItem == hit.id) {          // второй щелчок по тому же — осмотр (inv)
                    m_selectedItem.clear();
                    m_engine->clickInventory(hit.id);
                } else if (!m_selectedItem.empty()) {    // предмет к предмету
                    m_engine->use(m_selectedItem, hit.id);
                    m_selectedItem.clear();
                } else {
                    m_selectedItem = hit.id;
                    return;
                }
                break;
            case Hit::Kind::Menu: break;
        }
        afterCommand();
        return;
    }
}

void SdlPlayer::key(SDL_Keycode keycode) {
    if (keycode == SDLK_ESCAPE) {
        if (m_menu == Menu::None) openMenu(Menu::Main);
        else if (m_menu != Menu::Message) m_menu = Menu::None;
        return;
    }
    if (m_menu != Menu::None) return;
    if (keycode >= SDLK_1 && keycode <= SDLK_9) {
        // Цифры — варианты по порядку
        int n = static_cast<int>(keycode - SDLK_1), k = 0;
        for (const auto& c : m_screen.choices) {
            if (!c.enabled) continue;
            if (k++ == n) { m_engine->choose(static_cast<size_t>(c.index)); afterCommand(); return; }
        }
    } else if (keycode == SDLK_PAGEDOWN || keycode == SDLK_SPACE) {
        m_scroll += m_pageHeight;
    } else if (keycode == SDLK_PAGEUP) {
        m_scroll = std::max(0.0f, m_scroll - m_pageHeight);
    } else if (keycode == SDLK_F5) {
        openMenu(Menu::Save);
    } else if (keycode == SDLK_F9) {
        openMenu(Menu::Load);
    }
}

// ============================================================================ меню

void SdlPlayer::openMenu(Menu menu) {
    m_menu = menu;
}

std::string SdlPlayer::slotPath(int slot) const {
    return m_options.savesDir + "/slot" + std::to_string(slot) + ".sav";
}

void SdlPlayer::menuAction(int index) {
    std::string error;
    if (m_menu == Menu::Main) {
        switch (index) {
            case 0: if (!m_options.savesDir.empty() && m_engine->canSave()) m_menu = Menu::Save; break;
            case 1: if (!m_options.savesDir.empty()) m_menu = Menu::Load; break;
            case 2: m_engine->restart(); m_menu = Menu::None; afterCommand(); break;
            case 3:
                if (m_director) { m_soundOn = !m_soundOn; m_director->setMuted(!m_soundOn); }
                break;
            case 4: m_quit = true; break;
            default: m_menu = Menu::None; break;
        }
    } else if (m_menu == Menu::Save || m_menu == Menu::Load) {
        if (index < 1 || index > 9) { m_menu = Menu::Main; return; }
        const bool ok = m_menu == Menu::Save ? m_engine->saveGame(slotPath(index), error)
                                             : m_engine->loadGame(slotPath(index), error);
        m_menu = Menu::None;
        if (!ok) { m_message = "Не удалось: " + error; m_menu = Menu::Message; return; }
        afterCommand();
    }
}

// ============================================================================ рисование

void SdlPlayer::fillRect(const SDL_FRect& r, SDL_Color c) {
    SDL_SetRenderDrawColor(m_renderer, c.r, c.g, c.b, c.a);
    SDL_RenderFillRect(m_renderer, &r);
}

SDL_Texture* SdlPlayer::textTexture(const std::string& text, TTF_Font* font, SDL_Color color) {
    if (text.empty()) return nullptr;
    const std::string key = std::to_string(reinterpret_cast<uintptr_t>(font)) + "|" + std::to_string(color.r) + ","
        + std::to_string(color.g) + "," + std::to_string(color.b) + "|" + text;
    auto it = m_texts.find(key);
    if (it != m_texts.end()) return it->second;
    if (m_texts.size() > 4000) {  // кэш надписей не растёт бесконечно
        for (auto& [k, t] : m_texts) SDL_DestroyTexture(t);
        m_texts.clear();
    }
    SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), 0, color);
    SDL_Texture* tex = surface ? SDL_CreateTextureFromSurface(m_renderer, surface) : nullptr;
    if (surface) SDL_DestroySurface(surface);
    m_texts[key] = tex;
    return tex;
}

SDL_FPoint SdlPlayer::textSize(const std::string& text, TTF_Font* font) {
    int w = 0, h = 0;
    if (!text.empty()) TTF_GetStringSize(font, text.c_str(), 0, &w, &h);
    if (h == 0) h = TTF_GetFontHeight(font);
    return {static_cast<float>(w), static_cast<float>(h)};
}

void SdlPlayer::drawText(const std::string& text, TTF_Font* font, SDL_Color color, float x, float y) {
    SDL_Texture* tex = textTexture(text, font, color);
    if (!tex) return;
    float w, h;
    SDL_GetTextureSize(tex, &w, &h);
    SDL_FRect dst{x, y, w, h};
    SDL_RenderTexture(m_renderer, tex, nullptr, &dst);
}

SDL_Texture* SdlPlayer::textureFor(const std::string& path) {
    if (path.empty()) return nullptr;
    auto it = m_images.find(path);
    if (it != m_images.end()) return it->second;
    SDL_Texture* tex = nullptr;
    const std::string file = m_media->filePath(MediaRef{path});
    if (!file.empty()) {
        tex = IMG_LoadTexture(m_renderer, file.c_str());
    } else {
        std::vector<uint8_t> bytes;
        if (m_media->read(MediaRef{path}, bytes) && !bytes.empty()) {
            SDL_IOStream* io = SDL_IOFromConstMem(bytes.data(), bytes.size());
            if (io) tex = IMG_LoadTexture_IO(m_renderer, io, true);
        }
    }
    if (m_images.size() > 600) {  // анимации из сотен кадров: держим в памяти не всё
        for (auto& [k, t] : m_images) if (t) SDL_DestroyTexture(t);
        m_images.clear();
    }
    m_images[path] = tex;  // в том числе nullptr: отсутствующий файл не ищем каждый кадр
    return tex;
}

SDL_Texture* SdlPlayer::texture(const ImageRef& image) {
    return image.empty() ? nullptr : textureFor(framePath(image));
}

TTF_Font* SdlPlayer::fontFor(const ScreenTheme::Window& window) {
    const std::string file = (!window.fontFile.empty() && FileUtils::exists(window.fontFile)) ? window.fontFile : m_fontPath;
    const std::string key = file + "|" + std::to_string(window.fontSize);
    auto it = m_fonts.find(key);
    if (it != m_fonts.end()) return it->second;
    TTF_Font* f = TTF_OpenFont(file.c_str(), window.fontSize);
    if (!f && file != m_fontPath) f = TTF_OpenFont(m_fontPath.c_str(), window.fontSize);
    m_fonts[key] = f ? f : m_font;
    return m_fonts[key];
}

float SdlPlayer::renderParagraphs(const std::vector<Paragraph>& text, float x, float y, float width,
                                  TTF_Font* font, const ScreenTheme::Window& colors, bool draw) {
    const float lineHeight = static_cast<float>(TTF_GetFontLineSkip(font)) * 1.1f;  // win.fnt.height
    for (const auto& p : text) {
        float cx = x;
        bool any = false;
        for (const auto& run : p.runs) {
            const bool link = run.kind == TextRun::Kind::ObjectLink;
            const SDL_Color c = link ? (m_hoverObject == run.target ? colors.alink : colors.link) : colors.fg;
            for (const auto& w : words(run.text)) {
                const SDL_FPoint sz = textSize(w, font);
                if (cx + sz.x > x + width && cx > x) {  // перенос
                    cx = x;
                    y += lineHeight;
                }
                if (draw && y >= m_drawTop - 0.5f && y + lineHeight <= m_drawBottom + 0.5f) {
                    drawText(w, font, c, cx, y);
                    if (link) m_hits.push_back({{cx, y, sz.x, lineHeight}, Hit::Kind::Object, run.target});
                } else if (draw && y + lineHeight > m_drawBottom + 0.5f && m_firstHiddenY < 0) {
                    m_firstHiddenY = y;
                }
                cx += sz.x;
                any = true;
            }
        }
        y += any ? lineHeight : lineHeight * 0.5f;
    }
    return y;
}

float SdlPlayer::renderChoices(float x, float y, float width, TTF_Font* font, const ScreenTheme::Window& colors, bool draw) {
    const float lineHeight = static_cast<float>(TTF_GetFontLineSkip(font)) * 1.15f;
    y += lineHeight * 0.3f;
    int number = 0;
    for (const auto& c : m_screen.choices) {
        const std::string text = c.enabled ? std::to_string(++number) + ". " + c.text : c.altText;
        const SDL_FPoint sz = textSize(text, font);
        if (draw && y >= m_drawTop - 0.5f && y + lineHeight <= m_drawBottom + 0.5f) {
            drawText(text, font, c.enabled ? colors.link : colors.fg, x, y);
            if (c.enabled) m_hits.push_back({{x, y, std::min(sz.x, width), lineHeight}, Hit::Kind::Choice, "", c.index});
        } else if (draw && y + lineHeight > m_drawBottom + 0.5f && m_firstHiddenY < 0) {
            m_firstHiddenY = y;
        }
        y += lineHeight;
    }
    return y;
}

SDL_FRect SdlPlayer::objectRect(size_t index) {
    const SceneObject& o = m_screen.sceneObjects[index];
    float w = 0, h = 0;
    if (SDL_Texture* t = texture(o.image)) SDL_GetTextureSize(t, &w, &h);
    else if (isHumanLabel(o.label)) { const SDL_FPoint s = textSize(o.label, m_smallFont); w = s.x; h = s.y; }
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
        return {parent->second.x + ax, parent->second.y + ay, w, h};
    }
    // Корневой: vn.lua postoxy
    const int a = static_cast<int>(p.anchor);
    const int col = a % 3, row = a / 3;   // 0 — left/top, 1 — middle, 2 — right/bottom
    float x = col == 0 ? static_cast<float>(p.armX) : col == 2 ? SCREEN_W - w : std::floor((SCREEN_W - w) / 2);
    float y = row == 0 ? static_cast<float>(p.armY) : row == 2 ? SCREEN_H - h : std::floor((SCREEN_H - h) / 2);
    if (p.hasOffset) { x += static_cast<float>(p.offsetX); y += static_cast<float>(p.offsetY); }
    return {x, y, w, h};
}

void SdlPlayer::render() {
    SDL_SetRenderDrawColor(m_renderer, BACKGROUND.r, BACKGROUND.g, BACKGROUND.b, 255);
    SDL_RenderClear(m_renderer);
    m_hits.clear();
    if (m_screen.layout == Layout::VisualNovel) renderVisualNovel();
    else renderStandard();
    renderOverlayMessages();
    if (m_menu != Menu::None) renderMenu();
}

void SdlPlayer::renderTextWindow(const ScreenTheme::Window& window, bool panel) {
    TTF_Font* font = fontFor(window);
    const SDL_FRect r = window.rect;
    // Сколько места занимает текст (для прокрутки)
    float need = 0;
    if (!m_screen.caption.empty()) need += TTF_GetFontLineSkip(font) * 1.4f;
    need = renderParagraphs(m_screen.pageText, r.x, need, r.w, font, window, false);
    need = renderParagraphs(m_screen.reactionText, r.x, need, r.w, font, window, false);
    need = renderChoices(r.x, need, r.w, font, window, false);
    m_contentHeight = need;
    // Окно показывает целое число строк; не уместившийся текст листается страницами, как
    // paginator в INSTEAD: щелчок по окну (или колесо, PgDn) — следующая часть
    const float lh = static_cast<float>(TTF_GetFontLineSkip(font)) * 1.1f;
    const float pageH = std::max(lh, std::floor(r.h / lh) * lh);
    const float maxScroll = std::max(0.0f, need - pageH);
    m_scroll = std::min(m_scroll, maxScroll);
    m_textWindow = r;
    m_pageHeight = pageH;
    m_moreText = m_scroll < maxScroll;
    if (panel) fillRect({r.x - 16, r.y - 10, r.w + 32, r.h + 20}, {0, 0, 0, 140});
    SDL_Rect clip{static_cast<int>(r.x - 8), static_cast<int>(r.y), static_cast<int>(r.w + 16), static_cast<int>(r.h)};
    SDL_SetRenderClipRect(m_renderer, &clip);
    const size_t firstHit = m_hits.size();
    float y = r.y - m_scroll;
    m_drawTop = r.y;
    m_drawBottom = r.y + r.h;
    m_firstHiddenY = -1;
    if (!m_screen.caption.empty()) {
        drawText(m_screen.caption, font, window.fg, r.x, y);
        y += TTF_GetFontLineSkip(font) * 1.4f;
    }
    y = renderParagraphs(m_screen.pageText, r.x, y, r.w, font, window, true);
    y = renderParagraphs(m_screen.reactionText, r.x, y, r.w, font, window, true);
    renderChoices(r.x, y, r.w, font, window, true);
    SDL_SetRenderClipRect(m_renderer, nullptr);
    m_drawTop = -1e9f;
    m_drawBottom = 1e9f;
    // Следующая страница начинается с первой строки, которая не уместилась
    m_moreText = m_firstHiddenY >= 0;
    if (m_moreText) m_pageHeight = std::max(lh, m_firstHiddenY - r.y);
    // Ссылки за краем окна не кликабельны
    m_hits.erase(std::remove_if(m_hits.begin() + static_cast<long>(firstHit), m_hits.end(), [&](const Hit& h) {
        return h.rect.y + h.rect.h <= r.y || h.rect.y >= r.y + r.h;
    }), m_hits.end());
    // Есть ещё текст — стрелки, как win.gfx.up/down в INSTEAD (прокрутка колесом или PgUp/PgDn)
    if (m_scroll > 0) drawText("▲", font, window.link, r.x + r.w + 4, r.y);
    if (m_firstHiddenY >= 0) drawText("▼", font, window.link, r.x + r.w + 4, r.y + r.h - lh);
}

void SdlPlayer::renderStandard() {
    const ScreenTheme& t = m_standardTheme;
    fillRect({0, 0, SCREEN_W, SCREEN_H}, t.background);
    if (!t.backgroundImage.empty()) {
        if (SDL_Texture* bg = textureFor(t.backgroundImage)) {
            SDL_FRect full{0, 0, SCREEN_W, SCREEN_H};
            SDL_RenderTexture(m_renderer, bg, nullptr, &full);
        }
    }
    if (SDL_Texture* img = texture(m_screen.pageImage)) {
        SDL_FRect dst;
        if (m_screen.imageIsBackground) {
            dst = {0, 0, SCREEN_W, SCREEN_H};
        } else {
            // scr.gfx: картинка страницы в своей области, вписана и по центру (gfx.mode = float)
            float w, h;
            SDL_GetTextureSize(img, &w, &h);
            const SDL_FRect a = t.pictureArea;
            const float k = std::min(a.w / w, a.h / h);
            dst = {a.x + (a.w - w * k) / 2, a.y + (a.h - h * k) / 2, w * k, h * k};
        }
        SDL_RenderTexture(m_renderer, img, nullptr, &dst);
    }
    // Графические объекты (кнопки, подсказки) — на своих местах
    m_objectRects.clear();
    for (size_t i = 0; i < m_screen.sceneObjects.size(); ++i) {
        const SceneObject& o = m_screen.sceneObjects[i];
        if (!o.graphical || !o.visible || o.collapsed) continue;
        const SDL_FRect r = objectRect(i);
        m_objectRects[o.id] = r;
        const ImageRef& img = (m_hoverObject == o.id && !o.hoverImage.empty()) ? o.hoverImage : o.image;
        if (SDL_Texture* tex = texture(img)) SDL_RenderTexture(m_renderer, tex, nullptr, &r);
        else if (isHumanLabel(o.label)) drawText(o.label, fontFor(t.text), t.text.fg, r.x, r.y);
        if (o.active) m_hits.push_back({r, Hit::Kind::Object, o.id});
    }
    renderTextWindow(t.text, m_screen.imageIsBackground);
    renderInventory(t.inventory, false);
}

void SdlPlayer::renderInventory(const ScreenTheme::Window& window, bool panel) {
    if (m_screen.inventory.empty()) return;
    TTF_Font* font = fontFor(window);
    const SDL_FRect r = window.rect;
    if (panel) fillRect({r.x, r.y, r.w, r.h}, {0, 0, 0, 140});
    const float lineHeight = static_cast<float>(TTF_GetFontLineSkip(font)) * 1.15f;
    const float gap = textSize("    ", font).x;
    // Элемент: подпись (многострочная — через « / ») и значок
    struct Item { const InventoryItem* item; std::string label; float w; };
    std::vector<Item> items;
    for (const auto& item : m_screen.inventory) {
        std::string label = item.label;
        std::replace(label.begin(), label.end(), '\n', ' ');
        float w = textSize(label, font).x + (texture(item.icon) ? lineHeight + 6 : 0);
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
            const SDL_FRect cell{x, y, it.w, lineHeight};
            if (it.item->id == m_selectedItem) fillRect({cell.x - 6, cell.y, cell.w + 12, cell.h}, {255, 255, 255, 60});
            float tx = x;
            if (SDL_Texture* icon = texture(it.item->icon)) {
                SDL_FRect dst{x, y, lineHeight, lineHeight};
                SDL_RenderTexture(m_renderer, icon, nullptr, &dst);
                tx += lineHeight + 6;
            }
            const bool selected = it.item->id == m_selectedItem;
            drawText(it.label, font, !it.item->active ? window.fg : selected ? window.alink : window.link, tx, y);
            if (it.item->active) m_hits.push_back({cell, Hit::Kind::Inventory, it.item->id});
            x += it.w + gap;
        }
        y += lineHeight;
        if (y > r.y + r.h + lineHeight) break;
    }
}

void SdlPlayer::renderVisualNovel() {
    const ScreenTheme& t = m_vnTheme;
    fillRect({0, 0, SCREEN_W, SCREEN_H}, t.background);
    if (SDL_Texture* bg = texture(m_screen.pageImage)) {
        SDL_FRect full{0, 0, SCREEN_W, SCREEN_H};
        SDL_RenderTexture(m_renderer, bg, nullptr, &full);
    }
    // Графические объекты в порядке обхода сцены (родитель раньше детей)
    m_objectRects.clear();
    TTF_Font* labelFont = fontFor(t.text);
    for (size_t i = 0; i < m_screen.sceneObjects.size(); ++i) {
        const SceneObject& o = m_screen.sceneObjects[i];
        if (!o.graphical || !o.visible || o.collapsed) continue;
        const SDL_FRect r = objectRect(i);
        m_objectRects[o.id] = r;
        const ImageRef& img = (m_hoverObject == o.id && !o.hoverImage.empty()) ? o.hoverImage : o.image;
        if (SDL_Texture* tex = texture(img)) SDL_RenderTexture(m_renderer, tex, nullptr, &r);
        if (isHumanLabel(o.label)) {
            // Подпись спрайта — как draw_hud в vn.lua: справа от картинки (+4 +5 px), по вертикали
            // по углу привязки (top — верх, middle — середина, bottom — низ; у вложенного — середина)
            const float lh = static_cast<float>(TTF_GetFontLineSkip(labelFont));
            std::vector<std::string> lines;
            std::istringstream in(o.label);
            for (std::string line; std::getline(in, line);) lines.push_back(line);
            const float total = lh * static_cast<float>(lines.size());
            const int row = static_cast<int>(o.placement.anchor) / 3;
            const float coef = !o.parentId.empty() ? 0.5f : row == 0 ? 0.0f : row == 1 ? 0.5f : 1.0f;
            const float tx = r.x + r.w + 4 + 5;
            float ty = r.y + r.h * coef - total * coef;
            for (const auto& line : lines) {
                drawText(line, labelFont, t.text.fg, tx, ty);
                ty += lh;
            }
        }
        if (o.active) m_hits.push_back({r, Hit::Kind::Object, o.id});
    }
    const bool hasText = !m_screen.pageText.empty() || !m_screen.reactionText.empty() || !m_screen.choices.empty();
    if (hasText) renderTextWindow(t.text, true);
    renderInventory(t.inventory, false);
}

void SdlPlayer::renderOverlayMessages() {
    if (!m_toasts.empty() && SDL_GetTicks() < m_toastUntil) {
        float y = 30;
        for (const auto& t : m_toasts) {
            const SDL_FPoint s = textSize(t, m_font);
            fillRect({SCREEN_W - s.x - 70, y - 10, s.x + 40, s.y + 20}, SELECTED);
            drawText(t, m_font, TEXT, SCREEN_W - s.x - 50, y);
            y += s.y + 30;
        }
    } else {
        m_toasts.clear();
    }
    if (!m_selectedItem.empty() && m_menu == Menu::None) {
        drawText("Выбран предмет: щёлкните, к чему применить (правая кнопка — отмена)", m_smallFont, REACTION, 60, SCREEN_H - 36);
    }
}

void SdlPlayer::renderMenu() {
    fillRect({0, 0, SCREEN_W, SCREEN_H}, {0, 0, 0, 160});
    std::vector<std::pair<std::string, bool>> items;  // текст, доступен
    std::string title;
    if (m_menu == Menu::Main) {
        title = "Меню";
        const bool saves = !m_options.savesDir.empty();
        items = {{m_engine->canSave() ? "Сохранить игру (F5)" : "Сохранить игру — здесь нельзя", saves && m_engine->canSave()},
                 {"Загрузить игру (F9)", saves},
                 {"Начать заново", true},
                 {m_director ? (m_soundOn ? "Выключить звук" : "Включить звук") : "Звук недоступен", m_director != nullptr},
                 {"Выйти", true},
                 {"Вернуться в игру (Esc)", true}};
    } else if (m_menu == Menu::Save || m_menu == Menu::Load) {
        title = m_menu == Menu::Save ? "Сохранить в слот" : "Загрузить из слота";
        items.push_back({"Назад", true});
        for (int slot = 1; slot <= 9; ++slot) {
            const auto info = m_engine->readSaveInfo(slotPath(slot));
            const std::string text = std::to_string(slot) + ". " + (info.exists ? info.time + " — " + info.label : "пусто");
            items.push_back({text, m_menu == Menu::Save || (info.exists && info.compatible)});
        }
    } else {
        const float lh = static_cast<float>(TTF_GetFontLineSkip(m_titleFont));
        std::istringstream lines(m_message);
        std::string line;
        float y = SCREEN_H / 2 - lh;
        while (std::getline(lines, line)) {
            const SDL_FPoint s = textSize(line, m_titleFont);
            drawText(line, m_titleFont, TEXT, (SCREEN_W - s.x) / 2, y);
            y += lh * 1.2f;
        }
        return;
    }
    const float lh = static_cast<float>(TTF_GetFontLineSkip(m_font)) * 1.6f;
    float y = (SCREEN_H - lh * (static_cast<float>(items.size()) + 2)) / 2;
    const SDL_FPoint ts = textSize(title, m_titleFont);
    drawText(title, m_titleFont, TEXT, (SCREEN_W - ts.x) / 2, y);
    y += lh * 1.5f;
    for (size_t i = 0; i < items.size(); ++i) {
        const SDL_FPoint s = textSize(items[i].first, m_font);
        const float x = (SCREEN_W - s.x) / 2;
        drawText(items[i].first, m_font, items[i].second ? LINK : DIM, x, y);
        // Индексы: в главном меню — по порядку; в слотах «Назад» — 0, слоты — 1..9
        const int index = m_menu == Menu::Main ? static_cast<int>(i) : static_cast<int>(i);
        if (items[i].second) m_hits.push_back({{x - 20, y, s.x + 40, lh}, Hit::Kind::Menu, "", index});
        y += lh;
    }
}

// ============================================================================ сценарий (тесты)

bool SdlPlayer::saveScreenshot(const std::string& path) {
    render();
    SDL_Surface* s = SDL_RenderReadPixels(m_renderer, nullptr);
    if (!s) return false;
    const bool ok = IMG_SavePNG(s, path.c_str());
    SDL_DestroySurface(s);
    SDL_RenderPresent(m_renderer);
    return ok;
}

bool SdlPlayer::scriptCommand(const std::string& line) {
    auto starts = [&](const char* p) { return line.rfind(p, 0) == 0; };
    auto findObj = [&](const std::vector<PlayerEngine::ObjectView>& list, const std::string& label) {
        for (const auto& o : list) if (o.disp.rfind(label, 0) == 0) return o.instanceId;
        return std::string();
    };
    const PlayerEngine::PageView v = m_engine->view();
    bool ok = true;
    if (starts("c:")) {
        ok = false;
        for (size_t i = 0; i < v.choices.size(); ++i) {
            if (v.choices[i].text.rfind(line.substr(2), 0) == 0) { ok = m_engine->choose(i); break; }
        }
    } else if (starts("o:")) {
        const std::string id = findObj(v.sceneObjects, line.substr(2));
        ok = !id.empty() && m_engine->actOnObject(id);
    } else if (starts("i:")) {
        const std::string id = findObj(v.inventory, line.substr(2));
        ok = !id.empty() && m_engine->clickInventory(id);
    } else if (starts("u:")) {
        const auto gt = line.find('>');
        const std::string a = findObj(v.inventory, line.substr(2, gt - 2));
        std::string b = findObj(v.sceneObjects, line.substr(gt + 1));
        if (b.empty()) b = findObj(v.inventory, line.substr(gt + 1));
        ok = !a.empty() && !b.empty() && m_engine->use(a, b);
    } else if (starts("start:")) {
        // начать с указанной страницы (для проверки отдельных сцен)
        m_engine->start(line.substr(6));
    } else if (starts("set:")) {
        // set:имя=выражение — задать переменную
        const auto eq = line.find('=');
        m_engine->setVariable(line.substr(4, eq - 4), m_engine->evaluate(line.substr(eq + 1)));
    } else if (starts("wait:")) {
        // время идёт кадрами по 16 мс, как в настоящем цикле
        for (int ms = std::stoi(line.substr(5)); ms > 0; ms -= 16) {
            if (m_engine->advanceTime(16) > 0) refresh();
        }
    } else if (starts("click:")) {
        float x = 0, y = 0;
        std::sscanf(line.c_str() + 6, "%f,%f", &x, &y);
        render();
        click(x, y, false);
    } else if (starts("clickobj:") || starts("clickinv:")) {
        // щелчок мышью по объекту (ссылке в тексте или спрайту) или предмету — по его подписи
        const bool inv = starts("clickinv:");
        const std::string id = findObj(inv ? v.inventory : v.sceneObjects, line.substr(9));
        render();
        ok = false;
        for (const auto& h : m_hits) {
            if (h.id == id && h.kind == (inv ? Hit::Kind::Inventory : Hit::Kind::Object)) {
                click(h.rect.x + h.rect.w / 2, h.rect.y + h.rect.h / 2, false);
                ok = true;
                break;
            }
        }
    } else if (starts("hover:")) {
        float x = 0, y = 0;
        std::sscanf(line.c_str() + 6, "%f,%f", &x, &y);
        render();
        m_hoverObject.clear();
        for (auto it = m_hits.rbegin(); it != m_hits.rend(); ++it) {
            if (it->kind == Hit::Kind::Object && contains(it->rect, x, y)) { m_hoverObject = it->id; break; }
        }
    } else if (starts("key:")) {
        const std::string k = line.substr(4);
        key(k == "esc" ? SDLK_ESCAPE : k == "f5" ? SDLK_F5 : k == "f9" ? SDLK_F9 : static_cast<SDL_Keycode>(SDLK_0 + std::stoi(k)));
    } else if (starts("shot:")) {
        ok = saveScreenshot(line.substr(5));
    } else if (!line.empty() && line[0] != '#') {
        ok = false;
    }
    if (!starts("shot:") && !starts("click") && !starts("hover:") && !starts("key:")) afterCommand();
    std::cout << (ok ? "" : "[НЕ ВЫПОЛНЕНО] ") << line << std::endl;
    return ok;
}

bool SdlPlayer::runScript() {
    std::ifstream in(FileUtils::nativePath(m_options.scriptPath));
    if (!in) { std::cerr << "Нет сценария: " << m_options.scriptPath << std::endl; return false; }
    std::string line;
    bool all = true;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        all = scriptCommand(line) && all;
    }
    return all;
}
