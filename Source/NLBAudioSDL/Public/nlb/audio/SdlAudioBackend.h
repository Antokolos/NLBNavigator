#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "nlb/presentation/AudioBackend.h"
#include "nlb/presentation/MediaSource.h"

struct MIX_Mixer;
struct MIX_Track;
struct MIX_Audio;

namespace nlb::audio {

/**
 * @brief Звук через SDL3 + SDL3_mixer: музыка на отдельной дорожке, эффекты — на пуле дорожек.
 *
 * Работает без окна (инициализируется только аудиоподсистема SDL), поэтому подходит и для
 * консоли, в том числе в режиме экранного диктора. Файлы берутся через IMediaSource: с диска
 * или из памяти (пакет .nlbpack с медиа).
 */
class SdlAudioBackend : public nlb::presentation::IAudioBackend {
public:
    explicit SdlAudioBackend(const nlb::presentation::IMediaSource& media);
    ~SdlAudioBackend() override;
    SdlAudioBackend(const SdlAudioBackend&) = delete;
    SdlAudioBackend& operator=(const SdlAudioBackend&) = delete;

    /// Удалось ли открыть звуковое устройство
    bool isAvailable() const { return m_mixer != nullptr; }
    /// Последняя ошибка (не найден файл, не открылось устройство) — для журнала
    const std::string& lastError() const { return m_lastError; }

    void playMusic(const nlb::presentation::MediaRef& track, bool loop, int fadeInMs) override;
    void stopMusic(int fadeOutMs) override;
    void playEffect(const nlb::presentation::MediaRef& sound) override;
    void stopEffects() override;
    void setMusicVolume(float volume) override;
    void setEffectsVolume(float volume) override;

private:
    struct Loaded {
        MIX_Audio* audio = nullptr;
        std::shared_ptr<std::vector<uint8_t>> bytes;  ///< данные из памяти живут, пока жив звук
    };
    MIX_Audio* load(const nlb::presentation::MediaRef& ref, bool predecode);

    const nlb::presentation::IMediaSource& m_media;
    MIX_Mixer* m_mixer = nullptr;
    MIX_Track* m_music = nullptr;
    std::vector<MIX_Track*> m_effects;       ///< пул дорожек для эффектов
    size_t m_nextEffect = 0;
    std::map<std::string, Loaded> m_cache;
    std::string m_lastError;
    bool m_sdlInitialized = false;
};

}  // namespace nlb::audio
