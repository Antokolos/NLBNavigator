#include "nlb/util/BookPack.h"

#include "nlb/util/FileUtils.h"

#include <algorithm>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace {
const char MAGIC[8] = {'N', 'L', 'B', 'P', 'A', 'C', 'K', '\n'};
const uint32_t VERSION = 1;

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
bool isHidden(const fs::directory_entry& e) {
    const std::string name = e.path().filename().u8string();
    return !name.empty() && name[0] == '.';
}

int64_t mtimeOf(const fs::directory_entry& e) {
    std::error_code ec;
    const auto t = e.last_write_time(ec);
    return ec ? 0 : static_cast<int64_t>(t.time_since_epoch().count());
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

std::unique_ptr<BookPack> BookPack::build(const std::string& bookDir, std::string& error) {
    auto pack = std::make_unique<BookPack>();
    pack->m_sourceDir = bookDir;
    const fs::path root = FileUtils::nativePath(bookDir);
    std::error_code ec;
    if (!fs::is_directory(root, ec)) {
        error = "нет каталога книги " + bookDir;
        return nullptr;
    }
    pack->m_entries[""] = Entry{Entry::Type::Directory, 0, 0, {}};
    for (auto it = fs::recursive_directory_iterator(root, ec); !ec && it != fs::recursive_directory_iterator(); it.increment(ec)) {
        const fs::directory_entry& e = *it;
        if (isHidden(e)) {
            if (e.is_directory(ec)) it.disable_recursion_pending();
            continue;
        }
        const std::string rel = normalized(fs::relative(e.path(), root, ec).u8string());
        if (ec) break;
        Entry entry;
        entry.mtime = mtimeOf(e);
        if (e.is_directory(ec)) {
            entry.type = Entry::Type::Directory;
        } else {
            entry.size = e.file_size(ec);
            if (isMediaPath(rel)) {
                entry.type = Entry::Type::MediaFile;
            } else {
                entry.type = Entry::Type::File;
                std::ifstream in(e.path(), std::ios::binary);
                if (!in) { error = "не удалось прочитать " + rel; return nullptr; }
                entry.content.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
            }
        }
        pack->m_entries[rel] = std::move(entry);
    }
    if (ec) {
        error = "ошибка обхода каталога книги: " + ec.message();
        return nullptr;
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
    std::error_code ec;
    fs::rename(FileUtils::nativePath(tmp), FileUtils::nativePath(packPath), ec);
    if (ec) {
        fs::remove(FileUtils::nativePath(packPath), ec);
        fs::rename(FileUtils::nativePath(tmp), FileUtils::nativePath(packPath), ec);
    }
    if (ec) { error = "не удалось записать " + packPath; return false; }
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
    const fs::path root = FileUtils::nativePath(bookDir);
    std::error_code ec;
    size_t seen = 1;  // корень
    for (auto it = fs::recursive_directory_iterator(root, ec); !ec && it != fs::recursive_directory_iterator(); it.increment(ec)) {
        const fs::directory_entry& e = *it;
        if (isHidden(e)) {
            if (e.is_directory(ec)) it.disable_recursion_pending();
            continue;
        }
        const std::string rel = normalized(fs::relative(e.path(), root, ec).u8string());
        auto found = m_entries.find(rel);
        if (ec || found == m_entries.end() || found->second.mtime != mtimeOf(e)) return false;
        if (found->second.type != Entry::Type::Directory && found->second.size != e.file_size(ec)) return false;
        ++seen;
    }
    return !ec && seen == m_entries.size();
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
