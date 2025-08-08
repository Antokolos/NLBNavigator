#include "nlb/domain/AbstractIdentifiableItem.h"
#include "nlb/api/DummyNLB.h"
#include "nlb/util/MultiLangString.h"
#include "nlb/api/SearchContract.h"
#include "nlb/api/ModifyingItem.h"
#include "nlb/domain/SearchResult.h"
#include "nlb/util/StringHelper.h"
#include "nlb/util/QuotationHelper.h"
#include "nlb/util/UUID.h"
#include "nlb/exception/NLBExceptions.h"

AbstractIdentifiableItem::AbstractIdentifiableItem()
    : m_id(NLBUUID::randomUUID())
    , m_isDeleted(false)
    , m_currentNLB(DummyNLB::singleton())
    , m_parent(nullptr) {
}

AbstractIdentifiableItem::AbstractIdentifiableItem(NonLinearBook* currentNLB)
    : AbstractIdentifiableItem() {
    m_currentNLB = currentNLB;
}

AbstractIdentifiableItem::AbstractIdentifiableItem(
    const IdentifiableItem* identifiableItem,
    NonLinearBook* currentNLB
)
    : AbstractIdentifiableItem(currentNLB) {
    m_id = identifiableItem->getId();
    m_isDeleted = identifiableItem->isDeleted();
    m_parent = identifiableItem->getParent();
}

AbstractIdentifiableItem::AbstractIdentifiableItem(
    const IdentifiableItem* identifiableItem,
    const ModifyingItem* parent,
    NonLinearBook* currentNLB
)
    : AbstractIdentifiableItem(currentNLB) {
    m_id = identifiableItem->getId();
    m_isDeleted = identifiableItem->isDeleted();
    m_parent = (IdentifiableItem*) parent;
}

void AbstractIdentifiableItem::copy(const IdentifiableItem* identifiableItem) {
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

IdentifiableItem* AbstractIdentifiableItem::getParent() const {
    return m_parent;
}

void AbstractIdentifiableItem::setParent(const IdentifiableItem* parent) {
    m_parent = (IdentifiableItem*) parent;
}

bool AbstractIdentifiableItem::hasDeletedParent() const {
    IdentifiableItem* item = getParent();
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
    IdentifiableItem* item = getParent();
    ids.push_back(getId());
    
    while (item) {
        ids.push_back(item->getId());
        item = item->getParent();
    }
    
    return StringHelper::formatSequence(ids);
}

void AbstractIdentifiableItem::setDeleted(bool deleted) {
    m_isDeleted = deleted;
}

bool AbstractIdentifiableItem::isDeleted() const {
    return m_isDeleted;
}

NonLinearBook* AbstractIdentifiableItem::getCurrentNLB() const {
    return m_currentNLB;
}

SearchResult* AbstractIdentifiableItem::searchText(const SearchContract& contract) const {
    if (contract.isSearchInIds() && textMatches(m_id, contract)) {
        auto result = new SearchResult();
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
