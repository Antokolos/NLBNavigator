#pragma once

#include <string>
#include <vector>

/*!
 * \class FileUtils
 * \brief Utility class for file system operations
 */
class FileUtils {
public:
    /*!
     * \brief Get file name from path
     * \param path Full file path
     * \return File name with extension
     */
    static std::string getFileName(const std::string& path);

    /*!
     * \brief Get directory path from full path
     * \param path Full file path
     * \return Directory path without trailing separator
     */
    static std::string getFilePath(const std::string& path);

    /*!
     * \brief Combine two path parts
     * \param path1 First path part
     * \param path2 Second path part
     * \return Combined path with proper separator
     */
    static std::string combinePath(const std::string& path1, const std::string& path2);

    /*!
     * \brief Check if file or directory exists
     * \param path File path to check
     * \return true if file exists
     */
    static bool exists(const std::string& path);

    /*!
     * \brief Check if path is a directory
     * \param path Path to check
     * \return true if path is a directory
     */
    static bool isDirectory(const std::string& path);

    /*!
     * \brief Get list of files in directory
     * \param path Directory path
     * \return Vector of file names
     */
    static std::vector<std::string> getDirectoryFiles(const std::string& path);

    /*!
     * \brief Create directory
     * \param path Directory path to create
     * \return true if successful
     */
    static bool createDirectory(const std::string& path);

    /*!
     * \brief Delete file or directory
     * \param path Path to delete
     * \param recursive If true, recursively delete directory contents
     * \return true if successful
     */
    static bool remove(const std::string& path, bool recursive = false);

    /*!
     * \brief Get platform-specific path separator
     * \return Path separator character
     */
    static char getPathSeparator();
    
private:
    // Split path into components
    static std::vector<std::string> splitPath(const std::string& path);
    
    // Normalize path separators for current platform
    static std::string normalizePath(const std::string& path);
};