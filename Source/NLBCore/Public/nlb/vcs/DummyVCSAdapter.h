#pragma once

#include "nlb/vcs/VCSAdapter.h"

/*!
 * @class DummyVCSAdapter
 * @brief Dummy implementation of VCSAdapter interface
 * 
 * This class provides a null object pattern implementation of VCSAdapter
 * with empty/default implementations for all interface methods.
 * Used when no actual version control system integration is needed.
 */
class DummyVCSAdapter : public VCSAdapter {
public:
    /*!
     * @brief Default constructor
     */
    DummyVCSAdapter() = default;

    /*!
     * @brief Virtual destructor
     */
    virtual ~DummyVCSAdapter() = default;

    /*!
     * @brief Initialize repository - does nothing in dummy implementation
     * @param path Path to repository (unused)
     */
    void initRepo(const std::string& path) override;

    /*!
     * @brief Open repository - does nothing in dummy implementation
     * @param path Path to repository (unused)
     */
    void openRepo(const std::string& path) override;

    /*!
     * @brief Close adapter - does nothing in dummy implementation
     */
    void closeAdapter() override;

    /*!
     * @brief Get directory add flag
     * @return Always returns false in dummy implementation
     */
    bool getDirAddFlag() override;

    /*!
     * @brief Get flag for adding modified files
     * @return Always returns false in dummy implementation
     */
    bool getAddModifiedFilesFlag() override;

    /*!
     * @brief Get file status
     * @param path File path (unused)
     * @return Always returns Status::Unknown in dummy implementation
     */
    Status getStatus(const std::string& path) override;

    /*!
     * @brief Add file to version control - does nothing in dummy implementation
     * @param path File path (unused)
     */
    void add(const std::string& path) override;

    /*!
     * @brief Remove file from version control
     * @param path File path (unused)
     * @return Always returns false in dummy implementation
     */
    bool remove(const std::string& path) override;

    /*!
     * @brief Reset file - does nothing in dummy implementation
     * @param path File path (unused)
     */
    void reset(const std::string& path) override;

    /*!
     * @brief Commit changes - does nothing in dummy implementation
     * @param message Commit message (unused)
     */
    void commit(const std::string& message) override;

    /*!
     * @brief Pull changes from remote - does nothing in dummy implementation
     * @param userName User name (unused)
     * @param password Password (unused)
     * @param progressData Progress data (unused)
     */
    void pull(
        const std::string& userName, 
        const std::string& password, 
        std::shared_ptr<ProgressData> progressData
    ) override;

    /*!
     * @brief Push changes to remote - does nothing in dummy implementation
     * @param userName User name (unused)
     * @param password Password (unused)
     * @param progressData Progress data (unused)
     */
    void push(
        const std::string& userName, 
        const std::string& password, 
        std::shared_ptr<ProgressData> progressData
    ) override;
};