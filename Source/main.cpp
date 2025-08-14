#include "nlb/domain/NonLinearBookImpl.h"
#include "nlb/api/Page.h"
#include "nlb/api/Obj.h"
#include "nlb/api/Link.h"
#include "nlb/api/Variable.h"
#include "nlb/api/MediaFile.h"
#include "nlb/exception/NLBExceptions.h"
#include <iostream>
#include <memory>
#include <iomanip>

#include "nlb/api/ConsoleProgressData.h"

/**
 * @brief Simple NLB Reader application demonstrating read-only functionality
 */
class NLBReader {
public:
    /**
     * @brief Load and display book information
     * @param nlbPath Path to the NLB directory
     */
    void readBook(const std::string& nlbPath) {
        try {
            // Load the book
            auto book = new NonLinearBookImpl();
            ConsoleProgressData progressData;
            book->load(nlbPath, progressData);
            
            // Display book information
            displayBookInfo(book);
            
            // Display content overview
            displayContentOverview(book);
            
            // Display navigation structure
            displayNavigationStructure(book);
            
            // Display media files
            displayMediaFiles(book);
            
            // Validate book structure
            validateBook(book);
            
        } catch (const NLBIOException& e) {
            std::cerr << "Error reading NLB: " << e.what() << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Unexpected error: " << e.what() << std::endl;
        }
    }

private:
    /**
     * @brief Display basic book information
     */
    void displayBookInfo(NonLinearBook* book) {
        std::cout << "=== BOOK INFORMATION ===" << std::endl;
        // std::cout << book->getInfo() << std::endl;
        
        std::cout << "Theme: " << themeToString(book->getTheme()) << std::endl;
        std::cout << "Full Autowire: " << (book->isFullAutowire() ? "Yes" : "No") << std::endl;
        std::cout << "Suppress Media: " << (book->isSuppressMedia() ? "Yes" : "No") << std::endl;
        std::cout << "Suppress Sound: " << (book->isSuppressSound() ? "Yes" : "No") << std::endl;
        
        if (!book->getPerfectGameAchievementName().empty()) {
            std::cout << "Perfect Game Achievement: " << book->getPerfectGameAchievementName() << std::endl;
        }
        std::cout << std::endl;
    }
    
