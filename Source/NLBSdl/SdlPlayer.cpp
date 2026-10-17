#include "SdlPlayer.h"

#include <SDL3_image/SDL_image.h>

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iostream>

#include "nlb/audio/SdlAudioBackend.h"
#include "nlb/presentation/BookMediaSource.h"
#include "nlb/presentation/Session.h"
#include "nlb/util/BookPack.h"
#include "nlb/util/FileUtils.h"

using namespace nlb::presentation;

namespace {
const SDL_Color BACKGROUND = {12, 12, 16, 255};

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
    m_fontPath = m_options.fontPath.empty() ? findFont(m_options.bookPath) : m_options.fontPath;
    if (m_fontPath.empty()) {
        std::cerr << "Нет шрифта: укажите --font <файл .ttf>" << std::endl;
        return false;
    }
    m_media = std::make_unique<BookMediaSource>(m_options.bookPath);
    if (m_options.sound) {
        m_audio = std::make_unique<nlb::audio::SdlAudioBackend>(*m_media);
        if (!m_audio->isAvailable()) {
            std::cerr << "Warning: sound is not available: " << m_audio->lastError() << std::endl;
            m_audio.reset();
        }
    }
    m_session = std::make_unique<Session>(m_audio.get());
    Session::Options so;
    so.bookPath = m_options.bookPath;
    so.exportMode = m_options.exportMode;
    so.achievementsPath = m_options.achievementsPath;
    so.savesDir = m_options.savesDir;
    std::string error;
    if (!m_session->open(so, error)) {
        std::cerr << "Cannot load book: " << error << std::endl;
        return false;
    }
    const std::string title = "NLBNavigator — " + m_session->title();
    if (!SDL_CreateWindowAndRenderer(title.c_str(), m_options.windowWidth, m_options.windowHeight,
                                     SDL_WINDOW_RESIZABLE, &m_window, &m_renderer)) {
        std::cerr << "SDL_CreateWindowAndRenderer: " << SDL_GetError() << std::endl;
        return false;
    }
    // Экран экспорта — 1920×1080; в окно он вписывается с полями (letterbox), как в теме INSTEAD
    SDL_SetRenderLogicalPresentation(m_renderer, static_cast<int>(GameScreen::SCREEN_W), static_cast<int>(GameScreen::SCREEN_H),
                                     SDL_LOGICAL_PRESENTATION_LETTERBOX);
    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderVSync(m_renderer, 1);

    GameScreen::Options go;
    go.themeDir = m_options.themeDir;
    go.baseThemeDir = m_options.baseThemeDir;
    go.choicesScreen = m_options.choicesScreen;
    go.soundAvailable = m_audio != nullptr;
    go.clock = [] { return static_cast<uint64_t>(SDL_GetTicks()); };
    m_screen = std::make_unique<GameScreen>(*m_session, go);
    std::cerr << "Оформление: " << m_screen->describeThemes() << std::endl;
    return true;
}

void SdlPlayer::shutdown() {
    for (auto& [k, t] : m_images) if (t) SDL_DestroyTexture(t);
    for (auto& [k, t] : m_texts) if (t) SDL_DestroyTexture(t);
    m_images.clear();
    m_texts.clear();
    m_screen.reset();
    m_session.reset();
    m_audio.reset();
    for (auto& [k, f] : m_fonts) if (f) TTF_CloseFont(f);
    m_fonts.clear();
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

// ============================================================================ цикл и ввод

void SdlPlayer::loop() {
    Uint64 last = SDL_GetTicks();
    while (!m_quit && !m_screen->quitRequested()) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) handleEvent(e);
        const Uint64 now = SDL_GetTicks();
        m_screen->update(static_cast<int>(now - last));
        last = now;
        renderFrame();
        SDL_RenderPresent(m_renderer);
        if (SDL_GetWindowFlags(m_window) & SDL_WINDOW_MINIMIZED) SDL_Delay(50);
    }
}

