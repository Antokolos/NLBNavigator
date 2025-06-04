#include "nlb/domain/AbstractNodeItem.h"
#include "nlb/domain/CoordsImpl.h"
#include "nlb/domain/LinkImpl.h"
#include "nlb/util/FileManipulator.h"
#include "nlb/util/StringHelper.h"
#include "nlb/exception/NLBExceptions.h"
#include "nlb/domain/NonLinearBookImpl.h"
#include "nlb/api/Page.h"
#include "nlb/api/CoordsLw.h"

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

std::string AbstractNodeItem::addObserver(std::shared_ptr<NLBObserver> observer) {
    return m_observerHandler ? m_observerHandler->addObserver(observer) : nlb::Constants::EMPTY_STRING;
}

void AbstractNodeItem::removeObserver(const std::string& observerId) {
    if (m_observerHandler) {
        m_observerHandler->removeObserver(observerId);
    }
}

void AbstractNodeItem::notifyObservers() {
    if (m_observerHandler) {
        m_observerHandler->notifyObservers();
    }
}

void AbstractNodeItem::writeNodeItemProperties(const std::shared_ptr<FileManipulator>& fileManipulator,
                                           const std::string& nodeDir,
                                           std::shared_ptr<NonLinearBookImpl> nonLinearBook) {
    fileManipulator->writeOptionalString(nodeDir, STROKE_FILE_NAME, m_stroke, DEFAULT_STROKE);
    fileManipulator->writeOptionalString(nodeDir, FILL_FILE_NAME, m_fill, DEFAULT_FILL);
    fileManipulator->writeOptionalString(nodeDir, TEXTCOLOR_FILE_NAME, m_textColor, DEFAULT_TEXTCOLOR);
    fileManipulator->writeOptionalString(nodeDir, DEFTAGID_FILE_NAME, m_defaultTagId, DEFAULT_TAG_ID);
    
    // Запись порядка ссылок
    writeLinkOrderFile(fileManipulator, nodeDir);
    
    // Запись содержимого
    writeContent(fileManipulator, nodeDir, nonLinearBook);
    
    // Запись координат
    writeCoords(fileManipulator, nodeDir);
    
    // Запись ссылок
    writeLinks(fileManipulator, nodeDir);
}

void AbstractNodeItem::readNodeItemProperties(const std::string& nodeDir) {
    m_stroke = FileManipulator::getOptionalFileAsString(
        nodeDir, STROKE_FILE_NAME, DEFAULT_STROKE);
    
    m_fill = FileManipulator::getOptionalFileAsString(
        nodeDir, FILL_FILE_NAME, DEFAULT_FILL);
    
    m_textColor = FileManipulator::getOptionalFileAsString(
        nodeDir, TEXTCOLOR_FILE_NAME, DEFAULT_TEXTCOLOR);
    
    m_defaultTagId = FileManipulator::getOptionalFileAsString(
        nodeDir, DEFTAGID_FILE_NAME, DEFAULT_TAG_ID);
    
    // Чтение содержимого
    readContent(nodeDir);
    
    // Чтение координат
    readCoords(nodeDir);
    
    // Чтение ссылок
    readLinks(nodeDir);
}

std::shared_ptr<NLBCommand> AbstractNodeItem::createPageCommand(float left, float top) {
    // Создание команды для страницы
    // Эта реализация зависит от других классов, поэтому дана упрощенная версия
    return nullptr; // В реальной реализации возвращает команду
}

std::shared_ptr<NLBCommand> AbstractNodeItem::createLinkCommand(const std::string& pageId) {
    // Создание команды для ссылки
    // Эта реализация зависит от других классов, поэтому дана упрощенная версия
    return nullptr; // В реальной реализации возвращает команду
}

std::shared_ptr<NLBCommand> AbstractNodeItem::createObjCommand(float left, float top) {
    // Создание команды для объекта
    // Эта реализация зависит от других классов, поэтому дана упрощенная версия
    return nullptr; // В реальной реализации возвращает команду
}

void AbstractNodeItem::writeToFile(const std::shared_ptr<FileManipulator>& fileManipulator, 
                                 const std::string& nodesDir,
                                 const std::string& nodeDirName,
                                 std::shared_ptr<NonLinearBookImpl> nonLinearBook) {
    const std::string nodeDir = nodesDir + "/" + nodeDirName;
    
    if (AbstractIdentifiableItem::isDeleted()) {
        fileManipulator->deleteFileOrDir(nodeDir);
    } else {
        fileManipulator->createDir(nodeDir, 
            "Cannot create node directory for node with Id = " + AbstractIdentifiableItem::getId());
        
        writeNodeItemProperties(fileManipulator, nodeDir, nonLinearBook);
        
        // Запись модификаций
        writeModOrderFile(fileManipulator, nodeDir);
        writeModifications(fileManipulator, nodeDir);
    }
}

