#pragma once

#include "nlb/api/NLBCommand.h"
#include <memory>

class NonLinearBookImpl;
class Link;
class LinkImpl;

/*!
 * Command that updates the coordinates of a link, allowing to modify its position
 * and height while maintaining the ability to revert these changes.
 */
class UpdateLinkCoordsCommand : public NLBCommand {
public:
    /*!
     * Constructs a command to update link position.
     * 
     * @param nonLinearBook The book containing the link
     * @param link The link to be updated
     * @param left New left coordinate
     * @param top New top coordinate
     */
    UpdateLinkCoordsCommand(
        std::shared_ptr<NonLinearBookImpl> nonLinearBook,
        std::shared_ptr<Link> link,
        float left,
        float top
    );

    /*!
     * Constructs a command to update link height.
     * 
     * @param nonLinearBook The book containing the link
     * @param link The link to be updated
     * @param height New height value
     */
    UpdateLinkCoordsCommand(
        std::shared_ptr<NonLinearBookImpl> nonLinearBook,
        std::shared_ptr<Link> link,
        float height
    );

    /*!
     * Executes the coordinate update operation.
     */
    void execute() override;

    /*!
     * Reverts the coordinate changes back to their previous values.
     */
    void revert() override;

private:
    /*!
     * Internal constructor that initializes the common parts for both public constructors.
     */
    UpdateLinkCoordsCommand(
        std::shared_ptr<NonLinearBookImpl> nonLinearBook,
        std::shared_ptr<Link> link
    );

    std::shared_ptr<LinkImpl> m_linkImpl;
    float m_left;
    float m_top;
    float m_height;
    float m_leftPrev;
    float m_topPrev;
    float m_heightPrev;
};