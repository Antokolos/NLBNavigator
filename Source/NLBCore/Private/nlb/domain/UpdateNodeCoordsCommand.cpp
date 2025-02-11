#include "nlb/domain/UpdateNodeCoordsCommand.h"

#include "nlb/domain/NonLinearBookImpl.h"
#include "nlb/api/NodeItem.h"
#include "nlb/domain/AbstractNodeItem.h"
#include "nlb/api/Link.h"
#include "nlb/api/Coords.h"
#include "nlb/api/Link.h"
#include "nlb/api/Obj.h"
#include "nlb/api/Page.h"
#include "nlb/domain/CoordsImpl.h"
#include "nlb/domain/LinkImpl.h"
#include "nlb/domain/ObjImpl.h"
#include "nlb/domain/PageImpl.h"

UpdateNodeCoordsCommand::UpdateNodeCoordsCommand(
    std::shared_ptr<NonLinearBookImpl> nonLinearBook,
    std::shared_ptr<NodeItem> node
) {
    m_nodeItem = nonLinearBook->getPageImplById(node->getId());
    if (!m_nodeItem) {
        m_nodeItem = nonLinearBook->getObjImplById(node->getId());
    }
    m_associatedLinks = nonLinearBook->getAssociatedLinks(node);
    
    std::shared_ptr<Coords> coords = m_nodeItem->getCoords();
    m_leftPrev = coords->getLeft();
    m_topPrev = coords->getTop();
    m_heightPrev = coords->getHeight();
    m_widthPrev = coords->getWidth();
}

UpdateNodeCoordsCommand::UpdateNodeCoordsCommand(
    std::shared_ptr<NonLinearBookImpl> nonLinearBook,
    std::shared_ptr<NodeItem> node,
    float deltaX,
    float deltaY
) : UpdateNodeCoordsCommand(nonLinearBook, node) {
    m_width = m_widthPrev;
    m_height = m_heightPrev;
    
    std::shared_ptr<Coords> coords = m_nodeItem->getCoords();
    m_left = coords->getLeft() + deltaX;
    m_top = coords->getTop() + deltaY;
}

UpdateNodeCoordsCommand::UpdateNodeCoordsCommand(
    std::shared_ptr<NonLinearBookImpl> nonLinearBook,
    std::shared_ptr<NodeItem> node,
    float left,
    float top,
    float width,
    float height
) : UpdateNodeCoordsCommand(nonLinearBook, node) {
    m_width = width;
    m_height = height;
    m_left = left;
    m_top = top;
}

void UpdateNodeCoordsCommand::execute() {
    std::shared_ptr<CoordsImpl> coords = std::static_pointer_cast<CoordsImpl>(m_nodeItem->getCoords());
    coords->setLeft(m_left);
    coords->setTop(m_top);
    coords->setWidth(m_width);
    coords->setHeight(m_height);
    m_nodeItem->notifyObservers();
    
    for (const auto& link : m_associatedLinks) {
        link->notifyObservers();
    }
}

void UpdateNodeCoordsCommand::revert() {
    std::shared_ptr<CoordsImpl> coords = std::static_pointer_cast<CoordsImpl>(m_nodeItem->getCoords());
    coords->setLeft(m_leftPrev);
    coords->setTop(m_topPrev);
    coords->setWidth(m_widthPrev);
    coords->setHeight(m_heightPrev);
    m_nodeItem->notifyObservers();
    
    for (const auto& link : m_associatedLinks) {
        link->notifyObservers();
    }
}