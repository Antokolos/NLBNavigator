#pragma once

#include "nlb/api/NLBCommand.h"

#include <vector>
#include <memory>
#include <string>

class NonLinearBookImpl;
class NodeItem;
class AbstractNodeItem;
class Link;

/*!
 * Command for updating node coordinates and dimensions
 */
class UpdateNodeCoordsCommand : public NLBCommand {
public:
    /*!
     * Constructor for moving node by delta
     * @param nonLinearBook The book containing the node
     * @param node The node to update
     * @param deltaX The change in X coordinate
     * @param deltaY The change in Y coordinate
     */
    UpdateNodeCoordsCommand(
        std::shared_ptr<NonLinearBookImpl> nonLinearBook,
        std::shared_ptr<NodeItem> node,
        float deltaX,
        float deltaY
    );

    /*!
     * Constructor for resizing and moving node
     * @param nonLinearBook The book containing the node
     * @param node The node to update
     * @param left New left coordinate
     * @param top New top coordinate
     * @param width New width
     * @param height New height
     */
    UpdateNodeCoordsCommand(
        std::shared_ptr<NonLinearBookImpl> nonLinearBook,
        std::shared_ptr<NodeItem> node,
        float left,
        float top,
        float width,
        float height
    );

    /*!
     * Executes the command, updating node coordinates
     */
    void execute() override;

    /*!
     * Reverts the command, restoring previous coordinates
     */
    void revert() override;

private:
    /*!
     * Helper constructor for initialization
     */
    UpdateNodeCoordsCommand(
        std::shared_ptr<NonLinearBookImpl> nonLinearBook,
        std::shared_ptr<NodeItem> node
    );

    std::shared_ptr<AbstractNodeItem> m_nodeItem;
    std::vector<std::shared_ptr<Link>> m_associatedLinks;
    float m_left;
    float m_top;
    float m_height;
    float m_width;
    float m_leftPrev;
    float m_topPrev;
    float m_heightPrev;
    float m_widthPrev;
};