    /**
     * @brief Display content overview (pages, objects, variables)
     */
    void displayContentOverview(NonLinearBook* book) {
        std::cout << "=== CONTENT OVERVIEW ===" << std::endl;
        
        // Pages
        auto pages = book->getPages();
        std::cout << "Pages (" << pages.size() << "):" << std::endl;
        for (const auto& [pageId, page] : pages) {
            std::cout << "  - " << pageId << ": " << page->getCaption() << std::endl;
            std::cout << "    Links: " << page->getLinks().size() << std::endl;
            if (page->isLeaf()) {
                std::cout << "    [LEAF]";
            }
            if (page->isFinish()) {
                std::cout << "    [FINISH]";
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
        
        // Objects
        auto objects = book->getObjs();
        if (!objects.empty()) {
            std::cout << "Objects (" << objects.size() << "):" << std::endl;
            for (const auto& [objId, obj] : objects) {
                std::cout << "  - " << objId << ": " << obj->getName() << std::endl;
            }
            std::cout << std::endl;
        }
        
        // Variables
        auto variables = book->getVariables();
        if (!variables.empty()) {
            std::cout << "Variables (" << variables.size() << "):" << std::endl;
            for (const auto& variable : variables) {
                std::cout << "  - " << variable->getName()
                         << " (Type: " << dataTypeToString(variable->getDataType()) << ")" << std::endl;
            }
            std::cout << std::endl;
        }
    }
    
    /**
     * @brief Display navigation structure starting from start point
     */
    void displayNavigationStructure(NonLinearBook* book) {
        std::cout << "=== NAVIGATION STRUCTURE ===" << std::endl;
        
        std::string startPoint = book->getStartPoint();
        if (startPoint.empty()) {
            std::cout << "No start point defined." << std::endl;
            return;
        }
        
        auto startPage = book->getPageById(startPoint);
        if (!startPage) {
            std::cout << "Start point page not found: " << startPoint << std::endl;
            return;
        }
        
        std::cout << "Start Point: " << startPoint << " (" << startPage->getCaption() << ")" << std::endl;
        
        // Show navigation tree from start point
        std::set<std::string> visited;
        displayPageNavigation(book, startPage, 0, visited);
        std::cout << std::endl;
    }
    
    /**
     * @brief Recursively display page navigation
     */
    void displayPageNavigation(NonLinearBook* book,
                              Page* page,
                              int depth, 
                              std::set<std::string>& visited) {
        if (depth > 5) return; // Prevent deep recursion
        if (visited.count(page->getId())) {
            std::cout << std::string(depth * 2, ' ') << "→ " << page->getId() 
                     << " [ALREADY VISITED]" << std::endl;
            return;
        }
        
        visited.insert(page->getId());
        
        std::cout << std::string(depth * 2, ' ') << "→ " << page->getId() 
                 << " (" << page->getCaption() << ")";
        if (page->isFinish()) {
            std::cout << " [FINISH]";
        }
        std::cout << std::endl;
        
        // Show links from this page
        auto links = page->getLinks();
        for (const auto& link : links) {
            if (!link->isDeleted()) {
                std::cout << std::string((depth + 1) * 2, ' ') << "↳ \"" 
                         << link->getText() << "\" → " << link->getTarget() << std::endl;
                
                // Recursively show target page
                auto targetPage = book->getPageById(link->getTarget());
                if (targetPage) {
                    displayPageNavigation(book, targetPage, depth + 2, visited);
                }
            }
        }
    }
    
    /**
     * @brief Display media files information
     */
    void displayMediaFiles(NonLinearBook* book) {
        std::cout << "=== MEDIA FILES ===" << std::endl;
        
        auto imageFiles = book->getImageFiles();
        if (!imageFiles.empty()) {
            std::cout << "Images (" << imageFiles.size() << "):" << std::endl;
            for (const auto& image : imageFiles) {
                std::cout << "  - " << image->getFileName() << std::endl;
            }
            std::cout << std::endl;
        }
        
        auto soundFiles = book->getSoundFiles();
        if (!soundFiles.empty()) {
            std::cout << "Sounds (" << soundFiles.size() << "):" << std::endl;
            for (const auto& sound : soundFiles) {
                std::cout << "  - " << sound->getFileName() << std::endl;
            }
            std::cout << std::endl;
        }
    }
    
    /**
     * @brief Validate book structure and display results
     */
    void validateBook(NonLinearBook* book) {
        std::cout << "=== VALIDATION ===" << std::endl;
        
        // auto errors = book->validateStructure();
        // if (errors.empty()) {
        //     std::cout << "✓ Book structure is valid." << std::endl;
        // } else {
        //     std::cout << "✗ Found " << errors.size() << " validation error(s):" << std::endl;
        //     for (const auto& error : errors) {
        //         std::cout << "  - " << error << std::endl;
        //     }
        // }
        std::cout << std::endl;
    }
    
    /**
     * @brief Convert theme enum to string
     */
    std::string themeToString(Theme theme) {
        switch (theme) {
            case Theme::STANDARD: return "Standard";
            case Theme::VN: return "VN";
            default: return "Unknown";
        }
    }
    
    /**
     * @brief Convert data type enum to string
     */
    std::string dataTypeToString(Variable::DataType dataType) {
        switch (dataType) {
            case Variable::DataType::STRING: return "String";
            case Variable::DataType::NUMBER: return "Number";
            case Variable::DataType::BOOLEAN: return "Boolean";
            default: return "Unknown";
        }
    }
};

/**
 * @brief Interactive NLB explorer
 */
class NLBExplorer {
public:
    explicit NLBExplorer(NonLinearBook* book, const std::string &currentPageId)
        : m_book(book)
    {
        m_currentPage = m_book->getPageById(currentPageId);
    }

    /**
     * @brief Start interactive exploration
     */
    void explore() {
        try
        {
            // Interactive navigation loop
            while (showPageAndGetNextChoice(m_currentPage))
            {}
            NonLinearBook *parentBook = m_book->getParentNLB();
            if (parentBook && m_currentPage->isFinish()) {
                std::string returnPageId = m_currentPage->getReturnPageId();
                m_currentPage = returnPageId.empty() ? m_book->getParentPage() : parentBook->getPageById(returnPageId);
            }
        } catch (const NLBIOException& e) {
            std::cerr << "Error reading NLB: " << e.what() << std::endl;
        }
    }

private:
    NonLinearBook* m_book;
    Page* m_currentPage;
    
    /**
     * @brief Show current page and get user's choice for next page
     */
    bool showPageAndGetNextChoice(Page *page) {
        if (!page) return false;
        std::cout << "\n" << std::string(50, '=') << std::endl;
        std::cout << "PAGE: " << page->getCaption() << " (" << page->getFullId() << ")" << std::endl;
        std::cout << std::string(50, '=') << std::endl;
        
        // Show page text
        std::cout << page->getText() << std::endl;

        // Show available links
        auto links = page->getLinks();
        auto module = page->getModule();
        
        std::cout << "\nChoose your next action:" << std::endl;
        for (size_t i = 0; i < links.size(); ++i) {
            std::cout << (i + 1) << ". " << links[i]->getText() << std::endl;
        }
        size_t traverseChoice = 0;
        size_t returnChoice = 0;
        if (module && !module->isEmpty()) {
            traverseChoice = (links.size() + 1);
            std::cout << traverseChoice << ". " << page->getTraverseText() << std::endl;
        } else if (page->isFinish() && !page->isAutoReturn()) {
            returnChoice = traverseChoice > 0 ? traverseChoice + 1 : (links.size() + 1);
            std::cout << returnChoice << ". " << page->getReturnText() << std::endl;
        }
        std::cout << "0. Exit" << std::endl;
        
        // Get user choice
        int choice;
        std::cout << "\nYour choice: ";
        std::cin >> choice;
        
        if (choice == 0) {
            return false;
        }

        if (module && !module->isEmpty() && choice == traverseChoice) {
            NLBExplorer explorer(module, module->getStartPoint());
            explorer.explore();
            m_currentPage = explorer.m_currentPage;
            return true;
        }

        if (page->isFinish() && !page->isAutoReturn() && choice == returnChoice) {
            return false;
        }

        if (choice < 1 || choice > static_cast<int>(links.size())) {
            std::cout << "Invalid choice. Try again." << std::endl;
            return true; // Stay on current page
        }
        
        std::string pageId = links[choice - 1]->getTarget();
        auto next = m_book->getPageById(pageId);
        if (!next) {
            std::cout << "Page not found: " << pageId << std::endl;
        }
        m_currentPage = next;
        return true;
    }
};

/**
 * @brief Main function demonstrating NLB Reader usage
 */
int main(int argc, char* argv[]) {
    system("chcp 65001");
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <path_to_nlb_directory> [mode]" << std::endl;
        std::cout << "Modes: info (default) | explore" << std::endl;
        return 1;
    }
    
    std::string nlbPath = argv[1];
    std::string mode = (argc > 2) ? argv[2] : "info";
    
    if (mode == "explore") {
        auto book = new NonLinearBookImpl();
        ConsoleProgressData progressData;
        book->load(nlbPath, progressData);
        std::cout << "Loaded: " << book->getTitle() << " by " << book->getAuthor() << std::endl;
        NLBExplorer explorer(book, book->getStartPoint());
        explorer.explore();
    } else {
        NLBReader reader;
        reader.readBook(nlbPath);
    }
    
    return 0;
}
