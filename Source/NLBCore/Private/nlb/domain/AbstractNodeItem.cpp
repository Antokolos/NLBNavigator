#include "nlb/domain/AbstractNodeItem.h"
#include "nlb/domain/CoordsImpl.h"
#include "nlb/domain/LinkImpl.h"
#include "nlb/util/FileManipulator.h"
#include "nlb/util/StringHelper.h"
#include "nlb/exception/NLBExceptions.h"
#include "nlb/domain/NonLinearBookImpl.h"
#include "nlb/api/Page.h"

const std::string AbstractNodeItem::COORDS_DIR_NAME = "coords";
const std::string AbstractNodeItem::LINKS_DIR_NAME = "links";
const std::string AbstractNodeItem::LNKORDER_FILE_NAME = "lnkorder";
const std::string AbstractNodeItem::LNKORDER_SEPARATOR = "\n";
const std::string AbstractNodeItem::CONTENT_FILE_NAME = "content";
const std::string AbstractNodeItem::CONTENT_SEPARATOR = "\n";
const std::string AbstractNodeItem::DEFTAGID_FILE_NAME = "deftagid";
const std::string AbstractNodeItem::STROKE_FILE_NAME = "stroke";
const std::string AbstractNodeItem::FILL_FILE_NAME = "fill";
const std::string AbstractNodeItem::TEXTCOLOR_FILE_NAME = "txtcolor";

// ResizeNodeCommand implementation
AbstractNodeItem::ResizeNodeCommand::ResizeNodeCommand(
        Orientation orientation, double deltaX, double deltaY,
        const std::vector<std::shared_ptr<Link>>& adjacentLinks)
    : m_orientation(orientation)
    , m_deltaX(deltaX)
    , m_deltaY(deltaY)
    , m_adjacentLinks(adjacentLinks) {
}

void AbstractNodeItem::ResizeNodeCommand::execute() {
    // Implementation
}

void AbstractNodeItem::ResizeNodeCommand::revert() {
    // Implementation
}

// AddLinkCommand implementation
AbstractNodeItem::AddLinkCommand::AddLinkCommand(std::shared_ptr<LinkImpl> link)
    : m_link(link) {
}

void AbstractNodeItem::AddLinkCommand::execute() {
    m_link->setDeleted(false);
    // Implementation for adding link
}

void AbstractNodeItem::AddLinkCommand::revert() {
    m_link->setDeleted(true);
    // Implementation for removing link
}

// DeleteLinkCommand implementation
AbstractNodeItem::DeleteLinkCommand::DeleteLinkCommand(std::shared_ptr<Link> link)
    //: m_link(std::dynamic_pointer_cast<LinkImpl>(link))
    : m_link(std::static_pointer_cast<LinkImpl>(link))
    , m_previousDeletedState(m_link->AbstractIdentifiableItem::isDeleted()) {
}

void AbstractNodeItem::DeleteLinkCommand::execute() {
    m_link->setDeleted(true);
    m_link->notifyObservers();
}

void AbstractNodeItem::DeleteLinkCommand::revert() {
    m_link->setDeleted(m_previousDeletedState);
    m_link->notifyObservers();
}

// SortLinksCommand implementation
AbstractNodeItem::SortLinksCommand::SortLinksCommand(
        AbstractNodeItem* nodeItem,
        const std::vector<std::string>& newSortingOrder)
    : m_nodeItem(nodeItem)
    , m_newSortingOrder(newSortingOrder) {
    
    for (const auto& link : nodeItem->getLinks()) {
        m_previousSortingOrder.push_back(link->getId());
    }
}

void AbstractNodeItem::SortLinksCommand::execute() {
    m_nodeItem->applyLinkSortingOrder(m_newSortingOrder);
    m_nodeItem->notifyObservers();
}

void AbstractNodeItem::SortLinksCommand::revert() {
    m_nodeItem->applyLinkSortingOrder(m_previousSortingOrder);
    m_nodeItem->notifyObservers();
}

// AbstractNodeItem implementation
AbstractNodeItem::AbstractNodeItem()
    : AbstractModifyingItem()
    , m_defaultTagId(DEFAULT_TAG_ID)
    , m_stroke(DEFAULT_STROKE)
    , m_fill(DEFAULT_FILL)
    , m_textColor(DEFAULT_TEXTCOLOR)
    , m_coords(std::make_shared<CoordsImpl>())
    , m_observerHandler(std::make_shared<ObserverHandler>()) {
}

AbstractNodeItem::AbstractNodeItem(std::shared_ptr<NonLinearBook> currentNLB)
    : AbstractModifyingItem(currentNLB)
    , m_defaultTagId(DEFAULT_TAG_ID)
    , m_stroke(DEFAULT_STROKE)
    , m_fill(DEFAULT_FILL)
    , m_textColor(DEFAULT_TEXTCOLOR)
    , m_coords(std::make_shared<CoordsImpl>())
    , m_observerHandler(std::make_shared<ObserverHandler>()) {
}

