#pragma once

#include <string>
#include <memory>
#include "nlb/exception/NLBExceptions.h"

class ProgressData;

/*!
 * @class VCSAdapter
 * @brief Interface for Version Control System operations
 */
class VCSAdapter {
public:
    enum class Status {
        Modified,
        Added,
        Removed,
        Missing,
        Unknown,
        Clean,
        Ignored,
        Conflict,
        VCS_Undefined
    };

    virtual ~VCSAdapter() = default;

    virtual void initRepo(const std::string& path) = 0;
    virtual void openRepo(const std::string& path) = 0;
    virtual void closeAdapter() = 0;
    virtual bool getDirAddFlag() = 0;

    /*!
     * @brief If add to repository should be used on changed files already existent in repo to ensure
     * that this files will be staged for commit
     * @return true if modified files should be added
     */
    virtual bool getAddModifiedFilesFlag() = 0;

    virtual Status getStatus(const std::string& path) = 0;
    virtual void add(const std::string& path) = 0;

    /*!
     * @brief Remove file from VCS
     * @return true if file has been physically removed from the working directory, false otherwise
     */
    virtual bool remove(const std::string& path) = 0;

    virtual void reset(const std::string& path) = 0;
    virtual void commit(const std::string& message) = 0;
    
    virtual void pull(
        const std::string& userName, 
        const std::string& password, 
        ProgressData* progressData
    ) = 0;
    
    virtual void push(
        const std::string& userName, 
        const std::string& password, 
        ProgressData* progressData
    ) = 0;
};