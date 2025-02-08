#include "nlb/domain/ChangeContainerCommand.h"
#include "nlb/domain/AbstractNodeItem.h"
#include "nlb/domain/ObjImpl.h"

inline ChangeContainerCommand::ChangeContainerCommand(
    std::shared_ptr<AbstractNodeItem> prevContainer,
    std::shared_ptr<AbstractNodeItem> newContainer,
    std::shared_ptr<ObjImpl> obj
) : m_prevContainer(prevContainer)
  , m_newContainer(newContainer)
  , m_obj(obj)
{
}

inline void ChangeContainerCommand::execute() {
    if (m_prevContainer) {
        m_prevContainer->removeContainedObjId(m_obj->getId());
    }
    
    if (m_newContainer) {
        m_newContainer->addContainedObjId(m_obj->getId());
        m_obj->setContainerId(m_newContainer->getId());
    } else {
        m_obj->setContainerId("");
    }
    
    m_obj->notifyObservers();
}

inline void ChangeContainerCommand::revert() {
    if (m_newContainer) {
        m_newContainer->removeContainedObjId(m_obj->getId());
    }
    
    if (m_prevContainer) {
        m_prevContainer->addContainedObjId(m_obj->getId());
        m_obj->setContainerId(m_prevContainer->getId());
    } else {
        m_obj->setContainerId("");
    }
    
    m_obj->notifyObservers();
}