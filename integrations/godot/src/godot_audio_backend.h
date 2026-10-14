#pragma once

#include <godot_cpp/classes/audio_stream.hpp>
#include <godot_cpp/classes/audio_stream_player.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/string.hpp>

#include <vector>

#include "nlb/presentation/AudioBackend.h"
#include "nlb/presentation/MediaSource.h"

/// Звук NLBNavigator средствами Godot: AudioStreamPlayer для музыки и пул для эффектов.
/// Потоки создаются из байтов книги (ogg, mp3, wav) — без импорта в проект.
class GodotAudioBackend : public nlb::presentation::IAudioBackend {
public:
    GodotAudioBackend(godot::Node* owner, const nlb::presentation::IMediaSource* media);
    void setMedia(const nlb::presentation::IMediaSource* media) { m_media = media; }

    void playMusic(const nlb::presentation::MediaRef& track, bool loop, int fadeInMs) override;
    void stopMusic(int fadeOutMs) override;
    void playEffect(const nlb::presentation::MediaRef& sound) override;
    void stopEffects() override;
    void setMusicVolume(float volume) override;
    void setEffectsVolume(float volume) override;

private:
    godot::Ref<godot::AudioStream> load(const nlb::presentation::MediaRef& ref, bool loop);

    const nlb::presentation::IMediaSource* m_media;
    godot::AudioStreamPlayer* m_music = nullptr;
    std::vector<godot::AudioStreamPlayer*> m_effects;
    size_t m_next = 0;
};
