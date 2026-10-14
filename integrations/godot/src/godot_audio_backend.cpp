#include "godot_audio_backend.h"

#include <godot_cpp/classes/audio_stream_mp3.hpp>
#include <godot_cpp/classes/audio_stream_ogg_vorbis.hpp>
#include <godot_cpp/classes/audio_stream_wav.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

GodotAudioBackend::GodotAudioBackend(Node* owner, const nlb::presentation::IMediaSource* media) : m_media(media) {
    m_music = memnew(AudioStreamPlayer);
    m_music->set_name("NLBMusic");
    owner->add_child(m_music);
    for (int i = 0; i < 8; ++i) {
        AudioStreamPlayer* p = memnew(AudioStreamPlayer);
        p->set_name(String("NLBEffect") + String::num_int64(i));
        owner->add_child(p);
        m_effects.push_back(p);
    }
}

Ref<AudioStream> GodotAudioBackend::load(const nlb::presentation::MediaRef& ref, bool loop) {
    std::vector<uint8_t> bytes;
    if (!m_media || !m_media->read(ref, bytes) || bytes.empty()) return Ref<AudioStream>();
    PackedByteArray data;
    data.resize(static_cast<int64_t>(bytes.size()));
    memcpy(data.ptrw(), bytes.data(), bytes.size());
    String name = String::utf8(ref.path.c_str()).to_lower();
    if (name.ends_with(".ogg")) {
        Ref<AudioStreamOggVorbis> s = AudioStreamOggVorbis::load_from_buffer(data);
        if (s.is_valid()) s->set_loop(loop);
        return s;
    }
    if (name.ends_with(".mp3")) {
        Ref<AudioStreamMP3> s = AudioStreamMP3::load_from_buffer(data);
        if (s.is_valid()) s->set_loop(loop);
        return s;
    }
    if (name.ends_with(".wav")) {
        Ref<AudioStreamWAV> s = AudioStreamWAV::load_from_buffer(data);
        if (s.is_valid() && loop) s->set_loop_mode(AudioStreamWAV::LOOP_FORWARD);
        return s;
    }
    return Ref<AudioStream>();
}

void GodotAudioBackend::playMusic(const nlb::presentation::MediaRef& track, bool loop, int) {
    Ref<AudioStream> s = load(track, loop);
    if (s.is_null()) return;
    m_music->stop();
    m_music->set_stream(s);
    // Узел ещё не в дереве сцены (книга открыта до _ready) — заиграет, когда попадёт в него
    if (m_music->is_inside_tree()) m_music->play();
    else m_music->set_autoplay(true);
}

void GodotAudioBackend::stopMusic(int) {
    m_music->set_autoplay(false);
    m_music->stop();
}

void GodotAudioBackend::playEffect(const nlb::presentation::MediaRef& sound) {
    Ref<AudioStream> s = load(sound, false);
    if (s.is_null() || m_effects.empty()) return;
    AudioStreamPlayer* player = nullptr;
    for (AudioStreamPlayer* p : m_effects) {
        if (!p->is_playing()) { player = p; break; }
    }
    if (!player) { player = m_effects[m_next]; m_next = (m_next + 1) % m_effects.size(); }
    player->set_stream(s);
    if (player->is_inside_tree()) player->play();
}

void GodotAudioBackend::stopEffects() {
    for (AudioStreamPlayer* p : m_effects) p->stop();
}

void GodotAudioBackend::setMusicVolume(float volume) {
    m_music->set_volume_db(volume <= 0.0001f ? -80.0f : 20.0f * Math::log(volume) / Math::log(10.0f));
}

void GodotAudioBackend::setEffectsVolume(float volume) {
    const float db = volume <= 0.0001f ? -80.0f : 20.0f * Math::log(volume) / Math::log(10.0f);
    for (AudioStreamPlayer* p : m_effects) p->set_volume_db(db);
}
