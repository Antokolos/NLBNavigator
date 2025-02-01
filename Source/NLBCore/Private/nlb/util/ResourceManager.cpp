#include "nlb/util/ResourceManager.h"

const std::string ResourceManager::VNSTEAD = "vnstead";

void ResourceManager::exportBundledFiles(const std::string& targetDir) throw(NLBIOException) {
    void* loader = nullptr; // Thread::currentThread().getContextClassLoader() equivalent would be needed
    std::map<std::string, std::vector<std::string>> resourceFolderFiles = getResourceFolderFiles(loader);
    
    for (const auto& resourceFileEntry : resourceFolderFiles) {
        exportBundledFiles(resourceFileEntry, targetDir);
    }
}

std::map<std::string, std::vector<std::string>> ResourceManager::getResourceFolderFiles(void* loader) throw(NLBIOException) {
    std::string resDir = "res";
    if (!FileUtils::exists(resDir)) {
        throw NLBIOException("Resources dir 'res' does not exist!");
    }
    
    return getAllChildren(FileUtils::combinePath(resDir, VNSTEAD), "");
}

std::map<std::string, std::vector<std::string>> ResourceManager::getAllChildren(const std::string& parent, const std::string& parentPath) {
    std::map<std::string, std::vector<std::string>> result;
    std::vector<std::string> files = FileUtils::getDirectoryFiles(parent);
    
    for (const std::string& file : files) {
        std::string fullPath = FileUtils::combinePath(parent, file);
        
        if (FileUtils::isDirectory(fullPath)) {
            std::string key = StringHelper::notEmpty(parentPath) ? 
                FileUtils::combinePath(parentPath, file) : file;
            std::map<std::string, std::vector<std::string>> children = getAllChildren(fullPath, key);
            result.insert(children.begin(), children.end());
        } else {
            std::vector<std::string>& filesList = result[parentPath];
            filesList.push_back(fullPath);
        }
    }
    
    return result;
}

void ResourceManager::exportBundledFiles(
    const std::pair<std::string, std::vector<std::string>>& resourceFileEntry,
    const std::string& targetDir) throw(NLBIOException) {
    
    const std::string& key = resourceFileEntry.first;
    bool hasParentFolder = StringHelper::notEmpty(key);
    std::string resourceFileParent = hasParentFolder ? FileUtils::combinePath(targetDir, key) : targetDir;
    
    if (hasParentFolder) {
        if (!FileUtils::createDirectory(resourceFileParent)) {
            throw NLBIOException("Failed to create directory: " + resourceFileParent);
        }
    }
    
    for (const std::string& resourceFile : resourceFileEntry.second) {
        exportBundledFile(resourceFileParent, resourceFile);
    }
}

void ResourceManager::exportBundledFile(
    const std::string& resourceFileParent,
    const std::string& resourceFile) throw(NLBIOException) {
    
    std::string fileName = FileUtils::getFileName(resourceFile);
    std::string targetFile = FileUtils::combinePath(resourceFileParent, fileName);
    
    try {
        std::ifstream is(resourceFile.c_str(), std::ios::binary);
        if (!is) {
            throw NLBIOException("Could not open input file: " + resourceFile);
        }
        FileManipulator::writeFile(targetFile, is);
        is.close();
    } catch (const std::exception& e) {
        throw NLBIOException("Error exporting bundled file", e.what());
    }
}