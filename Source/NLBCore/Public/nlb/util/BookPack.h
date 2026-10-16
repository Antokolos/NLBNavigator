#pragma once

#include <cstdint>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
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
 * Крупные файлы (картинки и звук в пакете --with-media, шрифты темы) при чтении пакета не
 * загружаются: запоминается их место в файле, а содержимое читается при первом обращении
 * (readContent). Поэтому пакет с медиа открывается так же быстро, как без них, и не держит
 * сотни мегабайт в памяти.
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
        std::string content;        ///< содержимое (Type::File; у крупных файлов — пусто, см. lazy)
        bool lazy = false;          ///< содержимое в файле пакета: offset, length — читается по запросу
        uint64_t offset = 0;
        uint64_t length = 0;
        std::string sourceFile;     ///< build: файл, откуда взять содержимое при записи пакета
    };

    /// Файлы крупнее этого не загружаются при чтении пакета (и не читаются при сборке до записи)
    static constexpr uint64_t LAZY_THRESHOLD = 64 * 1024;

    /// Содержимое файла пакета (с диска, если оно не загружено); false — нет такого файла
    bool readContent(const std::string& rel, std::string& out) const;

    /// Упаковать каталог книги (путь в UTF-8). Медиафайлы — без содержимого, если не withMedia
    /// (с медиа пакет самодостаточен: картинки и звук берутся прямо из него)
    static std::unique_ptr<BookPack> build(const std::string& bookDir, std::string& error, bool withMedia = false,
                                           const std::string& themeDir = "", const std::string& baseThemeDir = "");
    /// Каталог внутри пакета, куда кладётся тема игры (theme.ini, theme_*.lua, fonts/, gfx/):
    /// <пакет>/__theme__/...
    static constexpr const char* THEME_DIR = "__theme__";
    /// Каталог внутри пакета для базовой темы (оформление по умолчанию): <пакет>/__base_theme__/...
    static constexpr const char* BASE_THEME_DIR = "__base_theme__";
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
    bool readContent(const Entry& e, std::string& out) const;
    std::string m_sourceDir;
    std::string m_packPath;                   ///< откуда прочитан пакет (для отложенного чтения)
    mutable std::mutex m_ioMutex;
    mutable std::ifstream m_file;
    std::map<std::string, Entry> m_entries;   ///< "" — корень, "pages/<id>/text/ru" и т.п.
    std::map<std::string, std::vector<std::string>> m_children;
    void index();
};
