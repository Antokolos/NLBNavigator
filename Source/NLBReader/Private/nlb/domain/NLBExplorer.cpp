#include "nlb/domain/NLBExplorer.h"

#include "nlb/api/Page.h"
#include "nlb/api/Obj.h"

#include "nlb/api/Variable.h"
#include "nlb/api/MediaFile.h"
#include "nlb/exception/NLBExceptions.h"
#include <iostream>
#include <memory>

#include "shunting-yard.h"
#include "nlb/api/Variable.h"

const std::string NLBExplorer::INVENTORY = "inventory";
const std::string NLBExplorer::TRUE = "true";
const std::string NLBExplorer::FALSE = "false";

inline NLBExplorer::NLBExplorer(NonLinearBook* book, const std::string& currentPageId): m_book(book)
{
    m_currentPage = m_book->getPageById(currentPageId);
    if (!cparse::GlobalScope::default_global().find(TRUE)) {
        cparse::GlobalScope::default_global()[TRUE] = true;
    }
    if (!cparse::GlobalScope::default_global().find(FALSE)) {
        cparse::GlobalScope::default_global()[FALSE] = false;
    }
    if (!cparse::GlobalScope::default_global().find(INVENTORY)) {
        cparse::GlobalScope::default_global()[INVENTORY] = cparse::TokenList();
    }
    for (auto var : m_book->getVariables()) {
        if (cparse::GlobalScope::default_global().find(var->getName())) continue;
        switch (var->getType())
        {
        case Variable::Type::VAR:
            switch (var->getDataType())
            {
            case Variable::DataType::STRING:
                cparse::GlobalScope::default_global()[var->getName()] = "";
                break;
            case Variable::DataType::AUTO:
                cparse::GlobalScope::default_global()[var->getName()] = 0;
                break;
            case Variable::DataType::BOOLEAN:
                cparse::GlobalScope::default_global()[var->getName()] = false;
                break;
            case Variable::DataType::NUMBER:
                cparse::GlobalScope::default_global()[var->getName()] = 0;
                break;
            }
            break;
        case Variable::Type::PAGE:
        case Variable::Type::TIMER:
        case Variable::Type::OBJ:
        case Variable::Type::LINK:
            cparse::GlobalScope::default_global()[var->getName()] = false;
            break;
        case Variable::Type::OBJCONSTRAINT:
        case Variable::Type::OBJREF:
        case Variable::Type::LINKCONSTRAINT:
        case Variable::Type::EXPRESSION:
        case Variable::Type::TAG:
        case Variable::Type::MODCONSTRAINT:
        case Variable::Type::AUTOWIRECONSTRAINT:
        default:
            break;
        }
    }
}

