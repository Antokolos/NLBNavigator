#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "nlb/presentation/ScreenModel.h"

namespace nlb::presentation {

/**
 * @brief Откуда брать медиафайлы книги. ЧЕРНОВИК ДЛЯ СОГЛАСОВАНИЯ.
 *
 * Реализации:
 *  - BookDirectoryMediaSource — каталог книги (images/, sound/, в т.ч. внешние модули);
 *  - PackMediaSource — пакет .nlbpack, собранный с медиа (pack --with-media);
 *  - в Godot — res://..., в Unreal — ассеты проекта (адаптер пишется в самом движке).
 */
class IMediaSource {
public:
    virtual ~IMediaSource() = default;
    virtual bool exists(const MediaRef& ref) const = 0;
    /// Содержимое файла целиком (картинки, короткие звуки)
    virtual bool read(const MediaRef& ref, std::vector<uint8_t>& data) const = 0;
    /// Путь в файловой системе (UTF-8), если файл лежит на диске; пусто — только через read().
    /// Нужен библиотекам, которые читают поток сами (музыка в SDL_mixer, потоковый звук)
    virtual std::string filePath(const MediaRef& ref) const = 0;
};

}  // namespace nlb::presentation
