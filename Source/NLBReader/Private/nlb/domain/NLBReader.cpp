#include "nlb/domain/NLBReader.h"

#include "nlb/api/ConsoleProgressData.h"
#include "nlb/api/Page.h"
#include "nlb/api/Obj.h"
#include "nlb/api/Link.h"

#include "nlb/api/MediaFile.h"
#include "nlb/exception/NLBExceptions.h"
#include <iostream>
#include <memory>

void NLBReader::readBook(const std::string& nlbPath) {
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
    }
    catch (const NLBIOException& e) {
        std::cerr << "Error reading NLB: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
    }
}

void NLBReader::displayBookInfo(NonLinearBook* book)
{
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

void NLBReader::displayContentOverview(NonLinearBook* book)
{
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

void NLBReader::displayNavigationStructure(NonLinearBook* book)
{
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

void NLBReader::displayPageNavigation(NonLinearBook* book, Page* page, int depth, std::set<std::string>& visited)
{
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

void NLBReader::displayMediaFiles(NonLinearBook* book)
{
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

void NLBReader::validateBook(NonLinearBook* book)
{
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

std::string NLBReader::themeToString(Theme theme)
{
    switch (theme) {
    case Theme::STANDARD: return "Standard";
    case Theme::VN: return "VN";
    default: return "Unknown";
    }
}

std::string NLBReader::dataTypeToString(Variable::DataType dataType)
{
    switch (dataType) {
    case Variable::DataType::STRING: return "String";
    case Variable::DataType::NUMBER: return "Number";
    case Variable::DataType::BOOLEAN: return "Boolean";
    default: return "Unknown";
    }
}