inline void NLBExplorer::explore()
{
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

void NLBExplorer::executeModifications(const ModifyingItem* item)
{
    for (auto modification : item->getModifications()) {
        const Variable *var = m_book->getVariableById(modification->getVarId());
        const Variable *expr = m_book->getVariableById(modification->getExprId());
        std::string exprString = expr->getValue();
        const char *exprSZ = exprString.c_str();
        switch (modification->getType()) {
        case Modification::Type::ASSIGN:
            switch (var->getDataType())
            {
            case Variable::DataType::STRING:
                cparse::GlobalScope::default_global()[var->getName()] = cparse::calculator::calculate(exprSZ, cparse::GlobalScope::default_global()).asString();
                break;
            case Variable::DataType::AUTO:
                cparse::GlobalScope::default_global()[var->getName()] = cparse::calculator::calculate(exprSZ, cparse::GlobalScope::default_global()).asInt();
                break;
            case Variable::DataType::BOOLEAN:
                cparse::GlobalScope::default_global()[var->getName()] = cparse::calculator::calculate(exprSZ, cparse::GlobalScope::default_global()).asBool();
                break;
            case Variable::DataType::NUMBER:
                cparse::GlobalScope::default_global()[var->getName()] = cparse::calculator::calculate(exprSZ, cparse::GlobalScope::default_global()).asInt();
                break;
            }
            break;
        case Modification::Type::ADDINV:
            cparse::GlobalScope::default_global()[INVENTORY].asList().push(exprString);
            break;
        case Modification::Type::TAG:
        case Modification::Type::GETTAG:
        case Modification::Type::WHILE:
        case Modification::Type::IF:
        case Modification::Type::IFHAVE:
        case Modification::Type::ELSE:
        case Modification::Type::ELSEIF:
        case Modification::Type::END:
        case Modification::Type::RETURN:
        case Modification::Type::HAVE:
        case Modification::Type::CLONE:
        case Modification::Type::CNTNR:
        case Modification::Type::ID:
        case Modification::Type::ADD:
        case Modification::Type::ADDU:
        case Modification::Type::ADDALL:
        case Modification::Type::ADDALLU:
        case Modification::Type::REMOVE:
        case Modification::Type::RMINV:
        case Modification::Type::CLEAR:
        case Modification::Type::CLRINV:
        case Modification::Type::OBJS:
        case Modification::Type::SSND:
        case Modification::Type::WSND:
        case Modification::Type::SND:
        case Modification::Type::SPUSH:
        case Modification::Type::WPUSH:
        case Modification::Type::PUSH:
        case Modification::Type::POP:
        case Modification::Type::SINJECT:
        case Modification::Type::INJECT:
        case Modification::Type::EJECT:
        case Modification::Type::SHUFFLE:
        case Modification::Type::PRN:
        case Modification::Type::DSC:
        case Modification::Type::PDSC:
        case Modification::Type::PDSCS:
        case Modification::Type::ACT:
        case Modification::Type::ACTT:
        case Modification::Type::ACTF:
        case Modification::Type::USE:
        case Modification::Type::SIZE:
        case Modification::Type::RND:
        case Modification::Type::ACHMAX:
        case Modification::Type::ACHIEVE:
        case Modification::Type::ACHIEVED:
        case Modification::Type::GOTO:
        case Modification::Type::SNAPSHOT:
        case Modification::Type::COUNTGET:
        case Modification::Type::COUNTRST:
        case Modification::Type::OPENURL:
        case Modification::Type::WINGEOM:
        case Modification::Type::INVGEOM:
        case Modification::Type::WINCOLOR:
        case Modification::Type::INVCOLOR:
            std::cout << "!!! MODIFICATION TYPE NOT IMPLEMENTED: " << Modification::typeToString(modification->getType()) << std::endl;
            break;

        }
    }
}

bool NLBExplorer::showPageAndGetNextChoice(Page* page)
{
    if (!page) return false;
    if (!page->getVarId().empty()) {
        cparse::GlobalScope::default_global()[m_book->getVariableById(page->getVarId())->getName()] = true;
    }
    executeModifications(page);
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "PAGE: " << page->getCaption() << " (" << page->getFullId() << ")" << std::endl;
    std::cout << std::string(50, '=') << std::endl;

    // Show page text
    std::cout << page->getText() << std::endl;

    cparse::TokenList &inventory = cparse::GlobalScope::default_global()[INVENTORY].asList();
    if (!inventory.list().empty()) {
        std::cout << std::endl;
        std::cout << INVENTORY << ":";
        for (auto item : inventory.list()) {
            std::cout << " " << item.asString();
        }
        std::cout << std::endl;
    }

    // Show available links
    auto links = page->getLinks();
    auto module = page->getModule();

    std::cout << "\nChoose your next action:" << std::endl;
    size_t choiceIdx = 1;
    std::map<size_t, size_t> linksMap;
    for (size_t i = 0; i < links.size(); ++i) {
        if (!links[i]->getConstrId().empty()) {
            Variable *constraint = m_book->getVariableById(links[i]->getConstrId());
            std::string exprString = constraint->getValue();
            const char *expr = exprString.c_str();
            if (!cparse::calculator::calculate(expr, cparse::GlobalScope::default_global()).asBool()) {
                continue;
            }
        }
        linksMap[choiceIdx] = i;
        std::cout << choiceIdx++ << ". " << links[i]->getText() << std::endl;
    }
    size_t traverseChoice = 0;
    size_t returnChoice = 0;
    bool hasModule = module && !module->isEmpty();
    if (hasModule) {
        traverseChoice = choiceIdx;
        std::cout << traverseChoice << ". " << page->getTraverseText() << std::endl;
    }
    bool hasReturn = page->isFinish() && !page->isAutoReturn();
    if (!hasModule && hasReturn) {
        returnChoice = traverseChoice > 0 ? traverseChoice + 1 : choiceIdx;
        std::cout << returnChoice << ". " << page->getReturnText() << std::endl;
    }
    std::cout << "0. Exit" << std::endl;

    // Get user choice
    size_t choice;
    std::cout << "\nYour choice: ";
    std::cin >> choice;

    if (choice == 0) {
        m_currentPage = nullptr;
        return false;
    }

    if (hasModule && choice == traverseChoice) {
        NLBExplorer explorer(module, module->getStartPoint());
        explorer.explore();
        m_currentPage = explorer.m_currentPage;
        return true;
    }

    if (hasReturn && choice == returnChoice) {
        return false;
    }

    if (choice < 1 || choice >= choiceIdx) {
        std::cout << "Invalid choice. Try again." << std::endl;
        return true; // Stay on current page
    }

    auto link = links[linksMap[choice]];
    std::string pageId = link->getTarget();
    auto next = m_book->getPageById(pageId);
    if (!next) {
        std::cout << "Page not found: " << pageId << std::endl;
    }
    if (!link->getVarId().empty()) {
        cparse::GlobalScope::default_global()[m_book->getVariableById(link->getVarId())->getName()] = true;
        executeModifications(link);
    }
    m_currentPage = next;
    return true;
}