void AbstractNodeItem::validateLinks() {
    // Удаление недействительных ссылок
    auto it = m_links.begin();
    while (it != m_links.end()) {
        if ((*it)->isDeleted() || (*it)->hasDeletedParent()) {
            it = m_links.erase(it);
        } else {
            ++it;
        }
    }
}

Coords& AbstractNodeItem::getRelativeCoords() const {
    // В Java версии этот метод возвращает CoordsLw.getZeroCoords()
    return CoordsLw::getZeroCoords();
}

void AbstractNodeItem::writeLinkOrderFile(const std::shared_ptr<FileManipulator>& fileManipulator,
                                        const std::string& nodeDir) {
    std::string content;
    const int lastElemIndex = static_cast<int>(m_links.size()) - 1;
    
    if (lastElemIndex >= 0) {
        for (int i = 0; i < lastElemIndex; i++) {
            const auto& link = m_links[i];
            if (!link->isDeleted()) {
                content += link->getId() + LNKORDER_SEPARATOR;
            }
        }
        
        if (!m_links[lastElemIndex]->isDeleted()) {
            content += m_links[lastElemIndex]->getId();
        }
        
        fileManipulator->writeOptionalString(nodeDir, LNKORDER_FILE_NAME, content, "");
    } else {
        fileManipulator->writeOptionalString(nodeDir, LNKORDER_FILE_NAME, "", "");
    }
}

void AbstractNodeItem::writeContent(const std::shared_ptr<FileManipulator>& fileManipulator,
                                  const std::string& nodeDir,
                                  std::shared_ptr<NonLinearBookImpl> nonLinearBook) {
    std::string content;
    const int lastElemIndex = static_cast<int>(m_containedObjIds.size()) - 1;
    
    if (lastElemIndex >= 0) {
        for (int i = 0; i < lastElemIndex; i++) {
            content += m_containedObjIds[i] + CONTENT_SEPARATOR;
        }
        content += m_containedObjIds[lastElemIndex];
        
        fileManipulator->writeOptionalString(nodeDir, CONTENT_FILE_NAME, content, "");
    } else {
        fileManipulator->writeOptionalString(nodeDir, CONTENT_FILE_NAME, "", "");
    }
}

void AbstractNodeItem::writeCoords(const std::shared_ptr<FileManipulator>& fileManipulator,
                                 const std::string& nodeDir) {
    const std::string coordsDir = FileUtils::combinePath(nodeDir, COORDS_DIR_NAME);
    fileManipulator->createDir(coordsDir,
        "Cannot create node coords directory for node with Id = " + AbstractIdentifiableItem::getId());
    
    auto coordsImpl = std::static_pointer_cast<CoordsImpl>(m_coords);
    coordsImpl->writeCoords(*fileManipulator, coordsDir);
}

void AbstractNodeItem::writeLinks(const std::shared_ptr<FileManipulator>& fileManipulator,
                                const std::string& nodeDir) {
    const std::string linksDir = FileUtils::combinePath(nodeDir, LINKS_DIR_NAME);
    
    if (m_links.empty()) {
        if (FileUtils::exists(linksDir)) {
            fileManipulator->deleteFileOrDir(linksDir);
        }
    } else {
        fileManipulator->createDir(linksDir,
            "Cannot create node links directory for node with Id = " + AbstractIdentifiableItem::getId());
        
        for (const auto& link : m_links) {
            link->writeLink(fileManipulator, linksDir);
        }
    }
}

void AbstractNodeItem::readContent(const std::string& nodeDir) {
    std::string contentString = FileManipulator::getOptionalFileAsString(
        nodeDir,
        CONTENT_FILE_NAME,
        ""
    );

    m_containedObjIds.clear();
    
    if (!contentString.empty()) {
        std::vector<std::string> objIds = StringHelper::tokenize(contentString, CONTENT_SEPARATOR);
        m_containedObjIds = objIds;
    }
}

void AbstractNodeItem::readCoords(const std::string& nodeDir) {
    const std::string coordsDir = FileUtils::combinePath(nodeDir, COORDS_DIR_NAME);
    if (!FileUtils::exists(coordsDir)) {
        throw NLBIOException(
            "Invalid NLB structure: coords directory does not exist for node with Id = " + AbstractIdentifiableItem::getId()
        );
    }
    
    auto coordsImpl = std::static_pointer_cast<CoordsImpl>(m_coords);
    coordsImpl->read(coordsDir);
}

