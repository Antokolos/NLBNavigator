#include "nlb/util/BookPack.h"

#include "nlb/util/FileUtils.h"

#include <algorithm>
#include <fstream>


namespace {
const char MAGIC[8] = {'N', 'L', 'B', 'P', 'A', 'C', 'K', '\n'};
const uint32_t VERSION = 2;  // 2: времена изменения из WinAPI/stat вместо std::filesystem

struct Mount {
    std::shared_ptr<BookPack> pack;
    std::string root;  ///< нормализованный корень: '/' вместо '\', без хвостового '/'
};
Mount g_mount;

std::string normalized(std::string p) {
    std::replace(p.begin(), p.end(), '\\', '/');
    std::string r;
    for (char c : p) {
        if (c == '/' && !r.empty() && r.back() == '/') continue;  // "a//b" -> "a/b"
        r += c;
    }
    while (r.size() > 1 && r.back() == '/') r.pop_back();
    return r;
}

/// Каталоги медиафайлов в книге и её модулях: содержимое плееру не нужно
bool isMediaPath(const std::string& rel) {
    const auto inDir = [&](const char* dir) {
        const std::string d = dir;
        return rel.compare(0, d.size() + 1, d + "/") == 0 || rel.find("/" + d + "/") != std::string::npos;
    };
    return inDir("images") || inDir("sound");
}

/// Скрытые каталоги и файлы (.git, .idea и т.п.) к книге не относятся и в пакет не попадают
bool isHidden(const std::string& name) {
    return !name.empty() && name[0] == '.';
}

/// Обход каталога книги: для каждого элемента — относительный путь и метаданные.
/// Только через FileUtils (WinAPI в Windows): std::filesystem не понимает префикс «\\?\».
template <typename Visit>
bool walk(const std::string& root, const std::string& rel, Visit&& visit) {
    const std::string dir = rel.empty() ? root : FileUtils::combinePath(root, rel);
    for (const std::string& name : FileUtils::getDirectoryFiles(dir)) {
        if (isHidden(name)) continue;
        const std::string childRel = rel.empty() ? name : rel + "/" + name;
        bool isDir = false;
        uint64_t size = 0;
        int64_t mtime = 0;
        if (!FileUtils::fileInfo(FileUtils::combinePath(root, childRel), isDir, size, mtime)) return false;
        if (!visit(childRel, isDir, size, mtime)) return false;
        if (isDir && !walk(root, childRel, visit)) return false;
    }
    return true;
}

void putU32(std::string& out, uint32_t v) { for (int i = 0; i < 4; ++i) out += static_cast<char>((v >> (8 * i)) & 0xFF); }
void putU64(std::string& out, uint64_t v) { for (int i = 0; i < 8; ++i) out += static_cast<char>((v >> (8 * i)) & 0xFF); }
void putStr(std::string& out, const std::string& s) { putU32(out, static_cast<uint32_t>(s.size())); out += s; }

}

std::unique_ptr<BookPack> BookPack::build(const std::string& bookDir, std::string& error, bool withMedia,
                                          const std::string& themeDir, const std::string& baseThemeDir) {
    auto pack = std::make_unique<BookPack>();
    pack->m_sourceDir = bookDir;
    if (!FileUtils::isDirectory(bookDir)) {
        error = "нет каталога книги " + bookDir;
        return nullptr;
    }
    pack->m_entries[""] = Entry{Entry::Type::Directory, 0, 0, {}};
    const bool ok = walk(bookDir, "", [&](const std::string& rel, bool isDir, uint64_t size, int64_t mtime) {
        Entry entry;
        entry.mtime = mtime;
        entry.size = isDir ? 0 : size;
        if (isDir) {
            entry.type = Entry::Type::Directory;
        } else if (isMediaPath(rel) && !withMedia) {
            entry.type = Entry::Type::MediaFile;
        } else {
            entry.type = Entry::Type::File;
            entry.length = size;
            if (size > LAZY_THRESHOLD) {
                entry.sourceFile = FileUtils::combinePath(bookDir, rel);  // прочитается при записи
            } else if (!FileUtils::readFile(FileUtils::combinePath(bookDir, rel), entry.content)) {
                error = "не удалось прочитать " + rel;
                return false;
            }
        }
        pack->m_entries[rel] = std::move(entry);
        return true;
    });
    if (!ok) {
        if (error.empty()) error = "ошибка обхода каталога книги " + bookDir;
        return nullptr;
    }
    // Тема игры и базовая тема (оформление INSTEAD) — целиком, с содержимым: шрифты и картинки
    // тем нужны проигрывателю, а в книге NLB их нет
    for (const auto& [dirToPack, prefixName] : {std::pair<std::string, const char*>{themeDir, THEME_DIR}, {baseThemeDir, BASE_THEME_DIR}}) {
        if (dirToPack.empty()) continue;
        if (!FileUtils::isDirectory(dirToPack)) {
            error = "нет каталога темы " + dirToPack;
            return nullptr;
        }
        const std::string prefix = prefixName;
        pack->m_entries[prefix] = Entry{Entry::Type::Directory, 0, 0, {}};
        const bool themeOk = walk(dirToPack, "", [&](const std::string& rel, bool isDir, uint64_t size, int64_t mtime) {
            Entry entry;
            entry.mtime = mtime;
            entry.size = isDir ? 0 : size;
            entry.type = isDir ? Entry::Type::Directory : Entry::Type::File;
            entry.length = isDir ? 0 : size;
            if (!isDir && size > LAZY_THRESHOLD) {
                entry.sourceFile = FileUtils::combinePath(dirToPack, rel);
            } else if (!isDir && !FileUtils::readFile(FileUtils::combinePath(dirToPack, rel), entry.content)) {
                error = "не удалось прочитать файл темы " + rel;
                return false;
            }
            pack->m_entries[prefix + "/" + rel] = std::move(entry);
            return true;
        });
        if (!themeOk) {
            if (error.empty()) error = "ошибка обхода каталога темы " + dirToPack;
            return nullptr;
        }
    }
    pack->index();
    return pack;
}