AbstractNodeItem::AbstractNodeItem(std::shared_ptr<NonLinearBook> currentNLB, float left, float top)
    : AbstractModifyingItem(currentNLB)
    , m_defaultTagId(DEFAULT_TAG_ID)
    , m_stroke(DEFAULT_STROKE)
    , m_fill(DEFAULT_FILL)
    , m_textColor(DEFAULT_TEXTCOLOR)
    , m_coords(std::make_shared<CoordsImpl>())
    , m_observerHandler(std::make_shared<ObserverHandler>()) {
    
    m_coords->setLeft(left);
    m_coords->setTop(top);
    m_coords->setWidth(DEFAULT_NODE_WIDTH);
    m_coords->setHeight(DEFAULT_NODE_HEIGHT);
}

AbstractNodeItem::AbstractNodeItem(
        const std::shared_ptr<NodeItem>& nodeItem,
        std::shared_ptr<NonLinearBook> currentNLB)
    : AbstractModifyingItem(nodeItem, currentNLB)
    , m_defaultTagId(nodeItem->getDefaultTagId())
    , m_stroke(nodeItem->getStroke())
    , m_fill(nodeItem->getFill())
    , m_textColor(nodeItem->getTextColor())
    , m_coords(std::make_shared<CoordsImpl>())
    , m_observerHandler(std::make_shared<ObserverHandler>()) {
    
    auto coords = nodeItem->getCoords();
    m_coords->setHeight(coords->getHeight());
    m_coords->setWidth(coords->getWidth());
    m_coords->setLeft(coords->getLeft());
    m_coords->setTop(coords->getTop());
    
    for (const auto& link : nodeItem->getLinks()) {
        auto linkImpl = std::static_pointer_cast<LinkImpl>(link);
        m_links.push_back(linkImpl);
    }
    
    m_containedObjIds = nodeItem->getContainedObjIds();
}

void AbstractNodeItem::applyLinkSortingOrder(const std::vector<std::string>& sortingOrder) {
    std::vector<std::shared_ptr<LinkImpl>> sortedLinks;
    // TODO: Optimize sorting
    for (const auto& linkId : sortingOrder) {
        auto it = std::find_if(m_links.begin(), m_links.end(),
                              [&linkId](const auto& link) {
                                  return link->getId() == linkId;
                              });
        if (it != m_links.end()) {
            sortedLinks.push_back(*it);
            m_links.erase(it);
        }
    }
    // m_links can be not empty. Add remaining elements to the tail
    sortedLinks.insert(sortedLinks.end(), m_links.begin(), m_links.end());
    m_links = std::move(sortedLinks);
}

// Getter implementations
std::string AbstractNodeItem::getDefaultTagId() const {
    return m_defaultTagId;
}

std::string AbstractNodeItem::getStroke() const {
    return m_stroke;
}

std::string AbstractNodeItem::getFill() const {
    return m_fill;
}

std::string AbstractNodeItem::getTextColor() const {
    return m_textColor;
}

std::vector<std::string> AbstractNodeItem::getContainedObjIds() const {
    return m_containedObjIds;
}

std::shared_ptr<Coords> AbstractNodeItem::getCoords() const {
    return m_coords;
}

std::vector<std::shared_ptr<Link>> AbstractNodeItem::getLinks() const {
    std::vector<std::shared_ptr<Link>> result;
    result.reserve(m_links.size());
    for (const auto& link : m_links) {
        result.push_back(link);
    }
    return result;
}

std::shared_ptr<Link> AbstractNodeItem::getLinkById(const std::string& linkId) const {
    auto it = std::find_if(m_links.begin(), m_links.end(),
                          [&linkId](const auto& link) {
                              return link->getId() == linkId;
                          });
    return (it != m_links.end()) ? *it : nullptr;
}

void AbstractNodeItem::notifyObservers() {
    if (m_observerHandler) {
        m_observerHandler->notifyObservers();
    }
}

std::string AbstractNodeItem::getExternalHierarchy() const {
    std::vector<std::string> hierarchy;
    auto currentNLB = AbstractIdentifiableItem::getCurrentNLB();
    
    // Build hierarchy by traversing up through parent pages
    while (true) {
        auto parentPage = currentNLB->getParentPage();
        if (!parentPage) {
            break;
        }
        
        if (parentPage->isModuleExternal()) {
            hierarchy.push_back(parentPage->getModuleName());
        }
        
        currentNLB = parentPage->getCurrentNLB();
    }
    
    // Build the result string
    std::string result;
    for (auto it = hierarchy.rbegin(); it != hierarchy.rend(); ++it) {
        result += *it;
        if (std::next(it) != hierarchy.rend()) {
            result += "/";
        }
    }
    
    return result;
}