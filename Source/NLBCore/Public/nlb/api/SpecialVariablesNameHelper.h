#pragma once

#include <string>

/*!
 * @brief Helper class for handling special variable names
 */
class SpecialVariablesNameHelper {
public:
    /*!
     * @brief Decorate an ID by replacing hyphens with underscores
     * @param id Original ID string
     * @return Decorated ID string
     */
    static std::string decorateId(const std::string& id);

    /*!
     * @brief Create a decorated link visit state variable name
     * @param linkId Original link ID
     * @return Decorated link visit state variable name
     */
    static std::string decorateLinkVisitStateVar(const std::string& linkId);

    /*!
     * @brief Create a decorated ID with optional autowired ID prefix
     * @param id Original ID string
     * @param autowiredId Optional autowired ID string
     * @return Decorated combined ID string
     */
    static std::string decorateId(const std::string& id, const std::string& autowiredId = "");

private:
    // Utility class - prevent instantiation
    SpecialVariablesNameHelper() = delete;
    ~SpecialVariablesNameHelper() = delete;

    // Helper method for replacing hyphens
    static std::string replaceHyphens(const std::string& str);
};