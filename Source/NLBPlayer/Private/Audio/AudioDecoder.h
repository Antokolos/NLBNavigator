#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace nlb::audio {

/**
 * @brief Потоковый декодер звука из памяти: ogg (stb_vorbis), mp3 (dr_mp3), wav (dr_wav).
 *
 * Выдаёт 16-битный PCM с чередованием каналов порциями — для музыки, которая декодируется по
 * мере проигрывания, — или целиком (decodeAll) для коротких эффектов. Не зависит от Unreal.
 * Данные файла держит сам (shared_ptr), поэтому декодер можно отдать в поток звука.
 */
class AudioDecoder {
public:
    virtual ~AudioDecoder() = default;

    /// Декодер по расширению имени файла (".ogg", ".mp3", ".wav"); nullptr — формат не поддержан
    /// или файл повреждён
    static std::unique_ptr<AudioDecoder> open(const std::string& fileName,
                                              std::shared_ptr<const std::vector<uint8_t>> data);

    int channels() const { return m_channels; }
    int sampleRate() const { return m_sampleRate; }
    /// Длина в кадрах (кадр — по одному отсчёту на канал); 0 — неизвестна
    uint64_t lengthFrames() const { return m_lengthFrames; }
    double lengthSeconds() const { return m_sampleRate > 0 ? static_cast<double>(m_lengthFrames) / m_sampleRate : 0.0; }

    /// Прочитать до frames кадров в out (frames * channels отсчётов); 0 — конец
    virtual size_t read(int16_t* out, size_t frames) = 0;
    /// В начало (для зацикленной музыки)
    virtual bool rewind() = 0;

    /// Весь звук целиком (для коротких эффектов)
    std::vector<int16_t> decodeAll();

protected:
    std::shared_ptr<const std::vector<uint8_t>> m_data;
    int m_channels = 0;
    int m_sampleRate = 0;
    uint64_t m_lengthFrames = 0;
};

}  // namespace nlb::audio
