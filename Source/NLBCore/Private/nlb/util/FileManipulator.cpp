#include "nlb/util/FileManipulator.h"
#include "nlb/vcs/VCSAdapter.h"
#include "nlb/util/StringHelper.h"
#include "nlb/util/MultiLangString.h"
#include "nlb/api/NonLinearBook.h"
#include <sstream>
#include <stdexcept>

FileManipulator::FileManipulator(VCSAdapter* vcsAdapter, const std::string& mainRoot)
    : m_vcsAdapter(vcsAdapter)
    , m_mainRoot(mainRoot) {
}

bool FileManipulator::deleteFileOrDir(const std::string& filePath) const {
    try {
        if (!FileUtils::exists(filePath)) {
            return true;
        }

        bool ret = true;
        if (FileUtils::isDirectory(filePath)) {
            std::vector<std::string> children = FileUtils::getDirectoryFiles(filePath);
            for (const auto& child : children) {
                std::string childPath = FileUtils::combinePath(filePath, child);
                if (!deleteFileOrDir(childPath)) {
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
                removed = true; // reset doesn't physically remove but marks as removed in VCS
                break;
            case VCSAdapter::Status::Missing:
            case VCSAdapter::Status::Ignored:
                throw NLBFileManipulationException(
                    "Incorrect file status while deleting file with path = " + 
                    relativePath + " from VCS: " + std::to_string(static_cast<int>(status))
                );
            case VCSAdapter::Status::Unknown:
            case VCSAdapter::Status::VCS_Undefined:
                // For unknown files, just delete physically
                removed = true;
                break;
            default:
                throw NLBFileManipulationException(
                    "Unsupported file status while deleting file with path = " + 
                    relativePath + " from VCS: " + std::to_string(static_cast<int>(status))
                );
        }

        if (!removed) {
            removed = FileUtils::remove(filePath, true);
        }
        return removed && ret;

    } catch (const NLBFileManipulationException&) {
        throw; // Re-throw VCS specific exceptions
    } catch (const std::exception& e) {
        throw NLBIOException("Error while deleting: " + std::string(e.what()));
    }
}

void FileManipulator::writeRequiredString(
    const std::string& rootDir,
    const std::string& fileName,
    const std::string& content) const {
    try {
        std::string filePath = FileUtils::combinePath(rootDir, fileName);
        bool newFile = !FileUtils::exists(filePath);

        if (!content.empty()) {
            std::stringstream ss;
            ss << content;
            writeFile(filePath, ss);
            addToVCS(filePath, newFile);
        } else {
            createFile(filePath, "Cannot create file with name " + fileName);
        }
    } catch (const std::exception& e) {
        throw NLBIOException("IOException occurred while writing required string: " + std::string(e.what()));
    }
}

void FileManipulator::writeOptionalString(
    const std::string& rootDir,
    const std::string& fileName,
    const std::string& content,
    const std::string& defaultContent) const {
    try {
        std::string filePath = FileUtils::combinePath(rootDir, fileName);
        bool newFile = !FileUtils::exists(filePath);

        if (content == defaultContent) {
            if (!newFile) {
                deleteFileOrDir(filePath);
            }
        } else {
            if (!content.empty()) {
                std::stringstream ss;
                ss << content;
                writeFile(filePath, ss);
                addToVCS(filePath, newFile);
            } else {
                if (!defaultContent.empty()) {
                    createFile(filePath, "Cannot create file with name " + fileName);
                } else {
                    // Both content and default are empty, delete file if exists
                    if (!newFile) {
                        deleteFileOrDir(filePath);
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        throw NLBIOException("IOException occurred while writing optional string: " + std::string(e.what()));
    }
}

void FileManipulator::writeOptionalMultiLangString(
    const std::string& mlsRootDir,
    const MultiLangString& content,
    const MultiLangString& defaultContent) const {
    try {
        bool rootDirExists = FileUtils::exists(mlsRootDir);
        
        if (rootDirExists && !FileUtils::isDirectory(mlsRootDir)) {
            deleteFileOrDir(mlsRootDir);
            rootDirExists = false;
        }

        bool isSubsetOfDefault = content.isSubsetOf(defaultContent);
        if (isSubsetOfDefault) {
            if (rootDirExists) {
                deleteFileOrDir(mlsRootDir);
            }
        } else {
            if (!rootDirExists) {
                createDir(mlsRootDir, "Cannot create MultiLangString root: " + mlsRootDir);
            }

            auto languages = content.keySet();
            for (const auto& langKey : languages) {
                std::string langValue = content.get(langKey);
                std::string filePath = FileUtils::combinePath(mlsRootDir, langKey);
                bool newFile = !FileUtils::exists(filePath);

                if (content.equalsAs(langKey, defaultContent)) {
                    if (!newFile) {
                        deleteFileOrDir(filePath);
                    }
                } else {
                    std::stringstream ss;
                    ss << langValue;
                    writeFile(filePath, ss);
                    addToVCS(filePath, newFile);
                }
            }
        }
    } catch (const std::exception& e) {
        throw NLBIOException("IOException occurred while writing MultiLangString: " + std::string(e.what()));
    }
}

void FileManipulator::createDir(
    const std::string& dirPath,
    const std::string& errorMessage) const {
    try {
        if (!FileUtils::exists(dirPath)) {
            if (!FileUtils::createDirectory(dirPath)) {
                throw NLBIOException(errorMessage);
            }
            addToVCS(dirPath, true);
        }
    } catch (const NLBIOException&) {
        throw; // Re-throw NLB specific exceptions
    } catch (const std::exception& e) {
        throw NLBIOException(errorMessage + ": " + e.what());
    }
}

void FileManipulator::copyFile(
    const std::string& target,
    const std::string& source,
    const std::string& errorMessage) const {
    try {
        bool newFile = !FileUtils::exists(target);
        
        if (!FileUtils::exists(source)) {
            throw NLBIOException("Source file does not exist: " + source);
        }

        std::ifstream sourceFile(source, std::ios::binary);
        if (!sourceFile.is_open()) {
            throw NLBIOException("Cannot open source file: " + source);
        }

        writeFile(target, sourceFile);
        sourceFile.close();
        addToVCS(target, newFile);
    } catch (const std::exception& e) {
        throw NLBIOException(errorMessage + ": " + e.what());
    }
}

void FileManipulator::createFile(
    const std::string& filePath,
    const std::string& errorMessage) const {
    try {
        if (!FileUtils::exists(filePath)) {
            std::ofstream file(filePath);
            if (!file.is_open()) {
                throw NLBIOException(errorMessage);
            }
            file.close();
            addToVCS(filePath, true);
        }
    } catch (const NLBIOException&) {
        throw; // Re-throw NLB specific exceptions
    } catch (const std::exception& e) {
        throw NLBIOException(errorMessage + ": " + e.what());
    }
}

std::string FileManipulator::getPathRelativeToMainRoot(const std::string& filePath) const {
    std::string absolutePath = FileUtils::normalizePath(filePath);
    std::string rootPath = FileUtils::normalizePath(m_mainRoot);
    
    // Ensure root path ends with separator for proper matching
    if (!rootPath.empty() && rootPath.back() != FileUtils::getPathSeparator()) {
        rootPath += FileUtils::getPathSeparator();
    }
    
    if (absolutePath.find(rootPath) == 0) {
        return absolutePath.substr(rootPath.length());
    }
    
    // If path is not under root, return as-is (this might indicate an error)
    return absolutePath;
}

std::string FileManipulator::getRequiredFileAsString(
    const std::string& rootDir,
    const std::string& fileName,
    const std::string& errorMessage) {
    std::string filePath = FileUtils::combinePath(rootDir, fileName);
    if (!FileUtils::exists(filePath)) {
        throw NLBIOException(errorMessage);
    }

    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw NLBIOException("Cannot open file: " + filePath);
    }

    std::string content = getFileAsString(file);
    file.close();
    return content;
}

std::string FileManipulator::getOptionalFileAsString(
    const std::string& rootDir,
    const std::string& fileName,
    const std::string& defaultValue) {
    std::string filePath = FileUtils::combinePath(rootDir, fileName);
    if (!FileUtils::exists(filePath)) {
        return defaultValue;
    }

    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            return defaultValue;
        }

        std::string content = getFileAsString(file);
        file.close();
        return content;
    } catch (const std::exception&) {
        return defaultValue;
    }
}

MultiLangString FileManipulator::readOptionalMultiLangString(
    const std::string& mlsRootDir,
    const MultiLangString& defaultValue) {
    if (!FileUtils::exists(mlsRootDir)) {
        return MultiLangString::createCopy(defaultValue);
    }

    MultiLangString result = MultiLangString::createCopy(defaultValue);

    try {
        if (FileUtils::isDirectory(mlsRootDir)) {
            std::vector<std::string> langKeys = FileUtils::getDirectoryFiles(mlsRootDir);
            
            for (const auto& langKey : langKeys) {
                std::string filePath = FileUtils::combinePath(mlsRootDir, langKey);
                if (!FileUtils::isDirectory(filePath)) {
                    std::ifstream file(filePath);
                    if (file.is_open()) {
                        std::string content = getFileAsString(file);
                        result.put(langKey, content);
                        file.close();
                    }
                }
            }
        } else {
            // Single file case - treat as default language
            std::ifstream file(mlsRootDir);
            if (file.is_open()) {
                std::string content = getFileAsString(file);
                result.put(NonLinearBook::DEFAULT_LANGUAGE, content);
                file.close();
            }
        }
    } catch (const std::exception& e) {
        throw NLBIOException("IOException occurred while reading MultiLangString: " + std::string(e.what()));
    }

    return result;
}

void FileManipulator::writeFile(const std::string& filePath, std::istream& input) {
    try {
        std::ofstream output(filePath, std::ios::binary);
        if (!output.is_open()) {
            throw NLBIOException("Cannot open file for writing: " + filePath);
        }
        transfer(input, output);
        output.close();
    } catch (const std::exception& e) {
        throw NLBIOException("Error writing to file " + filePath + ": " + e.what());
    }
}

std::string FileManipulator::getFileAsString(std::ifstream& stream) {
    try {
        std::stringstream buffer;
        buffer << stream.rdbuf();
        return buffer.str();
    } catch (const std::exception& e) {
        throw NLBIOException("Error reading from stream: " + std::string(e.what()));
    }
}

void FileManipulator::transfer(std::istream& input, std::ostream& output) {
    try {
        char buffer[BLOCK_SIZE];
        while (input.good() && output.good()) {
            input.read(buffer, BLOCK_SIZE);
            std::streamsize bytesRead = input.gcount();
            if (bytesRead > 0) {
                output.write(buffer, bytesRead);
            }
        }
        
        if (input.bad()) {
            throw std::runtime_error("Error reading from input stream");
        }
        if (output.bad()) {
            throw std::runtime_error("Error writing to output stream");
        }
    } catch (const std::exception& e) {
        throw NLBIOException("Error during stream transfer: " + std::string(e.what()));
    }
}

void FileManipulator::addToVCS(const std::string& filePath, bool isNewFile) const {
    if (FileUtils::isDirectory(filePath) && !m_vcsAdapter->getDirAddFlag()) {
        return;
    }

    std::string path = getPathRelativeToMainRoot(filePath);
    
    try {
        if (isNewFile) {
            VCSAdapter::Status status = m_vcsAdapter->getStatus(path);
            switch (status) {
                case VCSAdapter::Status::Unknown:
                case VCSAdapter::Status::VCS_Undefined:
                    m_vcsAdapter->add(path);
                    break;
                case VCSAdapter::Status::Added:
                    // File is already added, nothing to do
                    break;
                case VCSAdapter::Status::Modified:
                case VCSAdapter::Status::Clean:
                    // File exists but is new in our context, add it
                    m_vcsAdapter->add(path);
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
    } catch (const std::exception& e) {
        throw NLBVCSException("VCS operation failed for path " + path + ": " + e.what());
    }
}
