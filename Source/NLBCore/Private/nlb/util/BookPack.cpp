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

struct Reader {
    const std::string& data;
    size_t pos = 0;
    bool ok = true;
    uint64_t u(int bytes) {
        if (pos + bytes > data.size()) { ok = false; return 0; }
        uint64_t v = 0;
        for (int i = 0; i < bytes; ++i) v |= static_cast<uint64_t>(static_cast<unsigned char>(data[pos + i])) << (8 * i);
        pos += bytes;
        return v;
    }
    std::string str() {
        const uint64_t n = u(4);
        if (!ok || pos + n > data.size()) { ok = false; return {}; }
        std::string s = data.substr(pos, n);
        pos += n;
        return s;
    }
};
}

std::unique_ptr<BookPack> BookPack::build(const std::string& bookDir, std::string& error, bool withMedia,
                                          const std::string& themeDir) {
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
            if (!FileUtils::readFile(FileUtils::combinePath(bookDir, rel), entry.content)) {
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
    // Тема игры (оформление INSTEAD-экспорта) — целиком, с содержимым: шрифты и картинки темы
    // нужны проигрывателю, а в книге NLB их нет
    if (!themeDir.empty()) {
        if (!FileUtils::isDirectory(themeDir)) {
            error = "нет каталога темы " + themeDir;
            return nullptr;
        }
        const std::string prefix = THEME_DIR;
        pack->m_entries[prefix] = Entry{Entry::Type::Directory, 0, 0, {}};
        const bool themeOk = walk(themeDir, "", [&](const std::string& rel, bool isDir, uint64_t size, int64_t mtime) {
            Entry entry;
            entry.mtime = mtime;
            entry.size = isDir ? 0 : size;
            entry.type = isDir ? Entry::Type::Directory : Entry::Type::File;
            if (!isDir && !FileUtils::readFile(FileUtils::combinePath(themeDir, rel), entry.content)) {
                error = "не удалось прочитать файл темы " + rel;
                return false;
            }
            pack->m_entries[prefix + "/" + rel] = std::move(entry);
            return true;
        });
        if (!themeOk) {
            if (error.empty()) error = "ошибка обхода каталога темы " + themeDir;
            return nullptr;
        }
    }
    pack->index();
    return pack;
}

bool BookPack::write(const std::string& packPath, std::string& error) const {
    std::string out(MAGIC, sizeof(MAGIC));
    putU32(out, VERSION);
    putStr(out, m_sourceDir);
    putU64(out, m_entries.size());
    for (const auto& [rel, e] : m_entries) {
        out += static_cast<char>(e.type);
        putStr(out, rel);
        putU64(out, e.size);
        putU64(out, static_cast<uint64_t>(e.mtime));
        if (e.type == Entry::Type::File) putStr(out, e.content);
    }
    const std::string tmp = packPath + ".tmp";
    {
        std::ofstream f(FileUtils::nativePath(tmp), std::ios::binary | std::ios::trunc);
        if (!f || !f.write(out.data(), static_cast<std::streamsize>(out.size()))) {
            error = "не удалось записать " + packPath;
            return false;
        }
    }
    if (!FileUtils::renameReplace(tmp, packPath)) {
        error = "не удалось записать " + packPath;
        return false;
    }
    return true;
}

std::unique_ptr<BookPack> BookPack::read(const std::string& packPath, std::string& error) {
    std::ifstream f(FileUtils::nativePath(packPath), std::ios::binary);
    if (!f) { error = "не удалось открыть " + packPath; return nullptr; }
    const std::string data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if (data.size() < sizeof(MAGIC) || data.compare(0, sizeof(MAGIC), std::string(MAGIC, sizeof(MAGIC))) != 0) {
        error = "это не пакет книги NLB";
        return nullptr;
    }
    Reader r{data, sizeof(MAGIC)};
    if (r.u(4) != VERSION) { error = "неподдерживаемая версия пакета"; return nullptr; }
    auto pack = std::make_unique<BookPack>();
    pack->m_sourceDir = r.str();
    const uint64_t count = r.u(8);
    for (uint64_t i = 0; i < count && r.ok; ++i) {
        Entry e;
        e.type = static_cast<Entry::Type>(r.u(1));
        const std::string rel = r.str();
        e.size = r.u(8);
        e.mtime = static_cast<int64_t>(r.u(8));
        if (e.type == Entry::Type::File) e.content = r.str();
        pack->m_entries[rel] = std::move(e);
    }
    if (!r.ok) { error = "пакет повреждён"; return nullptr; }
    pack->index();
    return pack;
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