bool BookPack::write(const std::string& packPath, std::string& error) const {
    const std::string tmp = packPath + ".tmp";
    {
        std::ofstream f(FileUtils::nativePath(tmp), std::ios::binary | std::ios::trunc);
        if (!f) { error = "не удалось записать " + packPath; return false; }
        std::string head(MAGIC, sizeof(MAGIC));
        putU32(head, VERSION);
        putStr(head, m_sourceDir);
        putU64(head, m_entries.size());
        f.write(head.data(), static_cast<std::streamsize>(head.size()));
        std::string content;
        for (const auto& [rel, e] : m_entries) {
            std::string rec;
            rec += static_cast<char>(e.type);
            putStr(rec, rel);
            putU64(rec, e.size);
            putU64(rec, static_cast<uint64_t>(e.mtime));
            if (e.type == Entry::Type::File) {
                // содержимое: в памяти, в исходном файле (сборка) или в прежнем пакете (lazy)
                const std::string* data = &e.content;
                if (!e.sourceFile.empty()) {
                    if (!FileUtils::readFile(e.sourceFile, content)) { error = "не удалось прочитать " + e.sourceFile; return false; }
                    data = &content;
                } else if (e.lazy) {
                    if (!readContent(e, content)) { error = "не удалось прочитать " + rel + " из пакета"; return false; }
                    data = &content;
                }
                putU32(rec, static_cast<uint32_t>(data->size()));
                f.write(rec.data(), static_cast<std::streamsize>(rec.size()));
                f.write(data->data(), static_cast<std::streamsize>(data->size()));
            } else {
                f.write(rec.data(), static_cast<std::streamsize>(rec.size()));
            }
            if (!f) { error = "не удалось записать " + packPath; return false; }
        }
    }
    if (!FileUtils::renameReplace(tmp, packPath)) {
        error = "не удалось записать " + packPath;
        return false;
    }
    return true;
}

namespace {
/// Чтение полей пакета из потока (тот же формат, что пишет write)
struct StreamReader {
    std::istream& in;
    bool ok = true;
    uint64_t u(int bytes) {
        unsigned char buf[8];
        if (!in.read(reinterpret_cast<char*>(buf), bytes)) { ok = false; return 0; }
        uint64_t v = 0;
        for (int i = 0; i < bytes; ++i) v |= static_cast<uint64_t>(buf[i]) << (8 * i);
        return v;
    }
    std::string str() {
        const uint64_t n = u(4);
        std::string s(static_cast<size_t>(ok ? n : 0), '\0');
        if (ok && n && !in.read(&s[0], static_cast<std::streamsize>(n))) ok = false;
        return s;
    }
};
}

