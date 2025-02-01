#pragma once

#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <memory>
#include "nlb/util/FileUtils.h"
#include "nlb/exception/NLBExceptions.h"
#include "nlb/util/StringHelper.h"
#include "nlb/util/FileManipulator.h"

/*!
 * Class for managing and exporting bundled resource files
 */
class ResourceManager {
public:
    /*!
     * Exports all bundled files to the specified target directory
     * @param targetDir Directory where files should be exported
     * @throws NLBIOException if there's an error during export
     */
    static void exportBundledFiles(const std::string& targetDir) throw(NLBIOException);

private:
    static const std::string VNSTEAD;

    /*!
     * Gets map of resource folder files
     * @param loader Class loader to use for resource loading
     * @return Map of folder paths to lists of files
     * @throws NLBIOException if resources directory doesn't exist
     */
    static std::map<std::string, std::vector<std::string>> getResourceFolderFiles(void* loader) throw(NLBIOException);

    /*!
     * Recursively gets all child files from a directory
     * @param parent Parent directory to scan
     * @param parentPath Path relative to parent directory
     * @return Map of folder paths to lists of files
     */
    static std::map<std::string, std::vector<std::string>> getAllChildren(const std::string& parent, const std::string& parentPath);

    /*!
     * Exports files from a single resource folder
     * @param resourceFileEntry Entry containing path and files to export
     * @param targetDir Target directory for export
     * @throws NLBIOException if there's an error during export
     */
    static void exportBundledFiles(
        const std::pair<std::string, std::vector<std::string>>& resourceFileEntry,
        const std::string& targetDir) throw(NLBIOException);

    /*!
     * Exports a single bundled file
     * @param resourceFileParent Parent directory for the exported file
     * @param resourceFile File to export
     * @throws NLBIOException if there's an error during export
     */
    static void exportBundledFile(
        const std::string& resourceFileParent,
        const std::string& resourceFile) throw(NLBIOException);
};