void AbstractNodeItem::readLinks(const std::string& nodeDir) {
    std::string linkOrderString = FileManipulator::getOptionalFileAsString(
        nodeDir,
        LNKORDER_FILE_NAME,
        ""
    );

    const std::string linksDir = FileUtils::combinePath(nodeDir, LINKS_DIR_NAME);
    if (!FileUtils::exists(linksDir) && !linkOrderString.empty()) {
        throw NLBIOException(
            "Invalid NLB structure: links directory does not exist for node with Id = " 
            + AbstractIdentifiableItem::getId() 
            + ", but some links should be specified"
        );
    }

    m_links.clear();
    
    if (FileUtils::exists(linksDir)) {
        std::vector<std::string> linkDirs = FileUtils::getDirectoryFiles(linksDir);
        
        if (linkOrderString.empty()) {
            // Если нет файла порядка, читаем ссылки в произвольном порядке
            for (const auto& linkDirName : linkDirs) {
                std::string linkDir = FileUtils::combinePath(linksDir, linkDirName);
                if (FileUtils::isDirectory(linkDir)) {
                    auto link = std::make_shared<LinkImpl>(
                        std::enable_shared_from_this<AbstractNodeItem>::shared_from_this());
                    link->readLink(linkDir);
                    m_links.push_back(link);
                }
            }
        } else {
            // Если есть файл порядка, читаем ссылки в указанном порядке
            std::vector<std::string> linkOrderList = StringHelper::tokenize(linkOrderString, LNKORDER_SEPARATOR);
            std::vector<std::string> linkDirsSortedList = createSortedDirList(linkDirs, linkOrderList);

            for (const auto& linkDirName : linkDirsSortedList) {
                std::string linkDir = FileUtils::combinePath(linksDir, linkDirName);
                auto link = std::make_shared<LinkImpl>(
                    std::enable_shared_from_this<AbstractNodeItem>::shared_from_this());
                link->readLink(linkDir);
                m_links.push_back(link);
            }
        }
    }
}

void AbstractNodeItem::addLink(std::shared_ptr<LinkImpl> link) {
    m_links.push_back(link);
    notifyObservers();
}

std::vector<std::shared_ptr<LinkImpl>> AbstractNodeItem::getLinkImpls() const {
    return m_links;
}

size_t AbstractNodeItem::getLinkCount() const {
    return m_links.size();
}

void AbstractNodeItem::resizeNode(Orientation orientation, double deltaX, double deltaY) {
    std::shared_ptr<Coords> coordsPtr = getCoords();
    CoordsImpl* coords = static_cast<CoordsImpl*>(coordsPtr.get());
    
    float minNodeWidth = static_cast<float>(MIN_NODE_WIDTH);
    float minNodeHeight = static_cast<float>(MIN_NODE_HEIGHT);
    
    switch (orientation) {
    case Orientation::TOP:
        if (coords->getHeight() - deltaY >= minNodeHeight) {
            coords->setTop(coords->getTop() + static_cast<float>(deltaY));
            coords->setHeight(coords->getHeight() - static_cast<float>(deltaY));
        }
        break;
    case Orientation::BOTTOM:
        if (coords->getHeight() + deltaY >= minNodeHeight) {
            coords->setHeight(coords->getHeight() + static_cast<float>(deltaY));
        }
        break;
    case Orientation::LEFT:
        if (coords->getWidth() - deltaX >= minNodeWidth) {
            coords->setLeft(coords->getLeft() + static_cast<float>(deltaX));
            coords->setWidth(coords->getWidth() - static_cast<float>(deltaX));
        }
        break;
    case Orientation::RIGHT:
        if (coords->getWidth() + deltaX >= minNodeWidth) {
            coords->setWidth(coords->getWidth() + static_cast<float>(deltaX));
        }
        break;
    }
}

void AbstractNodeItem::addContainedObjId(const std::string& containedObjId) {
    if (std::find(m_containedObjIds.begin(), m_containedObjIds.end(), containedObjId) == m_containedObjIds.end()) {
        m_containedObjIds.push_back(containedObjId);
        notifyObservers();
    }
}

void AbstractNodeItem::removeContainedObjId(const std::string& containedObjId) {
    auto it = std::find(m_containedObjIds.begin(), m_containedObjIds.end(), containedObjId);
    if (it != m_containedObjIds.end()) {
        m_containedObjIds.erase(it);
        notifyObservers();
    }
}