std::unique_ptr<BookPack> BookPack::read(const std::string& packPath, std::string& error) {
    // Читаем только описание дерева и мелкие файлы; крупные (медиа, шрифты) — пропускаем,
    // запоминая место: их прочитает readContent при первом обращении
    std::ifstream f(FileUtils::nativePath(packPath), std::ios::binary);
    if (!f) { error = "не удалось открыть " + packPath; return nullptr; }
    std::vector<char> buffer(1 << 20);
    f.rdbuf()->pubsetbuf(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    char magic[sizeof(MAGIC)];
    if (!f.read(magic, sizeof(MAGIC)) || std::string(magic, sizeof(MAGIC)) != std::string(MAGIC, sizeof(MAGIC))) {
        error = "это не пакет книги NLB";
        return nullptr;
    }
    StreamReader r{f};
    if (r.u(4) != VERSION) { error = "неподдерживаемая версия пакета"; return nullptr; }
    auto pack = std::make_unique<BookPack>();
    pack->m_packPath = packPath;
    pack->m_sourceDir = r.str();
    const uint64_t count = r.u(8);
    for (uint64_t i = 0; i < count && r.ok; ++i) {
        Entry e;
        e.type = static_cast<Entry::Type>(r.u(1));
        const std::string rel = r.str();
        e.size = r.u(8);
        e.mtime = static_cast<int64_t>(r.u(8));
        if (e.type == Entry::Type::File && r.ok) {
            e.length = r.u(4);
            if (e.length > LAZY_THRESHOLD) {
                e.lazy = true;
                e.offset = static_cast<uint64_t>(f.tellg());
                f.seekg(static_cast<std::streamoff>(e.length), std::ios::cur);
                if (!f) r.ok = false;
            } else if (r.ok) {
                e.content.resize(static_cast<size_t>(e.length));
                if (e.length && !f.read(&e.content[0], static_cast<std::streamsize>(e.length))) r.ok = false;
            }
        }
        pack->m_entries[rel] = std::move(e);
    }
    if (!r.ok) { error = "пакет повреждён"; return nullptr; }
    pack->index();
    return pack;
}

bool BookPack::readContent(const std::string& rel, std::string& out) const {
    const Entry* e = find(rel);
    if (!e || e->type != Entry::Type::File) return false;
    return readContent(*e, out);
}

bool BookPack::readContent(const Entry& e, std::string& out) const {
    if (!e.lazy) {
        out = e.content;
        return true;
    }
    // отложенное чтение: файл пакета открыт один раз; звук может читать другой поток
    std::lock_guard<std::mutex> lock(m_ioMutex);
    if (!m_file.is_open()) {
        m_file.open(FileUtils::nativePath(m_packPath), std::ios::binary);
        if (!m_file) return false;
    }
    m_file.clear();
    m_file.seekg(static_cast<std::streamoff>(e.offset));
    out.resize(static_cast<size_t>(e.length));
    return e.length == 0 || static_cast<bool>(m_file.read(&out[0], static_cast<std::streamsize>(e.length)));
}

void BookPack::index() {
    m_children.clear();
    for (const auto& [rel, e] : m_entries) {
        if (rel.empty()) continue;
        const auto slash = rel.rfind('/');
        m_children[slash == std::string::npos ? "" : rel.substr(0, slash)]
            .push_back(slash == std::string::npos ? rel : rel.substr(slash + 1));
    }
}

bool BookPack::matches(const std::string& bookDir) const {
    size_t seen = 1;  // корень
    const bool ok = walk(bookDir, "", [&](const std::string& rel, bool isDir, uint64_t size, int64_t mtime) {
        auto found = m_entries.find(rel);
        if (found == m_entries.end() || found->second.mtime != mtime) return false;
        if (!isDir && found->second.size != size) return false;
        ++seen;
        return true;
    });
    return ok && seen == m_entries.size();
}

void BookPack::mount(std::shared_ptr<BookPack> pack, const std::string& virtualRoot) {
    g_mount.pack = std::move(pack);
    g_mount.root = normalized(virtualRoot);
}

void BookPack::unmount() {
    g_mount = Mount{};
}

BookPack* BookPack::mounted() {
    return g_mount.pack.get();
}

bool BookPack::resolve(const std::string& path, std::string& rel) {
    if (!g_mount.pack) return false;
    const std::string p = normalized(path);
    const std::string& root = g_mount.root;
    if (p == root) { rel.clear(); return true; }
    if (p.size() > root.size() && p.compare(0, root.size(), root) == 0 && p[root.size()] == '/') {
        rel = p.substr(root.size() + 1);
        return true;
    }
    return false;
}

const BookPack::Entry* BookPack::find(const std::string& rel) const {
    auto it = m_entries.find(rel);
    return it == m_entries.end() ? nullptr : &it->second;
}

std::vector<std::string> BookPack::children(const std::string& rel) const {
    auto it = m_children.find(rel);
    return it == m_children.end() ? std::vector<std::string>{} : it->second;
}

size_t BookPack::fileCount() const {
    return static_cast<size_t>(std::count_if(m_entries.begin(), m_entries.end(),
        [](const auto& kv) { return kv.second.type != Entry::Type::Directory; }));
}
