#pragma once

#include "nlb/domain/NonLinearBookImpl.h"
#include "nlb/api/Link.h"
#include <iomanip>

/**
 * @brief Interactive NLB explorer
 */
class NLBExplorer {
public:
    static const std::string INVENTORY;
    static const std::string TRUE;
    static const std::string FALSE;

    explicit NLBExplorer(NonLinearBook* book, const std::string &currentPageId);

    /**
     * @brief Start interactive exploration
     */
    void explore();

private:
    NonLinearBook* m_book;
    Page* m_currentPage;

    void executeModifications(const ModifyingItem *item);

    /**
     * @brief Show current page and get user's choice for next page
     */
    bool showPageAndGetNextChoice(Page *page);
};
