#pragma once

#include "nlb/domain/NonLinearBookImpl.h"
#include "nlb/api/Variable.h"
#include <iomanip>

/**
 * @brief Simple NLB Reader application demonstrating read-only functionality
 */
class NLBReader {
public:
    /**
     * @brief Load and display book information
     * @param nlbPath Path to the NLB directory
     */
    void readBook(const std::string& nlbPath);

private:
    /**
     * @brief Display basic book information
     */
    void displayBookInfo(NonLinearBook* book);

    /**
     * @brief Display content overview (pages, objects, variables)
     */
    void displayContentOverview(NonLinearBook* book);

    /**
     * @brief Display navigation structure starting from start point
     */
    void displayNavigationStructure(NonLinearBook* book);

    /**
     * @brief Recursively display page navigation
     */
    static void displayPageNavigation(NonLinearBook* book,
                                      Page* page,
                                      int depth,
                                      std::set<std::string>& visited);

    /**
     * @brief Display media files information
     */
    static void displayMediaFiles(NonLinearBook* book);

    /**
     * @brief Validate book structure and display results
     */
    static void validateBook(NonLinearBook* book);

    /**
     * @brief Convert theme enum to string
     */
    static std::string themeToString(Theme theme);

    /**
     * @brief Convert data type enum to string
     */
    static std::string dataTypeToString(Variable::DataType dataType);
};
