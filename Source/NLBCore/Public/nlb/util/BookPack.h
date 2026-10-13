#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

/**
 * @brief Упакованная книга: дерево каталога книги в одном файле (.nlbpack).
 *
 * Книга NLB — это десятки тысяч мелких файлов (Frontier — 26 723 в 10 790 каталогах), и при
 * загрузке время уходит на их открытие. Пакет хранит то же дерево целиком: каталоги, файлы
 * с содержимым, а у медиафайлов (images/, sound/) — только имя, размер и время изменения:
 * плееру нужен их список, сами картинки и звуки он не читает.
 *
 * Пока пакет «смонтирован» (mount), FileUtils::exists/isDirectory/getDirectoryFiles и
 * FileUtils::readFile для путей внутри корня книги обслуживаются из памяти, поэтому код
 * чтения книги (NonLinearBookImpl, PageImpl, ...) не меняется.
 */
class BookPack {
public:
    struct Entry {
        enum class Type : uint8_t { Directory = 0, File = 1, MediaFile = 2 };
        Type type = Type::File;
        uint64_t size = 0;
        int64_t mtime = 0;          ///< время изменения (для проверки свежести кэша)
        std::string content;        ///< содержимое (только для Type::File)
    };

    /// Упаковать каталог книги (путь в UTF-8). Медиафайлы — без содержимого, если не withMedia
    /// (с медиа пакет самодостаточен: картинки и звук берутся прямо из него)
    static std::unique_ptr<BookPack> build(const std::string& bookDir, std::string& error, bool withMedia = false);
    /// Прочитать пакет из файла
    static std::unique_ptr<BookPack> read(const std::string& packPath, std::string& error);
    bool write(const std::string& packPath, std::string& error) const;

    /// Совпадает ли пакет с каталогом книги: тот же набор файлов и каталогов, размеры и времена
    /// изменения (только метаданные, файлы не открываются)
    bool matches(const std::string& bookDir) const;

    /// Подключить пакет как каталог virtualRoot (может быть и путём к самому .nlbpack)
    static void mount(std::shared_ptr<BookPack> pack, const std::string& virtualRoot);
    static void unmount();
    static BookPack* mounted();

    // Доступ к смонтированному пакету (relPath — путь относительно корня книги, через '/')
    /// Путь внутри смонтированного корня: true и относительный путь в rel
    static bool resolve(const std::string& path, std::string& rel);
    const Entry* find(const std::string& rel) const;
    std::vector<std::string> children(const std::string& rel) const;

    size_t fileCount() const;
    const std::string& sourceDir() const { return m_sourceDir; }

private:
    std::string m_sourceDir;
    std::map<std::string, Entry> m_entries;   ///< "" — корень, "pages/<id>/text/ru" и т.п.
    std::map<std::string, std::vector<std::string>> m_children;
    void index();
};
