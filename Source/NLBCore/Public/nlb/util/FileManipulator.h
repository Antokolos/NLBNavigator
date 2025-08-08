#pragma once
#include <string>
#include <memory>
#include <fstream>
#include <sstream>
#include "nlb/api/Constants.h"
#include "nlb/exception/NLBExceptions.h"
#include "nlb/util/FileUtils.h"

// Forward declarations
class VCSAdapter;
class MultiLangString;
class NonLinearBook;

/*!
 * \class FileManipulator
 * \brief Handles file operations with version control system integration
 *
 * This class provides functionality for file manipulation operations including
 * reading, writing, copying, and deleting files while maintaining VCS integration.
 */
class FileManipulator {
public:
    /*!
     * \brief Constructs FileManipulator with VCS adapter and root directory
     * \param vcsAdapter Version control system adapter
     * \param mainRoot Main root directory path
     */
    FileManipulator(VCSAdapter* vcsAdapter, const std::string& mainRoot);

    /*!
     * \brief Deletes file or directory with VCS handling
     * \param filePath Path to file or directory to delete
     * \return true if deletion was successful
     */
    bool deleteFileOrDir(const std::string& filePath) const;

    /*!
     * \brief Writes required string content to file
     * \param rootDir Root directory path
     * \param fileName Name of file to write
     * \param content Content to write
     * \throws NLBIOException if write operation fails
     */
    void writeRequiredString(const std::string& rootDir,
                           const std::string& fileName,
                           const std::string& content) const;

    /*!
     * \brief Writes optional string content to file
     * \param rootDir Root directory path
     * \param fileName Name of file to write
     * \param content Content to write
     * \param defaultContent Default content to compare against
     * \throws NLBIOException if write operation fails
     */
    void writeOptionalString(const std::string& rootDir,
                           const std::string& fileName,
                           const std::string& content,
                           const std::string& defaultContent) const;

    /*!
     * \brief Writes multilanguage string content
     * \param mlsRootDir Root directory for multilanguage content
     * \param content Content to write
     * \param defaultContent Default content to compare against
     * \throws NLBIOException if write operation fails
     */
    void writeOptionalMultiLangString(const std::string& mlsRootDir,
                                    const MultiLangString& content,
                                    const MultiLangString& defaultContent) const;

    /*!
     * \brief Creates directory with error handling
     * \param dirPath Directory path to create
     * \param errorMessage Error message if creation fails
     * \throws NLBIOException if directory creation fails
     */
    void createDir(const std::string& dirPath,
                  const std::string& errorMessage) const;

    /*!
     * \brief Copies file with error handling
     * \param target Target file path
     * \param source Source file path
     * \param errorMessage Error message if copy fails
     * \throws NLBIOException if file copy fails
     */
    void copyFile(const std::string& target,
                 const std::string& source,
                 const std::string& errorMessage) const;

    /*!
     * \brief Creates empty file
     * \param filePath Path of file to create
     * \param errorMessage Error message if creation fails
     * \throws NLBIOException if file creation fails
     */
    void createFile(const std::string& filePath, const std::string& errorMessage) const;

    /*!
     * \brief Gets path relative to main root directory
     * \param filePath Absolute file path
     * \return Path relative to main root
     */
    std::string getPathRelativeToMainRoot(const std::string& filePath) const;

    /*!
     * \brief Reads required file as string
     * \param rootDir Root directory path
     * \param fileName Name of file to read
     * \param errorMessage Error message if read fails
     * \return File content as string
     * \throws NLBIOException if file cannot be read
     */
    static std::string getRequiredFileAsString(const std::string& rootDir,
                                             const std::string& fileName,
                                             const std::string& errorMessage);

    /*!
     * \brief Reads optional file as string
     * \param rootDir Root directory path
     * \param fileName Name of file to read
     * \param defaultValue Default value if file doesn't exist
     * \return File content as string or default value
     */
    static std::string getOptionalFileAsString(const std::string& rootDir,
                                             const std::string& fileName,
                                             const std::string& defaultValue);

    /*!
     * \brief Reads optional multilanguage string
     * \param mlsRootDir Root directory for multilanguage content
     * \param defaultValue Default value if content doesn't exist
     * \return MultiLangString object with content
     */
    static MultiLangString readOptionalMultiLangString(const std::string& mlsRootDir,
                                                      const MultiLangString& defaultValue);

    /*!
     * \brief Writes input stream to file
     * \param filePath Path of file to write
     * \param input Input stream containing data
     * \throws NLBIOException if write operation fails
     */
    static void writeFile(const std::string& filePath, std::istream& input);

    /*!
     * \brief Reads entire file stream to string
     * \param stream Input file stream
     * \return File content as string
     * \throws NLBIOException if read operation fails
     */
    static std::string getFileAsString(std::ifstream& stream);

    /*!
     * \brief Transfers data between streams
     * \param input Input stream
     * \param output Output stream
     * \throws NLBIOException if transfer operation fails
     */
    static void transfer(std::istream& input, std::ostream& output);
    
private:
    static const int BLOCK_SIZE = 1024; ///< Buffer size for file operations

    VCSAdapter* m_vcsAdapter; ///< Version control system adapter
    std::string m_mainRoot; ///< Main root directory path

    /*!
     * \brief Adds file to version control
     * \param filePath Path of file to add
     * \param isNewFile true if file is newly created
     */
    void addToVCS(const std::string& filePath, bool isNewFile) const;
};
