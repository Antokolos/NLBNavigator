#pragma once

#include <string>
#include <vector>

/*!
 * @brief Class representing a search result with associated information
 *
 * This class maintains information about a search result including an ID,
 * a module page ID, and a list of information strings.
 */
class SearchResult {
private:
    std::string m_id;
    std::string m_modulePageId;
    std::vector<std::string> m_information;

public:
    /*!
     * @brief Default constructor
     */
    SearchResult();

    /*!
     * @brief Constructor with initial values
     * @param id The ID of the search result
     * @param modulePageId The module page ID
     * @param information Initial information string
     */
    SearchResult(
        const std::string& id,
        const std::string& modulePageId,
        const std::string& information
    );

    /*!
     * @brief Get the ID of the search result
     * @return The ID string
     */
    std::string getId() const;

    /*!
     * @brief Set the ID of the search result
     * @param id The new ID string
     */
    void setId(const std::string& id);

    /*!
     * @brief Get the module page ID
     * @return The module page ID string
     */
    std::string getModulePageId() const;

    /*!
     * @brief Set the module page ID
     * @param modulePageId The new module page ID string
     */
    void setModulePageId(const std::string& modulePageId);

    /*!
     * @brief Set the information, clearing any existing information
     * @param information The new information string
     */
    void setInformation(const std::string& information);

    /*!
     * @brief Add additional information
     * @param information The information string to add
     */
    void addInformation(const std::string& information);

    /*!
     * @brief Get information at a specific position
     * @param i The position index
     * @return The information string at the specified position
     */
    std::string getInformationByPosition(size_t i) const;
};