void SdlPlayer::handleEvent(SDL_Event& e) {
    SDL_ConvertEventToRenderCoordinates(m_renderer, &e);
    switch (e.type) {
        case SDL_EVENT_QUIT: m_quit = true; break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            m_screen->click(e.button.x, e.button.y, e.button.button == SDL_BUTTON_RIGHT);
            break;
        case SDL_EVENT_MOUSE_MOTION:
            m_screen->mouseMove(e.motion.x, e.motion.y);
            break;
        case SDL_EVENT_MOUSE_WHEEL: {
            // «перевёрнутое» колесо (macOS) — наоборот
            float dy = e.wheel.y;
            if (e.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) dy = -dy;
            m_screen->wheel(dy);
            break;
        }
        case SDL_EVENT_KEY_DOWN: {
            const SDL_Keycode k = e.key.key;
            using Key = GameScreen::Key;
            if (k == SDLK_ESCAPE) m_screen->key(Key::Escape);
            else if (k == SDLK_SPACE) m_screen->key(Key::Space);
            else if (k == SDLK_RETURN) m_screen->key(Key::Enter);
            else if (k == SDLK_PAGEUP) m_screen->key(Key::PageUp);
            else if (k == SDLK_PAGEDOWN) m_screen->key(Key::PageDown);
            else if (k == SDLK_F5) m_screen->key(Key::Save);
            else if (k == SDLK_F9) m_screen->key(Key::Load);
            else if (k >= SDLK_1 && k <= SDLK_9) m_screen->key(Key::Digit, static_cast<int>(k - SDLK_1) + 1);
            break;
        }
        default: break;
    }
}

void SdlPlayer::renderFrame() {
    SDL_SetRenderDrawColor(m_renderer, BACKGROUND.r, BACKGROUND.g, BACKGROUND.b, 255);
    SDL_RenderClear(m_renderer);
    m_screen->render(*this);
}

// ============================================================================ рендерер

TTF_Font* SdlPlayer::openFont(const std::string& path, float size) {
    std::string rel;
    if (!BookPack::resolve(path, rel)) return TTF_OpenFont(path.c_str(), size);
    // шрифт темы, зашитой в пакет: SDL_ttf читает его из памяти, пока шрифт открыт
    auto& data = m_fontData[path];
    if (!data) {
        data = std::make_shared<std::string>();
        if (!FileUtils::readFile(path, *data)) return nullptr;
    }
    SDL_IOStream* io = SDL_IOFromConstMem(data->data(), data->size());
    return io ? TTF_OpenFontIO(io, true, size) : nullptr;
}

TTF_Font* SdlPlayer::font(const FontRef& ref) {
    const std::string file = (!ref.file.empty() && FileUtils::exists(ref.file)) ? ref.file : m_fontPath;
    const std::string key = file + "|" + std::to_string(ref.size);
    auto it = m_fonts.find(key);
    if (it != m_fonts.end()) return it->second;
    TTF_Font* f = openFont(file, ref.size);
    if (!f && file != m_fontPath) f = TTF_OpenFont(m_fontPath.c_str(), ref.size);
    m_fonts[key] = f;
    return f;
}

SDL_Texture* SdlPlayer::textTexture(const std::string& text, TTF_Font* f, SDL_Color color) {
    if (text.empty() || !f) return nullptr;
    const std::string key = std::to_string(reinterpret_cast<uintptr_t>(f)) + "|" + std::to_string(color.r) + ","
        + std::to_string(color.g) + "," + std::to_string(color.b) + "|" + text;
    auto it = m_texts.find(key);
    if (it != m_texts.end()) return it->second;
    if (m_texts.size() > 4000) {  // кэш надписей не растёт бесконечно
        for (auto& [k, t] : m_texts) SDL_DestroyTexture(t);
        m_texts.clear();
    }
    SDL_Surface* surface = TTF_RenderText_Blended(f, text.c_str(), 0, color);
    SDL_Texture* tex = surface ? SDL_CreateTextureFromSurface(m_renderer, surface) : nullptr;
    if (surface) SDL_DestroySurface(surface);
    m_texts[key] = tex;
    return tex;
}

