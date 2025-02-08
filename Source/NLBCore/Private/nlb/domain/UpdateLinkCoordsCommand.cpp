#include "nlb/domain/UpdateLinkCoordsCommand.h"
#include "nlb/domain/PageImpl.h"
#include "nlb/domain/LinkImpl.h"
#include "nlb/domain/ObjImpl.h"
#include "nlb/domain/CoordsImpl.h"
#include "nlb/domain/NonLinearBookImpl.h"
#include "nlb/api/Link.h"
#include "nlb/api/IdentifiableItem.h"
#include "nlb/domain/AbstractNodeItem.h"
#include "nlb/api/Coords.h"
#include <stdexcept>

UpdateLinkCoordsCommand::UpdateLinkCoordsCommand(
    std::shared_ptr<NonLinearBookImpl> nonLinearBook,
    std::shared_ptr<Link> link
) {
    std::shared_ptr<IdentifiableItem> parent = link->getParent();
    // auto pageImpl = nonLinearBook->getPageImplById(parent->getId());
    // AbstractNodeItem* rawPagePtr = static_cast<AbstractNodeItem*>(pageImpl.get());
    // std::shared_ptr<AbstractNodeItem> nodeItem(pageImpl, rawPagePtr);
    std::shared_ptr<AbstractNodeItem> nodeItem = std::static_pointer_cast<AbstractNodeItem>(
        nonLinearBook->getPageImplById(parent->getId())
    );
    
    if (!nodeItem) {
        // auto objImpl = nonLinearBook->getObjImplById(parent->getId());
        // AbstractNodeItem* rawObjPtr = static_cast<AbstractNodeItem*>(objImpl.get());
        // nodeItem = std::shared_ptr<AbstractNodeItem>(objImpl, rawObjPtr);
        nodeItem = std::static_pointer_cast<AbstractNodeItem>(
            nonLinearBook->getObjImplById(parent->getId())
        );
    }

    if (!nodeItem) {
        throw std::runtime_error("Failed to get node item");
    }
    
    m_linkImpl = std::static_pointer_cast<LinkImpl>(nodeItem->getLinkById(link->getId()));
    if (!m_linkImpl) {
        throw std::runtime_error("Failed to get link implementation");
    }

    const Coords& coords = m_linkImpl->getCoords();
    const CoordsImpl* coordsImpl = static_cast<const CoordsImpl*>(&coords);
    
    m_leftPrev = coordsImpl->getLeft();
    m_topPrev = coordsImpl->getTop();
    m_heightPrev = coordsImpl->getHeight();
}

UpdateLinkCoordsCommand::UpdateLinkCoordsCommand(
    std::shared_ptr<NonLinearBookImpl> nonLinearBook,
    std::shared_ptr<Link> link,
    float left,
    float top
) : UpdateLinkCoordsCommand(nonLinearBook, link)
{
    m_height = m_heightPrev;
    m_left = left;
    m_top = top;
}

UpdateLinkCoordsCommand::UpdateLinkCoordsCommand(
    std::shared_ptr<NonLinearBookImpl> nonLinearBook,
    std::shared_ptr<Link> link,
    float height
) : UpdateLinkCoordsCommand(nonLinearBook, link)
{
    m_height = height;
    m_left = m_leftPrev;
    m_top = m_topPrev;
}

void UpdateLinkCoordsCommand::execute() {
    const Coords& coords = m_linkImpl->getCoords();
    const CoordsImpl* coordsImpl = static_cast<const CoordsImpl*>(&coords);
    
    const_cast<CoordsImpl*>(coordsImpl)->setLeft(m_left);
    const_cast<CoordsImpl*>(coordsImpl)->setTop(m_top);
    const_cast<CoordsImpl*>(coordsImpl)->setHeight(m_height);
    m_linkImpl->notifyObservers();
}

void UpdateLinkCoordsCommand::revert() {
    const Coords& coords = m_linkImpl->getCoords();
    const CoordsImpl* coordsImpl = static_cast<const CoordsImpl*>(&coords);
    
    const_cast<CoordsImpl*>(coordsImpl)->setLeft(m_leftPrev);
    const_cast<CoordsImpl*>(coordsImpl)->setTop(m_topPrev);
    const_cast<CoordsImpl*>(coordsImpl)->setHeight(m_heightPrev);
    m_linkImpl->notifyObservers();
}