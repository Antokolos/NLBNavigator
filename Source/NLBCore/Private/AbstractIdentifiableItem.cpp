#include "AbstractIdentifiableItem.h"
#include "DummyNLB.h"
#include "MultiLangString.h"
#include "SearchContract.h"
#include "SearchResult.h"
#include "StringHelper.h"
#include "QuotationHelper.h"
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
    std::list<std::string> ids;
    std::shared_ptr<IdentifiableItem> item = std::dynamic_pointer_cast<IdentifiableItem>(getParent());
    ids.push_front(getId());
    
    while (item) {
        ids.push_front(item->getId());
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