SDL_Texture* SdlPlayer::textureFor(const std::string& path) {
    if (path.empty()) return nullptr;
    auto it = m_images.find(path);
    if (it != m_images.end()) return it->second;
    SDL_Texture* tex = nullptr;
    // Полный путь (картинки темы: фон, рамки) — напрямую, с диска или из пакета; иначе — медиа книги
    std::string rel;
    const bool fullPath = path.find('/') != std::string::npos && FileUtils::exists(path) && !FileUtils::isDirectory(path);
    const std::string file = fullPath ? (BookPack::resolve(path, rel) ? std::string() : path) : m_media->filePath(MediaRef{path});
    if (fullPath && file.empty()) {
        std::string direct;
        if (FileUtils::readFile(path, direct) && !direct.empty()) {
            SDL_IOStream* io = SDL_IOFromConstMem(direct.data(), direct.size());
            if (io) tex = IMG_LoadTexture_IO(m_renderer, io, true);
        }
    } else if (!file.empty()) {
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

bool SdlPlayer::imageSize(const std::string& path, float& w, float& h) {
    SDL_Texture* tex = textureFor(path);
    if (!tex) return false;
    SDL_GetTextureSize(tex, &w, &h);
    return true;
}

void SdlPlayer::drawImage(const std::string& path, const RectF& dst, float alpha) {
    SDL_Texture* tex = textureFor(path);
    if (!tex) return;
    const SDL_FRect d{dst.x, dst.y, dst.w, dst.h};
    SDL_SetTextureAlphaModFloat(tex, alpha);
    SDL_RenderTexture(m_renderer, tex, nullptr, &d);
    SDL_SetTextureAlphaModFloat(tex, 1.0f);
}

void SdlPlayer::fillRect(const RectF& r, Color c) {
    SDL_SetRenderDrawColor(m_renderer, c.r, c.g, c.b, c.a);
    const SDL_FRect d{r.x, r.y, r.w, r.h};
    SDL_RenderFillRect(m_renderer, &d);
}

float SdlPlayer::textWidth(const FontRef& ref, const std::string& text) {
    int w = 0, h = 0;
    if (TTF_Font* f = font(ref); f && !text.empty()) TTF_GetStringSize(f, text.c_str(), 0, &w, &h);
    return static_cast<float>(w);
}

float SdlPlayer::fontHeight(const FontRef& ref) {
    TTF_Font* f = font(ref);
    return f ? static_cast<float>(TTF_GetFontHeight(f)) : 0.0f;
}

float SdlPlayer::lineSkip(const FontRef& ref) {
    TTF_Font* f = font(ref);
    return f ? static_cast<float>(TTF_GetFontLineSkip(f)) : 0.0f;
}

void SdlPlayer::drawText(const FontRef& ref, const std::string& text, Color c, float x, float y, float alpha) {
    SDL_Texture* tex = textTexture(text, font(ref), SDL_Color{c.r, c.g, c.b, c.a});
    if (!tex) return;
    float w, h;
    SDL_GetTextureSize(tex, &w, &h);
    const SDL_FRect dst{x, y, w, h};
    SDL_SetTextureAlphaModFloat(tex, alpha);
    SDL_RenderTexture(m_renderer, tex, nullptr, &dst);
    SDL_SetTextureAlphaModFloat(tex, 1.0f);
}

void SdlPlayer::setClip(const RectF* clip) {
    if (!clip) {
        SDL_SetRenderClipRect(m_renderer, nullptr);
        return;
    }
    const SDL_Rect r{static_cast<int>(clip->x), static_cast<int>(clip->y), static_cast<int>(clip->w), static_cast<int>(clip->h)};
    SDL_SetRenderClipRect(m_renderer, &r);
}

// ============================================================================ сценарий (тесты)

bool SdlPlayer::saveScreenshot(const std::string& path) {
    renderFrame();
    SDL_Surface* s = SDL_RenderReadPixels(m_renderer, nullptr);
    if (!s) return false;
    const bool ok = IMG_SavePNG(s, path.c_str());
    SDL_DestroySurface(s);
    SDL_RenderPresent(m_renderer);
    return ok;
}

bool SdlPlayer::scriptCommand(const std::string& line) {
    auto starts = [&](const char* p) { return line.rfind(p, 0) == 0; };
    PlayerEngine& engine = m_session->engine();
    auto findObj = [&](const std::vector<PlayerEngine::ObjectView>& list, const std::string& label) {
        for (const auto& o : list) if (o.disp.rfind(label, 0) == 0) return o.instanceId;
        return std::string();
    };
    const PlayerEngine::PageView v = engine.view();
    bool ok = true;
    if (starts("c:")) {
        ok = false;
        for (size_t i = 0; i < v.choices.size(); ++i) {
            if (v.choices[i].text.rfind(line.substr(2), 0) == 0) { ok = engine.choose(i); break; }
        }
    } else if (starts("o:")) {
        const std::string id = findObj(v.sceneObjects, line.substr(2));
        ok = !id.empty() && engine.actOnObject(id);
    } else if (starts("i:")) {
        const std::string id = findObj(v.inventory, line.substr(2));
        ok = !id.empty() && engine.clickInventory(id);
    } else if (starts("u:")) {
        const auto gt = line.find('>');
        const std::string a = findObj(v.inventory, line.substr(2, gt - 2));
        std::string b = findObj(v.sceneObjects, line.substr(gt + 1));
        if (b.empty()) b = findObj(v.inventory, line.substr(gt + 1));
        ok = !a.empty() && !b.empty() && engine.use(a, b);
    } else if (starts("start:")) {
        engine.start(line.substr(6));  // начать с указанной страницы (проверка отдельных сцен)
    } else if (starts("set:")) {
        const auto eq = line.find('=');
        engine.setVariable(line.substr(4, eq - 4), engine.evaluate(line.substr(eq + 1)));
    } else if (starts("wait:")) {
        for (int ms = std::stoi(line.substr(5)); ms > 0; ms -= 16) {
            SDL_Delay(16);  // настоящее время: эффекты спрайтов идут по часам SDL
            m_screen->update(16);
        }
    } else if (starts("click:")) {
        float x = 0, y = 0;
        std::sscanf(line.c_str() + 6, "%f,%f", &x, &y);
        renderFrame();
        m_screen->click(x, y, false);
    } else if (starts("clickobj:") || starts("clickinv:")) {
        // щелчок мышью по объекту (ссылке или спрайту) или предмету — по его подписи
        const bool inv = starts("clickinv:");
        const std::string id = findObj(inv ? v.inventory : v.sceneObjects, line.substr(9));
        renderFrame();
        float x = 0, y = 0;
        ok = m_screen->hitCenter(id, inv, x, y);
        if (ok) m_screen->click(x, y, false);
    } else if (starts("menu:")) {
        // щелчок мышью по пункту открытого меню с номером N (0 — первый)
        renderFrame();
        float x = 0, y = 0;
        ok = m_screen->menuItemCenter(std::stoi(line.substr(5)), x, y);
        if (ok) m_screen->click(x, y, false);
    } else if (starts("wheel:")) {
        renderFrame();
        SDL_Event e{};
        e.type = SDL_EVENT_MOUSE_WHEEL;
        e.wheel.y = std::stof(line.substr(6));
        e.wheel.direction = SDL_MOUSEWHEEL_NORMAL;
        e.wheel.windowID = SDL_GetWindowID(m_window);
        handleEvent(e);
    } else if (starts("hover:")) {
        float x = 0, y = 0;
        std::sscanf(line.c_str() + 6, "%f,%f", &x, &y);
        renderFrame();
        m_screen->mouseMove(x, y);
    } else if (starts("key:")) {
        const std::string k = line.substr(4);
        using Key = GameScreen::Key;
        if (k == "esc") m_screen->key(Key::Escape);
        else if (k == "f5") m_screen->key(Key::Save);
        else if (k == "f9") m_screen->key(Key::Load);
        else if (k == "space") m_screen->key(Key::Space);
        else m_screen->key(Key::Digit, std::stoi(k));
        renderFrame();
    } else if (starts("shot:")) {
        ok = saveScreenshot(line.substr(5));
    } else if (!line.empty() && line[0] != '#') {
        ok = false;
    }
    if (!starts("shot:") && !starts("click") && !starts("hover:") && !starts("key:") && !starts("wheel:") && !starts("menu:")) {
        m_session->refresh();
        m_screen->afterCommand();
    }
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
        if (m_screen->quitRequested()) { std::cout << "ВЫХОД" << std::endl; break; }
    }
    return all;
}
