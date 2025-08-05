#include "nlb/util/ResourceManager.h"

const std::string ResourceManager::VNSTEAD = "vnstead";

void ResourceManager::exportBundledFiles(const std::string& targetDir) {
    try {
        // Используем nullptr для стандартного загрузчика классов
        std::map<std::string, std::vector<std::string>> resourceFolderFiles = getResourceFolderFiles(nullptr);
        
        for (const auto& resourceFileEntry : resourceFolderFiles) {
            exportBundledFiles(resourceFileEntry, targetDir);
        }
    } catch (const std::exception& e) {
        throw NLBIOException("Error exporting bundled files: " + std::string(e.what()));
    }
}

std::map<std::string, std::vector<std::string>> ResourceManager::getResourceFolderFiles(void* loader) {
    // В C++ версии мы адаптируем концепцию ресурсов Java
    // Проверяем существование ресурсной директории
    std::string resDir = "res";
    std::string vnsteadPath = FileUtils::combinePath(resDir, VNSTEAD);
    
    if (!resourceExists(vnsteadPath)) {
        // Пытаемся найти ресурсы в стандартных местах
        std::vector<std::string> possiblePaths = {
            "res/" + VNSTEAD,
            "resources/" + VNSTEAD,
            "../res/" + VNSTEAD,
            "../resources/" + VNSTEAD
        };
        
        bool found = false;
        for (const auto& path : possiblePaths) {
            if (FileUtils::exists(path)) {
                vnsteadPath = path;
                found = true;
                break;
            }
        }
        
        if (!found) {
            throw NLBIOException("Resources dir '" + VNSTEAD + "' does not exist!");
        }
    }
    
    return getAllChildren(vnsteadPath, "");
}

std::map<std::string, std::vector<std::string>> ResourceManager::getAllChildren(const std::string& parent, const std::string& parentPath) {
    std::map<std::string, std::vector<std::string>> result;
    
    if (!FileUtils::exists(parent)) {
        return result;
    }
    
    try {
        std::vector<std::string> files;
        
        // Если это ресурсная директория, используем специальную обработку
        if (resourceExists(parent)) {
            files = getResourceDirectoryListing(parent);
        } else {
            files = FileUtils::getDirectoryFiles(parent);
        }
        
        for (const std::string& file : files) {
            std::string fullPath = FileUtils::combinePath(parent, file);
            
            if (FileUtils::isDirectory(fullPath)) {
                std::string key = StringHelper::notEmpty(parentPath) ? 
                    FileUtils::combinePath(parentPath, file) : file;
                    
                std::map<std::string, std::vector<std::string>> children = getAllChildren(fullPath, key);
                result.insert(children.begin(), children.end());
            } else {
                // Добавляем файл в соответствующую категорию
                std::vector<std::string>& filesList = result[parentPath];
                filesList.push_back(fullPath);
            }
        }
    } catch (const std::exception& e) {
        throw NLBIOException("Error reading directory contents: " + std::string(e.what()));
    }
    
    return result;
}

void ResourceManager::exportBundledFiles(
    const std::pair<std::string, std::vector<std::string>>& resourceFileEntry,
    const std::string& targetDir) {
    
    const std::string& key = resourceFileEntry.first;
    const std::vector<std::string>& files = resourceFileEntry.second;
    
    bool hasParentFolder = StringHelper::notEmpty(key);
    std::string resourceFileParent = hasParentFolder ? 
        FileUtils::combinePath(targetDir, key) : targetDir;
    
    if (hasParentFolder) {
        if (!FileUtils::createDirectory(resourceFileParent)) {
            throw NLBIOException("Failed to create directory: " + resourceFileParent);
        }
    }
    
    for (const std::string& resourceFile : files) {
        exportBundledFile(resourceFileParent, resourceFile);
    }
}

void ResourceManager::exportBundledFile(
    const std::string& resourceFileParent,
    const std::string& resourceFile) {
    
    std::string fileName = FileUtils::getFileName(resourceFile);
    std::string targetFile = FileUtils::combinePath(resourceFileParent, fileName);
    
    try {
        std::ifstream is;
        
        // Пытаемся открыть как ресурс, затем как обычный файл
        if (resourceExists(resourceFile)) {
            is = getResourceAsStream(resourceFile);
        } else {
            is.open(resourceFile, std::ios::binary);
        }
        
        if (!is.is_open()) {
            throw NLBIOException("Could not open input file: " + resourceFile);
        }
        
        // Создаем целевой файл
        std::ofstream os(targetFile, std::ios::binary);
        if (!os.is_open()) {
            throw NLBIOException("Could not create output file: " + targetFile);
        }
        
        // Копируем содержимое
        os << is.rdbuf();
        
        is.close();
        os.close();
        
    } catch (const std::exception& e) {
        throw NLBIOException("Error exporting bundled file '" + resourceFile + "': " + std::string(e.what()));
    }
}

std::vector<std::string> ResourceManager::getResourceDirectoryListing(const std::string& resourcePath) {
    // В C++ адаптируем концепцию получения списка ресурсов
    // Поскольку C++ не имеет встроенной поддержки ресурсов JAR, 
    // используем файловую систему как fallback
    
    std::vector<std::string> result;
    
    if (FileUtils::exists(resourcePath)) {
        result = FileUtils::getDirectoryFiles(resourcePath);
    } else {
        // Можно добавить поддержку встроенных ресурсов через другие механизмы
        // например, через ресурсы CMake или статические массивы
    }
    
    return result;
}

bool ResourceManager::resourceExists(const std::string& resourcePath) {
    // Простая проверка существования ресурса
    // В более сложной реализации здесь может быть проверка встроенных ресурсов
    return FileUtils::exists(resourcePath);
}

std::ifstream ResourceManager::getResourceAsStream(const std::string& resourcePath) {
    std::ifstream stream(resourcePath, std::ios::binary);
    if (!stream.is_open()) {
        throw NLBIOException("Could not open resource: " + resourcePath);
    }
    return stream;
}
