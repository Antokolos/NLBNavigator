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
    
    // Попытка получить AbstractNodeItem как PageImpl
    std::shared_ptr<AbstractNodeItem> nodeItem = std::static_pointer_cast<AbstractNodeItem>(
        nonLinearBook->getPageImplById(parent->getId())
    );
    
    // Если не найден как страница, ищем как объект
    if (!nodeItem) {
        nodeItem = std::static_pointer_cast<AbstractNodeItem>(
            nonLinearBook->getObjImplById(parent->getId())
        );
    }

    if (!nodeItem) {
        throw std::runtime_error("Failed to get node item for link with id: " + link->getId());
    }
    
    // Получаем LinkImpl из nodeItem
    m_linkImpl = std::static_pointer_cast<LinkImpl>(nodeItem->getLinkById(link->getId()));
    if (!m_linkImpl) {
        throw std::runtime_error("Failed to get link implementation for link with id: " + link->getId());
    }

    // Сохраняем текущие координаты
    const Coords& coords = m_linkImpl->getCoords();
    m_leftPrev = coords.getLeft();
    m_topPrev = coords.getTop();
    m_heightPrev = coords.getHeight();
}

UpdateLinkCoordsCommand::UpdateLinkCoordsCommand(
    std::shared_ptr<NonLinearBookImpl> nonLinearBook,
    std::shared_ptr<Link> link,
    float left,
    float top
) : UpdateLinkCoordsCommand(nonLinearBook, link)
{
    // Устанавливаем новые координаты, высота остается прежней
    m_left = left;
    m_top = top;
    m_height = m_heightPrev;
}

UpdateLinkCoordsCommand::UpdateLinkCoordsCommand(
    std::shared_ptr<NonLinearBookImpl> nonLinearBook,
    std::shared_ptr<Link> link,
    float height
) : UpdateLinkCoordsCommand(nonLinearBook, link)
{
    // Устанавливаем новую высоту, координаты остаются прежними
    m_left = m_leftPrev;
    m_top = m_topPrev;
    m_height = height;
}

void UpdateLinkCoordsCommand::execute() {
    const Coords& coords = m_linkImpl->getCoords();
    const CoordsImpl* coordsImpl = static_cast<const CoordsImpl*>(&coords);
    
    // Применяем новые координаты
    const_cast<CoordsImpl*>(coordsImpl)->setLeft(m_left);
    const_cast<CoordsImpl*>(coordsImpl)->setTop(m_top);
    const_cast<CoordsImpl*>(coordsImpl)->setHeight(m_height);
    
    // Уведомляем наблюдателей об изменениях
    m_linkImpl->notifyObservers();
}

void UpdateLinkCoordsCommand::revert() {
    const Coords& coords = m_linkImpl->getCoords();
    const CoordsImpl* coordsImpl = static_cast<const CoordsImpl*>(&coords);
    
    // Восстанавливаем предыдущие координаты
    const_cast<CoordsImpl*>(coordsImpl)->setLeft(m_leftPrev);
    const_cast<CoordsImpl*>(coordsImpl)->setTop(m_topPrev);
    const_cast<CoordsImpl*>(coordsImpl)->setHeight(m_heightPrev);
    
    // Уведомляем наблюдателей об изменениях
    m_linkImpl->notifyObservers();
}