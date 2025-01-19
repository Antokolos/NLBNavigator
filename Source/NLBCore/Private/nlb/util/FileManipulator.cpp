#include "nlb/util/FileManipulator.h"
#include "nlb/vcs/VCSAdapter.h"
#include "nlb/util/StringHelper.h"
#include "nlb/util/MultiLangString.h"
#include "nlb/api/NonLinearBook.h"
#include <sstream>

FileManipulator::FileManipulator(std::shared_ptr<VCSAdapter> vcsAdapter, const std::string& mainRoot)
    : m_vcsAdapter(vcsAdapter)
    , m_mainRoot(mainRoot) {
}

bool FileManipulator::deleteFileOrDir(const std::string& filePath) {
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
                break;
            case VCSAdapter::Status::Missing:
            case VCSAdapter::Status::Ignored:
                throw NLBFileManipulationException(
                    "Incorrect file status while deleting file with path = " + 
                    relativePath + " from VCS: " + std::to_string(static_cast<int>(status))
                );
        }

        if (!removed) {
            removed = FileUtils::remove(filePath, true);
        }
        return removed && ret;

    } catch (const std::exception& e) {
        throw NLBIOException("Error while deleting: " + std::string(e.what()));
    }
}

void FileManipulator::writeRequiredString(
    const std::string& rootDir,
    const std::string& fileName,
    const std::string& content) {
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
        throw NLBIOException("IOException occurred: " + std::string(e.what()));
    }
}

void FileManipulator::writeOptionalString(
    const std::string& rootDir,
    const std::string& fileName,
    const std::string& content,
    const std::string& defaultContent) {
    try {
        std::string filePath = FileUtils::combinePath(rootDir, fileName);
        bool newFile = !FileUtils::exists(filePath);

        if (!content.empty()) {
            if (content == defaultContent) {
                if (!newFile) {
                    deleteFileOrDir(filePath);
                }
            } else {
                std::stringstream ss;
                ss << content;
                writeFile(filePath, ss);
                addToVCS(filePath, newFile);
            }
        } else {
            if (!defaultContent.empty()) {
                createFile(filePath, "Cannot create file with name " + fileName);
            }
        }
    } catch (const std::exception& e) {
        throw NLBIOException("IOException occurred: " + std::string(e.what()));
    }
}

void FileManipulator::writeOptionalMultiLangString(
    const std::string& mlsRootDir,
    const MultiLangString& content,
    const MultiLangString& defaultContent) {
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

            for (const auto& pair : content.content()) {
                const std::string& langKey = pair.first;
                const std::string& langValue = pair.second;
                
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
        throw NLBIOException("IOException occurred: " + std::string(e.what()));
    }
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

    return getFileAsString(file);
}

std::string FileManipulator::getOptionalFileAsString(
    const std::string& rootDir,
    const std::string& fileName,
    const std::string& defaultValue) {
    std::string filePath = FileUtils::combinePath(rootDir, fileName);
    if (!FileUtils::exists(filePath)) {
        return defaultValue;
    }

    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw NLBIOException("Cannot open file: " + filePath);
    }

    return getFileAsString(file);
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
                std::ifstream file(filePath);
                if (!file.is_open()) {
                    throw NLBIOException("Cannot open file: " + filePath);
                }
                result.put(langKey, getFileAsString(file));
            }
        } else {
            std::ifstream file(mlsRootDir);
            if (!file.is_open()) {
                throw NLBIOException("Cannot open file: " + mlsRootDir);
            }
            result.put(NonLinearBook::DEFAULT_LANGUAGE, getFileAsString(file));
        }
    } catch (const std::exception& e) {
        throw NLBIOException("IOException occurred: " + std::string(e.what()));
    }

    return result;
}

void FileManipulator::createDir(
    const std::string& dirPath,
    const std::string& errorMessage) {
    try {
        if (!FileUtils::exists(dirPath)) {
            if (!FileUtils::createDirectory(dirPath)) {
                throw NLBIOException(errorMessage);
            }
            addToVCS(dirPath, true);
        }
    } catch (const std::exception& e) {
        throw NLBIOException(errorMessage + ": " + e.what());
    }
}

void FileManipulator::copyFile(
    const std::string& target,
    const std::string& source,
    const std::string& errorMessage) {
    try {
        bool newFile = !FileUtils::exists(target);
        
        if (newFile) {
            std::ofstream file(target, std::ios::binary);
            if (!file.is_open()) {
                throw NLBIOException("Cannot create file: " + target);
            }
            file.close();
        }

        std::ifstream sourceFile(source, std::ios::binary);
        if (!sourceFile.is_open()) {
            throw NLBIOException("Cannot open source file: " + source);
        }

        writeFile(target, sourceFile);
        addToVCS(target, newFile);
    } catch (const std::exception& e) {
        throw NLBIOException(errorMessage + ": " + e.what());
    }
}

void FileManipulator::addToVCS(const std::string& filePath, bool isNewFile) {
    if (FileUtils::isDirectory(filePath) && !m_vcsAdapter->getDirAddFlag()) {
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

void FileManipulator::createFile(
    const std::string& filePath,
    const std::string& errorMessage) {
    try {
        if (!FileUtils::exists(filePath)) {
            std::ofstream file(filePath);
            if (!file.is_open()) {
                throw NLBIOException(errorMessage);
            }
            file.close();
            addToVCS(filePath, true);
        }
    } catch (const std::exception& e) {
        throw NLBIOException(errorMessage + ": " + e.what());
    }
}

std::string FileManipulator::getFileAsString(std::ifstream& stream) {
    std::stringstream buffer;
    buffer << stream.rdbuf();
    return buffer.str();
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

void FileManipulator::writeFile(const std::string& filePath, std::istream& input) {
    std::ofstream output(filePath, std::ios::binary);
    if (!output.is_open()) {
        throw NLBIOException("Cannot open file for writing: " + filePath);
    }
    transfer(input, output);
}

std::string FileManipulator::getPathRelativeToMainRoot(const std::string& filePath) {
    std::string absolutePath = FileUtils::normalizePath(filePath);
    std::string rootPath = FileUtils::normalizePath(m_mainRoot);
    
    if (absolutePath.find(rootPath) == 0) {
        return absolutePath.substr(rootPath.length() + 1);
    }
    
    return absolutePath;
}