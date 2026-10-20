#include "nlb/presentation/AudioDirector.h"

namespace nlb::presentation {

AudioDirector::AudioDirector(IAudioBackend& backend) : m_backend(backend) {}

void AudioDirector::setMuted(bool muted) {
    if (muted == m_muted) return;
    m_muted = muted;
    if (m_muted) {
        m_backend.stopMusic(0);
        m_backend.stopEffects();
    } else if (!m_currentMusic.empty()) {
        m_backend.playMusic(MediaRef{m_currentMusic}, true, 0);  // продолжить то, что должно играть
    }
}

void AudioDirector::apply(const std::vector<PlayerEngine::Event>& events) {
    using Kind = PlayerEngine::Event::Kind;
    for (const auto& event : events) {
        if (event.kind == Kind::Music) {
            // set_music: та же дорожка не перезапускается; пустое имя (VOID) — тишина
            if (event.text == m_currentMusic) continue;
            m_currentMusic = event.text;
            if (m_muted) continue;
            if (m_currentMusic.empty()) {
                m_backend.stopMusic(0);
            } else {
                m_backend.playMusic(MediaRef{m_currentMusic}, true, 0);
            }
        } else if (event.kind == Kind::Sound) {
            // add_sound: эффект поверх музыки
            if (!m_muted && !event.text.empty()) m_backend.playEffect(MediaRef{event.text});
        }
    }
}

void AudioDirector::playVoice(const std::string& file) {
    if (m_muted || file.empty()) return;
    m_backend.stopEffects();
    m_backend.playEffect(MediaRef{file});
}

void AudioDirector::stopVoice() {
    m_backend.stopEffects();
}

}  // namespace nlb::presentation
