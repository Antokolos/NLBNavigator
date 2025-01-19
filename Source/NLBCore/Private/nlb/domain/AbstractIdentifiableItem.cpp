#include "nlb/domain/AbstractIdentifiableItem.h"
#include "nlb/api/DummyNLB.h"
#include "nlb/util/MultiLangString.h"
#include "nlb/api/SearchContract.h"
#include "nlb/api/ModifyingItem.h"
#include "nlb/domain/SearchResult.h"
#include "nlb/util/StringHelper.h"
#include "nlb/util/QuotationHelper.h"
#include "nlb/exception/NLBExceptions.h"

#include <algorithm>
#include <iterator>
#include <random>
#include <sstream>
#include <iomanip>

std::string AbstractIdentifiableItem::generateUUID() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static std::uniform_int_distribution<> dis2(8, 11);

    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    
    for (int i = 0; i < 8; i++) 
        ss << std::setw(1) << dis(gen);
    ss << "-";
    for (int i = 0; i < 4; i++) 
        ss << std::setw(1) << dis(gen);
    ss << "-4";
    for (int i = 0; i < 3; i++) 
        ss << std::setw(1) << dis(gen);
    ss << "-" << std::setw(1) << dis2(gen);
    for (int i = 0; i < 3; i++) 
        ss << std::setw(1) << dis(gen);
    ss << "-";
    for (int i = 0; i < 12; i++) 
        ss << std::setw(1) << dis(gen);
    
    return ss.str();
}

AbstractIdentifiableItem::AbstractIdentifiableItem()
    : m_id(generateUUID())
    , m_isDeleted(false)
    , m_currentNLB(DummyNLB::singleton())
    , m_parent(nullptr) {
}

AbstractIdentifiableItem::AbstractIdentifiableItem(std::shared_ptr<NonLinearBook> currentNLB)
    : AbstractIdentifiableItem() {
    m_currentNLB = currentNLB;
}

AbstractIdentifiableItem::AbstractIdentifiableItem(
    const std::shared_ptr<IdentifiableItem>& identifiableItem,
    std::shared_ptr<NonLinearBook> currentNLB
)
    : AbstractIdentifiableItem(currentNLB) {
    m_id = identifiableItem->getId();
    m_isDeleted = identifiableItem->isDeleted();
    m_parent = identifiableItem->getParent();
}

AbstractIdentifiableItem::AbstractIdentifiableItem(
    const std::shared_ptr<IdentifiableItem>& identifiableItem,
    const std::shared_ptr<ModifyingItem>& parent,
    std::shared_ptr<NonLinearBook> currentNLB
)
    : AbstractIdentifiableItem(currentNLB) {
    m_id = identifiableItem->getId();
    m_isDeleted = identifiableItem->isDeleted();
    m_parent = parent;
}

void AbstractIdentifiableItem::copy(const std::shared_ptr<IdentifiableItem>& identifiableItem) {
    m_id = identifiableItem->getId();
    m_isDeleted = identifiableItem->isDeleted();
    m_parent = identifiableItem->getParent();
    m_currentNLB = identifiableItem->getCurrentNLB();
}

void AbstractIdentifiableItem::setId(const std::string& id) {
    m_id = id;
}

std::string AbstractIdentifiableItem::getId() const {
    return m_id;
}

std::shared_ptr<IdentifiableItem> AbstractIdentifiableItem::getParent() const {
    return m_parent;
}

void AbstractIdentifiableItem::setParent(const std::shared_ptr<IdentifiableItem>& parent) {
    m_parent = parent;
}

bool AbstractIdentifiableItem::hasDeletedParent() const {
    std::shared_ptr<IdentifiableItem> item = getParent();
    while (item) {
        if (item->isDeleted()) {
            return true;
        }
        item = item->getParent();
    }
    return false;
}

std::string AbstractIdentifiableItem::getFullId() const {
    std::vector<std::string> ids;
    std::shared_ptr<IdentifiableItem> item = getParent();
    ids.push_back(getId());
    
    while (item) {
        ids.push_back(item->getId());
        item = item->getParent();
    }
    
    return StringHelper::formatSequence(ids);
}

bool AbstractIdentifiableItem::isDeleted() const {
    return m_isDeleted;
}

std::shared_ptr<NonLinearBook> AbstractIdentifiableItem::getCurrentNLB() const {
    return m_currentNLB;
}

std::shared_ptr<SearchResult> AbstractIdentifiableItem::searchText(const SearchContract& contract) const {
    if (contract.isSearchInIds() && textMatches(m_id, contract)) {
        auto result = std::make_shared<SearchResult>();
        result->setId(m_id);
        result->setInformation(m_id);
        return result;
    }
    return nullptr;
}

bool AbstractIdentifiableItem::textMatches(const std::string& stringToTest, const SearchContract& contract) const {
    std::string patternText;
    if (contract.isWholeWords()) {
        patternText += "\\b";
    }
    patternText += contract.getSearchText();
    if (contract.isWholeWords()) {
        patternText += "\\b";
    }

    std::regex::flag_type flags = std::regex::ECMAScript;
    if (contract.isIgnoreCase()) {
        flags |= std::regex::icase;
    }

    std::regex pattern(patternText, flags);
    bool result = std::regex_search(stringToTest, pattern);
    
    if (contract.isFindUnusualQuotes()) {
        return result || QuotationHelper::find(stringToTest);
    }
    return result;
}

bool AbstractIdentifiableItem::textMatches(const MultiLangString& mlsToTest, const SearchContract& contract) const {
    for (const auto& text : mlsToTest.values()) {
        if (textMatches(text, contract)) {
            return true;
        }
    }
    return false;
}

std::vector<std::string> AbstractIdentifiableItem::createSortedDirList(
    const std::vector<std::string>& dirs,
    const std::vector<std::string>& orderList) const {

    // Скопировать dirs, так как список будет модифицироваться
    std::vector<std::string> dirsList = dirs;

    std::vector<std::string> result;

    for (const auto& dirName : orderList) {
        auto it = std::find(dirsList.begin(), dirsList.end(), dirName);
        if (it != dirsList.end()) {
            result.push_back(*it);
            dirsList.erase(it); // Удаляем найденный элемент
        } else {
            throw NLBConsistencyException(
                "Inconsistent NLB structure: cannot locate directory with name = " + dirName +
                " for item with id = " + this->getId());
        }
    }

    if (!dirsList.empty()) {
        std::ostringstream oss;
        std::copy(dirsList.begin(), dirsList.end(), std::ostream_iterator<std::string>(oss, ", "));
        throw NLBConsistencyException(
            "Inconsistent NLB structure: directories with names = [" + oss.str() +
            "] for item with id = " + this->getId() + " cannot be located in the order file");
    }

    return result;
}