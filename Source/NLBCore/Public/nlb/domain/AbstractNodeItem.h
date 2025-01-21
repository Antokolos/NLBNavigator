#pragma once

#include <vector>
#include <memory>
#include <string>
#include "nlb/domain/AbstractModifyingItem.h"
#include "nlb/api/ModifyingItem.h"
#include "nlb/api/Modification.h"
#include "nlb/api/NLBCommand.h"
#include "nlb/api/NodeItem.h"
#include "nlb/domain/ModificationImpl.h"
#include "nlb/domain/ObserverHandler.h"

class FileManipulator;
class NonLinearBook;
class NonLinearBookImpl;
class CoordsImpl;
class LinkImpl;
class Link;
class Page;

class AbstractNodeItem : public AbstractModifyingItem, public NodeItem {
public:
    static const std::string COORDS_DIR_NAME;
    static const std::string LINKS_DIR_NAME;
    static const std::string LNKORDER_FILE_NAME;
    static const std::string LNKORDER_SEPARATOR;
    static const std::string CONTENT_FILE_NAME;
    static const std::string CONTENT_SEPARATOR;
    static const std::string DEFTAGID_FILE_NAME;
    static const std::string STROKE_FILE_NAME;
    static const std::string FILL_FILE_NAME;
    static const std::string TEXTCOLOR_FILE_NAME;

    enum class Orientation {
        TOP,
        BOTTOM,
        LEFT,
        RIGHT
    };

    class ResizeNodeCommand : public NLBCommand {
    public:
        ResizeNodeCommand(Orientation orientation, double deltaX, double deltaY,
                         const std::vector<std::shared_ptr<Link>>& adjacentLinks);
        void execute() override;
        void revert() override;

    private:
        Orientation m_orientation;
        double m_deltaX;
        double m_deltaY;
        std::vector<std::shared_ptr<Link>> m_adjacentLinks;
    };

    class AddLinkCommand : public NLBCommand {
    public:
        explicit AddLinkCommand(std::shared_ptr<LinkImpl> link);
        void execute() override;
        void revert() override;

    private:
        std::shared_ptr<LinkImpl> m_link;
    };

    class DeleteLinkCommand : public NLBCommand {
    public:
        explicit DeleteLinkCommand(std::shared_ptr<Link> link);
        void execute() override;
        void revert() override;

    private:
        std::shared_ptr<LinkImpl> m_link;
        bool m_previousDeletedState;
    };

    class SortLinksCommand : public NLBCommand {
    public:
        SortLinksCommand(AbstractNodeItem* nodeItem, const std::vector<std::string>& newSortingOrder);
        void execute() override;
        void revert() override;

    private:
        AbstractNodeItem* m_nodeItem;
        std::vector<std::string> m_previousSortingOrder;
        std::vector<std::string> m_newSortingOrder;
    };

    AbstractNodeItem();
    explicit AbstractNodeItem(std::shared_ptr<NonLinearBook> currentNLB);
    AbstractNodeItem(float left, float top, std::shared_ptr<NonLinearBook> currentNLB);
    AbstractNodeItem(const std::shared_ptr<NodeItem>& nodeItem, std::shared_ptr<NonLinearBook> currentNLB);
    virtual ~AbstractNodeItem() = default;

    // Getters and setters
    std::string getDefaultTagId() const;
    void setDefaultTagId(const std::string& defaultTagId);
    std::string getStroke() const;
    void setStroke(const std::string& stroke);
    std::string getFill() const;
    void setFill(const std::string& fill);
    std::string getTextColor() const;
    void setTextColor(const std::string& textColor);
    
    std::vector<std::string> getContainedObjIds() const;
    void addContainedObjId(const std::string& containedObjId);
    void removeContainedObjId(const std::string& containedObjId);
    
    std::shared_ptr<Coords> getCoords() const;
    std::vector<std::shared_ptr<Link>> getLinks() const;
    std::vector<std::shared_ptr<LinkImpl>> getLinkImpls() const;
    size_t getLinkCount() const;
    void addLink(std::shared_ptr<LinkImpl> link);
    std::shared_ptr<Link> getLinkById(const std::string& linkId) const;

    // Observer methods
    std::string addObserver(std::shared_ptr<NLBObserver> observer);
    void removeObserver(const std::string& observerId);
    void notifyObservers();

    // File operations
    void writeNodeItemProperties(const std::shared_ptr<FileManipulator>& fileManipulator,
                               const std::string& nodeDir,
                               std::shared_ptr<NonLinearBookImpl> nonLinearBook);
    void readNodeItemProperties(const std::string& nodeDir);

    std::string getExternalHierarchy() const;

protected:
    std::shared_ptr<ResizeNodeCommand> createResizeNodeCommand(
            Orientation orientation, double deltaX, double deltaY,
            const std::vector<std::shared_ptr<Link>>& adjacentLinks);
    std::shared_ptr<ResizeNodeCommand> createResizeNodeCommand(
            Orientation orientation, double deltaX, double deltaY);
    std::shared_ptr<AddLinkCommand> createAddLinkCommand(std::shared_ptr<LinkImpl> link);
    std::shared_ptr<DeleteLinkCommand> createDeleteLinkCommand(std::shared_ptr<Link> link);
    std::shared_ptr<SortLinksCommand> createSortLinksCommand(
            const std::vector<std::shared_ptr<Link>>& newSortingOrder);

private:
    void applyLinkSortingOrder(const std::vector<std::string>& sortingOrder);
    void resizeNode(Orientation orientation, double deltaX, double deltaY);
    void writeLinkOrderFile(const std::shared_ptr<FileManipulator>& fileManipulator,
                           const std::string& nodeDir);
    void writeContent(const std::shared_ptr<FileManipulator>& fileManipulator,
                     const std::string& nodeDir,
                     std::shared_ptr<NonLinearBookImpl> nonLinearBook);
    void writeCoords(const std::shared_ptr<FileManipulator>& fileManipulator,
                    const std::string& nodeDir);
    void writeLinks(const std::shared_ptr<FileManipulator>& fileManipulator,
                   const std::string& nodeDir);
    
    void readContent(const std::string& nodeDir);
    void readCoords(const std::string& nodeDir);
    void readLinks(const std::string& nodeDir);

    std::string m_defaultTagId;
    std::string m_stroke;
    std::string m_fill;
    std::string m_textColor;
    std::shared_ptr<CoordsImpl> m_coords;
    std::vector<std::shared_ptr<LinkImpl>> m_links;
    std::vector<std::string> m_containedObjIds;
    std::shared_ptr<ObserverHandler> m_observerHandler;
};