#include "nlb/audio/SdlAudioBackend.h"

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

namespace nlb::audio {

namespace {
const size_t EFFECT_TRACKS = 8;
const char* const EFFECTS_TAG = "nlb_effects";
}

SdlAudioBackend::SdlAudioBackend(const nlb::presentation::IMediaSource& media) : m_media(media) {
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        m_lastError = SDL_GetError();
        return;
    }
    m_sdlInitialized = true;
    if (!MIX_Init()) {
        m_lastError = SDL_GetError();
        return;
    }
    m_mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    if (!m_mixer) {
        m_lastError = SDL_GetError();
        return;
    }
    m_music = MIX_CreateTrack(m_mixer);
    for (size_t i = 0; i < EFFECT_TRACKS; ++i) {
        MIX_Track* track = MIX_CreateTrack(m_mixer);
        if (track) {
            MIX_TagTrack(track, EFFECTS_TAG);
            m_effects.push_back(track);
        }
    }
}

SdlAudioBackend::~SdlAudioBackend() {
    if (m_mixer) {
        MIX_StopAllTracks(m_mixer, 0);
        for (auto& [name, loaded] : m_cache) {
            if (loaded.audio) MIX_DestroyAudio(loaded.audio);
        }
        m_cache.clear();
        MIX_DestroyMixer(m_mixer);  // дорожки уничтожаются вместе с микшером
        MIX_Quit();
    }
    if (m_sdlInitialized) SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

MIX_Audio* SdlAudioBackend::load(const nlb::presentation::MediaRef& ref, bool predecode) {
    auto cached = m_cache.find(ref.path);
    if (cached != m_cache.end()) return cached->second.audio;
    Loaded loaded;
    const std::string path = m_media.filePath(ref);
    if (!path.empty()) {
        loaded.audio = MIX_LoadAudio(m_mixer, path.c_str(), predecode);  // путь в UTF-8, как ждёт SDL
    } else {
        loaded.bytes = std::make_shared<std::vector<uint8_t>>();
        if (m_media.read(ref, *loaded.bytes) && !loaded.bytes->empty()) {
            SDL_IOStream* io = SDL_IOFromConstMem(loaded.bytes->data(), loaded.bytes->size());
            loaded.audio = io ? MIX_LoadAudio_IO(m_mixer, io, predecode, true) : nullptr;
        }
    }
    if (!loaded.audio) {
        m_lastError = "не удалось загрузить звук " + ref.path + (path.empty() ? "" : ": " + std::string(SDL_GetError()));
        return nullptr;  // не кэшируем: файл могут добавить позже
    }
    m_cache[ref.path] = loaded;
    return loaded.audio;
}

void SdlAudioBackend::playMusic(const nlb::presentation::MediaRef& track, bool loop, int fadeInMs) {
    if (!m_mixer || !m_music) return;
    MIX_Audio* audio = load(track, false);  // музыка — потоком, без полного декодирования
    if (!audio) return;
    MIX_StopTrack(m_music, 0);
    MIX_SetTrackAudio(m_music, audio);
    SDL_PropertiesID options = SDL_CreateProperties();
    SDL_SetNumberProperty(options, MIX_PROP_PLAY_LOOPS_NUMBER, loop ? -1 : 0);
    if (fadeInMs > 0) SDL_SetNumberProperty(options, MIX_PROP_PLAY_FADE_IN_MILLISECONDS_NUMBER, fadeInMs);
    if (!MIX_PlayTrack(m_music, options)) m_lastError = SDL_GetError();
    SDL_DestroyProperties(options);
}

void SdlAudioBackend::stopMusic(int fadeOutMs) {
    if (!m_music) return;
    MIX_StopTrack(m_music, fadeOutMs > 0 ? MIX_TrackMSToFrames(m_music, fadeOutMs) : 0);
}

void SdlAudioBackend::playEffect(const nlb::presentation::MediaRef& sound) {
    if (!m_mixer || m_effects.empty()) return;
    MIX_Audio* audio = load(sound, true);  // эффекты короткие — декодируем заранее
    if (!audio) return;
    // свободная дорожка, иначе — по кругу (самый старый эффект обрывается)
    MIX_Track* track = nullptr;
    for (MIX_Track* t : m_effects) {
        if (!MIX_TrackPlaying(t)) { track = t; break; }
    }
    if (!track) {
        track = m_effects[m_nextEffect];
        m_nextEffect = (m_nextEffect + 1) % m_effects.size();
        MIX_StopTrack(track, 0);
    }
    MIX_SetTrackAudio(track, audio);
    if (!MIX_PlayTrack(track, 0)) m_lastError = SDL_GetError();
}

void SdlAudioBackend::stopEffects() {
    if (m_mixer) MIX_StopTag(m_mixer, EFFECTS_TAG, 0);
}

void SdlAudioBackend::setMusicVolume(float volume) {
    if (m_music) MIX_SetTrackGain(m_music, volume);
}

void SdlAudioBackend::setEffectsVolume(float volume) {
    if (m_mixer) MIX_SetTagGain(m_mixer, EFFECTS_TAG, volume);
}

}  // namespace nlb::audio
