#pragma once

#include "nlb/presentation/ScreenModel.h"

namespace nlb::presentation {

/**
 * @brief Вывод звука средствами конкретной платформы. ЧЕРНОВИК ДЛЯ СОГЛАСОВАНИЯ.
 *
 * Намеренно минимален: логика «какую музыку играть» живёт в AudioDirector, общем для всех.
 * Адаптер к Godot (AudioStreamPlayer), Unreal (UAudioComponent) или SDL3_mixer — это только
 * эти несколько методов.
 */
class IAudioBackend {
public:
    virtual ~IAudioBackend() = default;
    /// Фоновая музыка: одна дорожка, по умолчанию зацикленная (set_music в INSTEAD)
    virtual void playMusic(const MediaRef& track, bool loop, int fadeInMs) = 0;
    virtual void stopMusic(int fadeOutMs) = 0;
    /// Звуковой эффект поверх музыки (add_sound); несколько эффектов могут звучать сразу
    virtual void playEffect(const MediaRef& sound) = 0;
    virtual void stopEffects() = 0;
    /// 0..1
    virtual void setMusicVolume(float volume) = 0;
    virtual void setEffectsVolume(float volume) = 0;
};

}  // namespace nlb::presentation
