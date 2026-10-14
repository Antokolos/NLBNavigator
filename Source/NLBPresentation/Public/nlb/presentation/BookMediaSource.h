#pragma once

#include <map>
#include <string>

#include "nlb/presentation/MediaSource.h"

namespace nlb::presentation {

/**
 * @brief Медиафайлы из каталога книги или из смонтированного пакета .nlbpack.
 *
 * Плеер называет медиа именем файла ("geomystery - blue shaman.ogg", "029.1.png"), а лежать
 * он может в images/ или sound/ самой книги или её внешних модулей (modules/<имя>/...).
 * При первом обращении строится индекс «имя -> путь» по всем таким каталогам; файл книги
 * важнее файла модуля. Работает и с пакетом: FileUtils обслуживает смонтированный пакет,
 * а содержимое медиа есть в пакете, собранном с ключом --with-media.
 */
class BookMediaSource : public IMediaSource {
public:
    explicit BookMediaSource(std::string bookRoot);
    bool exists(const MediaRef& ref) const override;
    bool read(const MediaRef& ref, std::vector<uint8_t>& data) const override;
    std::string filePath(const MediaRef& ref) const override;
    /// Полный путь (UTF-8) к файлу; пусто — не найден
    std::string resolve(const MediaRef& ref) const;

private:
    std::string m_root;
    mutable bool m_indexed = false;
    mutable std::map<std::string, std::string> m_index;  ///< имя или "images/имя" -> путь
    void buildIndex(const std::string& dir, int depth) const;
};

}  // namespace nlb::presentation
