#pragma once

#include "nlb/api/NLBCommand.h"
#include <memory>

class AbstractNodeItem;
class ObjImpl;

/*!
 * Command that changes the container of an object, handling the removal from
 * the previous container and addition to the new container while maintaining
 * proper object relationships and observer notifications.
 */
class ChangeContainerCommand : public NLBCommand {
public:
    /*!
     * Constructs a new ChangeContainerCommand.
     * 
     * @param prevContainer The previous container of the object (can be nullptr)
     * @param newContainer The new container for the object (can be nullptr)
     * @param obj The object whose container is being changed
     */
    ChangeContainerCommand(
        AbstractNodeItem* prevContainer,
        AbstractNodeItem* newContainer,
        ObjImpl* obj
    );

    /*!
     * Executes the container change operation.
     * Removes the object from its previous container (if any),
     * adds it to the new container (if any), and updates the object's
     * container ID accordingly.
     */
    void execute() override;

    /*!
     * Reverts the container change operation.
     * Restores the object to its previous container state by removing it
     * from the current container and adding it back to the previous one.
     */
    void revert() override;

private:
    AbstractNodeItem* m_prevContainer;
    AbstractNodeItem* m_newContainer;
    ObjImpl* m_obj;
};
