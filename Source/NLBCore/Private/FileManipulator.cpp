#include "FileManipulator.h"
#include <filesystem>
#include <fstream>
#include <sstream>

FileManipulator::FileManipulator(std::shared_ptr<VCSAdapter> vcsAdapter, const std::string& mainRoot)
    : m_vcsAdapter(vcsAdapter), m_mainRoot(mainRoot) {}

bool FileManipulator::deleteFileOrDir(const std::string& filePath) {
    namespace fs = std::filesystem;
    try {
        fs::path path(filePath);
        if (!fs::exists(path)) {
            return false;
        }

        bool ret = true;
        if (fs::is_directory(path)) {
            for (const auto& entry : fs::directory_iterator(path)) {
                if (!deleteFileOrDir(entry.path().string())) {
                    ret = false;
                }
            }
        }

        std::string relativePath = getPathRelativeToMainRoot(filePath);
        VCSAdapter::Status status = m_vcsAdapter->getStatus(relativePath);
        bool removed = false;

        switch (status) {
            case VCSAdapter::Status::Modified:
            case VCSAdapter::Status::Clean:
                removed = m_vcsAdapter->remove(relativePath);
                break;
            case VCSAdapter::Status::Added:
                m_vcsAdapter->reset(relativePath);
                break;
            case VCSAdapter::Status::Missing:
            case VCSAdapter::Status::Ignored:
                throw NLBFileManipulationException(
                    "Incorrect file status while deleting file with path = " + 
                    relativePath + " from VCS: " + std::to_string(static_cast<int>(status))
                );
        }

        if (!removed) {
            removed = fs::remove(path);
        }
        return removed && ret;
    }
    catch (const fs::filesystem_error& e) {
        throw NLBIOException("Error while deleting directory: " + std::string(e.what()));
    }
}

std::string FileManipulator::getRequiredFileAsString(
    const std::string& rootDir,
    const std::string& fileName,
    const std::string& errorMessage) 
{
    namespace fs = std::filesystem;
    fs::path filePath = fs::path(rootDir) / fileName;
    
    if (!fs::exists(filePath)) {
        throw NLBIOException(errorMessage);
    }

    std::ifstream file(filePath);
    if (!file) {
        throw NLBIOException("Cannot open file: " + filePath.string());
    }

    return getFileAsString(file);
}

std::string FileManipulator::getOptionalFileAsString(
    const std::string& rootDir,
    const std::string& fileName,
    const std::string& defaultValue) 
{
    namespace fs = std::filesystem;
    fs::path filePath = fs::path(rootDir) / fileName;
    
    if (!fs::exists(filePath)) {
        return defaultValue;
    }

    std::ifstream file(filePath);
    if (!file) {
        throw NLBIOException("Cannot open file: " + filePath.string());
    }

    return getFileAsString(file);
}

std::string FileManipulator::getFileAsString(std::ifstream& stream) {
    std::stringstream buffer;
    buffer << stream.rdbuf();
    return buffer.str();
}

void FileManipulator::writeRequiredString(
    const std::string& rootDir,
    const std::string& fileName,
    const std::string& content) 
{
    namespace fs = std::filesystem;
    fs::path filePath = fs::path(rootDir) / fileName;
    bool newFile = !fs::exists(filePath);

    if (!content.empty()) {
        std::stringstream ss(content);
        writeFile(filePath.string(), ss);
        addToVCS(filePath.string(), newFile);
    } else {
        createFile(filePath.string(), "Cannot create file with name " + fileName);
    }
}

void FileManipulator::writeFile(const std::string& filePath, std::istream& input) {
    std::ofstream output(filePath, std::ios::binary);
    if (!output) {
        throw NLBIOException("Cannot open file for writing: " + filePath);
    }

    transfer(input, output);
}

void FileManipulator::transfer(std::istream& input, std::ostream& output) {
    char buffer[BLOCK_SIZE];
    while (input.read(buffer, BLOCK_SIZE)) {
        output.write(buffer, input.gcount());
    }
    if (input.gcount() > 0) {
        output.write(buffer, input.gcount());
    }
}

std::string FileManipulator::getPathRelativeToMainRoot(const std::string& filePath) {
    namespace fs = std::filesystem;
    fs::path pathAbsolute = fs::absolute(filePath);
    fs::path pathBase = fs::absolute(m_mainRoot);
    return fs::relative(pathAbsolute, pathBase).string();
}

void FileManipulator::addToVCS(const std::string& filePath, bool isNewFile) {
    namespace fs = std::filesystem;
    
    if (fs::is_directory(filePath) && !m_vcsAdapter->getDirAddFlag()) {
        return;
    }

    std::string path = getPathRelativeToMainRoot(filePath);
    
    if (isNewFile) {
        VCSAdapter::Status status = m_vcsAdapter->getStatus(path);
        switch (status) {
            case VCSAdapter::Status::Unknown:
            case VCSAdapter::Status::VCS_Undefined:
                m_vcsAdapter->add(path);
                break;
            case VCSAdapter::Status::Added:
                break;
            default:
                throw NLBFileManipulationException(
                    "Incorrect file status while adding file with path = " +
                    path + " to VCS: " + std::to_string(static_cast<int>(status))
                );
        }
    } else if (m_vcsAdapter->getAddModifiedFilesFlag()) {
        m_vcsAdapter->add(path);
    }
}

void FileManipulator::createFile(const std::string& filePath, const std::string& errorMessage) {
    namespace fs = std::filesystem;
    try {
        fs::path path(filePath);
        if (!fs::exists(path)) {
            std::ofstream file(path);
            if (!file) {
                throw NLBIOException(errorMessage);
            }
            file.close();
            addToVCS(filePath, true);
        }
    } catch (const fs::filesystem_error& e) {
        throw NLBIOException(errorMessage + ": " + e.what());
    }
}