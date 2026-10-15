#include "AudioDecoder.h"

#include <algorithm>
#include <cctype>

// Реализации однофайловых декодеров — только здесь, в одной единице трансляции.
// Предупреждения сторонних библиотек не касаются кода плеера
#if defined(_MSC_VER)
#pragma warning(push, 0)
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-value"
#pragma GCC diagnostic ignored "-Wsign-compare"
#endif
#define STB_VORBIS_NO_STDIO
#define STB_VORBIS_NO_PUSHDATA_API
#include "audiodecoders/stb_vorbis.c"
#undef L
#undef C
#undef R
#define DR_MP3_IMPLEMENTATION
#define DR_MP3_NO_STDIO
#include "audiodecoders/dr_mp3.h"
#define DR_WAV_IMPLEMENTATION
#define DR_WAV_NO_STDIO
#include "audiodecoders/dr_wav.h"
#if defined(_MSC_VER)
#pragma warning(pop)
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

namespace nlb::audio {

namespace {

class VorbisDecoder : public AudioDecoder {
public:
    bool init(std::shared_ptr<const std::vector<uint8_t>> data) {
        m_data = std::move(data);
        int error = 0;
        m_vorbis = stb_vorbis_open_memory(m_data->data(), static_cast<int>(m_data->size()), &error, nullptr);
        if (!m_vorbis) return false;
        const stb_vorbis_info info = stb_vorbis_get_info(m_vorbis);
        m_channels = info.channels;
        m_sampleRate = static_cast<int>(info.sample_rate);
        m_lengthFrames = stb_vorbis_stream_length_in_samples(m_vorbis);
        return m_channels > 0 && m_sampleRate > 0;
    }
    ~VorbisDecoder() override {
        if (m_vorbis) stb_vorbis_close(m_vorbis);
    }
    size_t read(int16_t* out, size_t frames) override {
        size_t done = 0;
        while (done < frames) {
            const int got = stb_vorbis_get_samples_short_interleaved(
                m_vorbis, m_channels, out + done * m_channels, static_cast<int>((frames - done) * m_channels));
            if (got <= 0) break;
            done += static_cast<size_t>(got);
        }
        return done;
    }
    bool rewind() override { return stb_vorbis_seek_start(m_vorbis) != 0; }

private:
    stb_vorbis* m_vorbis = nullptr;
};

class Mp3Decoder : public AudioDecoder {
public:
    bool init(std::shared_ptr<const std::vector<uint8_t>> data) {
        m_data = std::move(data);
        if (!drmp3_init_memory(&m_mp3, m_data->data(), m_data->size(), nullptr)) return false;
        m_open = true;
        m_channels = static_cast<int>(m_mp3.channels);
        m_sampleRate = static_cast<int>(m_mp3.sampleRate);
        m_lengthFrames = drmp3_get_pcm_frame_count(&m_mp3);  // проходит файл и возвращается в начало
        return m_channels > 0 && m_sampleRate > 0;
    }
    ~Mp3Decoder() override {
        if (m_open) drmp3_uninit(&m_mp3);
    }
    size_t read(int16_t* out, size_t frames) override {
        return static_cast<size_t>(drmp3_read_pcm_frames_s16(&m_mp3, frames, out));
    }
    bool rewind() override { return drmp3_seek_to_pcm_frame(&m_mp3, 0) != 0; }

private:
    drmp3 m_mp3{};
    bool m_open = false;
};

class WavDecoder : public AudioDecoder {
public:
    bool init(std::shared_ptr<const std::vector<uint8_t>> data) {
        m_data = std::move(data);
        if (!drwav_init_memory(&m_wav, m_data->data(), m_data->size(), nullptr)) return false;
        m_open = true;
        m_channels = static_cast<int>(m_wav.channels);
        m_sampleRate = static_cast<int>(m_wav.sampleRate);
        m_lengthFrames = m_wav.totalPCMFrameCount;
        return m_channels > 0 && m_sampleRate > 0;
    }
    ~WavDecoder() override {
        if (m_open) drwav_uninit(&m_wav);
    }
    size_t read(int16_t* out, size_t frames) override {
        return static_cast<size_t>(drwav_read_pcm_frames_s16(&m_wav, frames, out));
    }
    bool rewind() override { return drwav_seek_to_pcm_frame(&m_wav, 0) != 0; }

private:
    drwav m_wav{};
    bool m_open = false;
};

std::string extensionOf(const std::string& name) {
    const auto dot = name.find_last_of('.');
    std::string ext = dot == std::string::npos ? "" : name.substr(dot + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext;
}

template <typename D>
std::unique_ptr<AudioDecoder> make(std::shared_ptr<const std::vector<uint8_t>> data) {
    auto d = std::make_unique<D>();
    if (!d->init(std::move(data))) return nullptr;
    return d;
}

}  // namespace

std::unique_ptr<AudioDecoder> AudioDecoder::open(const std::string& fileName,
                                                 std::shared_ptr<const std::vector<uint8_t>> data) {
    if (!data || data->empty()) return nullptr;
    const std::string ext = extensionOf(fileName);
    if (ext == "ogg") return make<VorbisDecoder>(std::move(data));
    if (ext == "mp3") return make<Mp3Decoder>(std::move(data));
    if (ext == "wav") return make<WavDecoder>(std::move(data));
    return nullptr;
}

std::vector<int16_t> AudioDecoder::decodeAll() {
    std::vector<int16_t> pcm;
    if (m_lengthFrames > 0) pcm.reserve(static_cast<size_t>(m_lengthFrames) * m_channels);
    std::vector<int16_t> chunk(4096 * static_cast<size_t>(m_channels));
    for (;;) {
        const size_t got = read(chunk.data(), 4096);
        if (got == 0) break;
        pcm.insert(pcm.end(), chunk.begin(), chunk.begin() + static_cast<long>(got * m_channels));
    }
    return pcm;
}

}  // namespace nlb::audio
