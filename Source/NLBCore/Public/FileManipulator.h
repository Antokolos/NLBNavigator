#pragma once

#include <string>
#include <fstream>
#include <memory>
#include <vector>
#include "VCSAdapter.h"
#include "MultiLangString.h"
#include "Constants.h"
#include "NonLinearBook.h"
#include "NLBExceptions.h"

/*!
 * The FileManipulator class contains useful file operations.
 */
class FileManipulator {
public:
    /*!
     * Constructor for FileManipulator
     * @param vcsAdapter VCS adapter instance
     * @param mainRoot Main root directory
     */
    FileManipulator(std::shared_ptr<VCSAdapter> vcsAdapter, const std::string& mainRoot);

    /*!
     * Deletes recursively file and directories.
     * @param filePath File or directory path for delete
     * @return true if all files and subdirectories successfully deleted.
     */
    bool deleteFileOrDir(const std::string& filePath);

    /*!
     * Writes content to file with required existence check
     * @param rootDir Root directory
     * @param fileName File name
     * @param content Content to write
     */
    void writeRequiredString(const std::string& rootDir, const std::string& fileName, const std::string& content);

    /*!
     * Writes content to file with optional existence check
     * @param rootDir Root directory
     * @param fileName File name
     * @param content Content to write
     * @param defaultContent Default content if file doesn't exist
     */
    void writeOptionalString(const std::string& rootDir, const std::string& fileName, 
                           const std::string& content, const std::string& defaultContent);

    /*!
     * Creates directory with error checking
     * @param dirPath Directory path
     * @param errorMessage Error message in case of failure
     */
    void createDir(const std::string& dirPath, const std::string& errorMessage);

    /*!
     * Copies file with error checking
     * @param target Target file path
     * @param source Source file path
     * @param errorMessage Error message in case of failure
     */
    void copyFile(const std::string& target, const std::string& source, const std::string& errorMessage);

    /*!
     * Writes multilanguage string content to files
     * @param mlsRootDir Root directory for language files
     * @param content Content to write
     * @param defaultContent Default content
     */
    void writeOptionalMultiLangString(const std::string& mlsRootDir,
                                    const MultiLangString& content,
                                    const MultiLangString& defaultContent);

    // Static methods
    /*!
     * Reads file content as string with required existence
     * @param rootDir Root directory
     * @param fileName File name
     * @param errorMessage Error message if file not found
     * @return File content as string
     */
    static std::string getRequiredFileAsString(const std::string& rootDir,
                                             const std::string& fileName,
                                             const std::string& errorMessage);

    /*!
     * Reads file content as string with optional existence
     * @param rootDir Root directory
     * @param fileName File name
     * @param defaultValue Default value if file not found
     * @return File content as string
     */
    static std::string getOptionalFileAsString(const std::string& rootDir,
                                             const std::string& fileName,
                                             const std::string& defaultValue);

    /*!
     * Reads multilanguage string from files
     * @param mlsRootDir Root directory for language files
     * @param defaultValue Default value if no files found
     * @return MultiLangString instance
     */
    static MultiLangString readOptionalMultiLangString(const std::string& mlsRootDir,
                                                      const MultiLangString& defaultValue);

private:
    static const int BLOCK_SIZE = 1024;
    std::shared_ptr<VCSAdapter> m_vcsAdapter;
    std::string m_mainRoot;

    std::string getPathRelativeToMainRoot(const std::string& filePath);
    void addToVCS(const std::string& filePath, bool isNewFile);
    void createFile(const std::string& filePath, const std::string& errorMessage);
    static std::string getFileAsString(std::ifstream& stream);
    static void transfer(std::istream& input, std::ostream& output);
    static void writeFile(const std::string& filePath, std::istream& input);
};
