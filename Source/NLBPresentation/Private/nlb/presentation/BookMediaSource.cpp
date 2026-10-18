#include "nlb/presentation/BookMediaSource.h"

#include "nlb/util/BookPack.h"
#include "nlb/util/FileUtils.h"

namespace nlb::presentation {

BookMediaSource::BookMediaSource(std::string bookRoot) : m_root(std::move(bookRoot)) {}

void BookMediaSource::buildIndex(const std::string& dir, int depth) const {
    for (const char* media : {"images", "sound"}) {
        const std::string mediaDir = FileUtils::combinePath(dir, media);
        if (!FileUtils::isDirectory(mediaDir)) continue;
        for (const auto& name : FileUtils::getDirectoryFiles(mediaDir)) {
            const std::string path = FileUtils::combinePath(mediaDir, name);
            m_index.emplace(name, path);                           // книга раньше модулей
            m_index.emplace(std::string(media) + "/" + name, path);
        }
    }
    const std::string modules = FileUtils::combinePath(dir, "modules");
    if (depth < 8 && FileUtils::isDirectory(modules)) {
        for (const auto& module : FileUtils::getDirectoryFiles(modules)) {
            buildIndex(FileUtils::combinePath(modules, module), depth + 1);
        }
    }
}

std::string BookMediaSource::resolve(const MediaRef& ref) const {
    if (ref.path.empty()) return "";
    // полный путь (звуки и картинки темы, в том числе из пакета) — как есть
    if (ref.path.find_first_of("/\\") != std::string::npos && FileUtils::exists(ref.path) && !FileUtils::isDirectory(ref.path)) {
        return ref.path;
    }
    if (!m_indexed) {
        buildIndex(m_root, 0);
        m_indexed = true;
    }
    auto it = m_index.find(ref.path);
    if (it == m_index.end()) {
        const auto slash = ref.path.find_last_of('/');
        if (slash != std::string::npos) it = m_index.find(ref.path.substr(slash + 1));
    }
    return it == m_index.end() ? std::string() : it->second;
}

bool BookMediaSource::exists(const MediaRef& ref) const {
    return !resolve(ref).empty();
}

bool BookMediaSource::read(const MediaRef& ref, std::vector<uint8_t>& data) const {
    const std::string path = resolve(ref);
    std::string content;
    if (path.empty() || !FileUtils::readFile(path, content)) return false;
    data.assign(content.begin(), content.end());
    return true;
}

std::string BookMediaSource::filePath(const MediaRef& ref) const {
    const std::string path = resolve(ref);
    std::string rel;
    // Файл внутри смонтированного пакета на диске не лежит — только read()
    if (path.empty() || BookPack::resolve(path, rel)) return "";
    return path;
}

}  // namespace nlb::presentation
