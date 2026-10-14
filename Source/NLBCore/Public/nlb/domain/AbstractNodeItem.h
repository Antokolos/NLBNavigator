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
class PageImpl;
class ObjImpl;

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

    class ResizeNodeCommand : public NLBCommand {
    public:
        ResizeNodeCommand(AbstractNodeItem* nodeItem, Orientation orientation, double deltaX, double deltaY,
                         const std::vector<Link*>& adjacentLinks);
        void execute() override;
        void revert() override;

    private:
        AbstractNodeItem* m_nodeItem;
        Orientation m_orientation;
        double m_deltaX;
        double m_deltaY;
        std::vector<Link*> m_adjacentLinks;
    };

    class AddLinkCommand : public NLBCommand {
    public:
        explicit AddLinkCommand(AbstractNodeItem* nodeItem, LinkImpl* link);
        void execute() override;
        void revert() override;

    private:
        AbstractNodeItem* m_nodeItem;
        LinkImpl* m_link;
    };

    class DeleteLinkCommand : public NLBCommand {
    public:
        explicit DeleteLinkCommand(AbstractNodeItem* nodeItem, Link* link);
        void execute() override;
        void revert() override;

    private:
        AbstractNodeItem* m_nodeItem;
        LinkImpl* m_link;
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
    explicit AbstractNodeItem(NonLinearBook* currentNLB);
    AbstractNodeItem(NonLinearBook* currentNLB, float left, float top);
    AbstractNodeItem(const NodeItem* nodeItem, NonLinearBook* currentNLB);
    virtual ~AbstractNodeItem() = default;

    // Getters and setters
    virtual std::string getDefaultTagId() const override;
    void setDefaultTagId(const std::string& defaultTagId);
    virtual std::string getStroke() const override;
    void setStroke(const std::string& stroke);
    virtual std::string getFill() const override;
    void setFill(const std::string& fill);
    virtual std::string getTextColor() const override;
    void setTextColor(const std::string& textColor);
    
    virtual std::vector<std::string> getContainedObjIds() const override;
    void addContainedObjId(const std::string& containedObjId);
    void removeContainedObjId(const std::string& containedObjId);
    
    virtual Coords* getCoords() const override;
    virtual std::vector<Link*> getLinks() const override;
    std::vector<LinkImpl*> getLinkImpls() const;
    size_t getLinkCount() const;
    void addLink(LinkImpl* link);
    virtual Link* getLinkById(const std::string& linkId) const override;
    void removeLinkById(const std::string& linkId);
    static void filterTargetLinkList(
        AbstractNodeItem* target,
        AbstractNodeItem* source,
        const std::vector<std::string>& linkIdsToBeExcluded
    );

    // Observer methods
    std::string addObserver(NLBObserver* observer);
    void removeObserver(const std::string& observerId);
    void notifyObservers();

    // File operations
    void writeNodeItemProperties(const FileManipulator* fileManipulator,
                               const std::string& nodeDir,
                               NonLinearBookImpl* nonLinearBook);
    void readNodeItemProperties(const std::string& nodeDir);

    virtual std::string getExternalHierarchy() const override;

    // Команды создания объектов
    virtual NLBCommand* createPageCommand(float left, float top);
    virtual NLBCommand* createLinkCommand(const std::string& pageId);
    virtual NLBCommand* createObjCommand(float left, float top);
    
    ResizeNodeCommand* createResizeNodeCommand(
            Orientation orientation, double deltaX, double deltaY,
            const std::vector<Link*>& adjacentLinks);
    ResizeNodeCommand* createResizeNodeCommand(
            Orientation orientation, double deltaX, double deltaY);
    AddLinkCommand* createAddLinkCommand(LinkImpl* link);
    DeleteLinkCommand* createDeleteLinkCommand(Link* link);
    SortLinksCommand* createSortLinksCommand(const std::vector<Link*>& newSortingOrder);
    
    // Методы для файловой системы
    virtual void writeToFile(const FileManipulator* fileManipulator, 
                           const std::string& nodesDir,
                           const std::string& nodeDirName,
                           NonLinearBookImpl* nonLinearBook);
                           
    // Методы валидации
    virtual void validateLinks();

    virtual std::string getId() const override { return AbstractModifyingItem::getId(); }
    virtual std::string getFullId() const override { return AbstractModifyingItem::getFullId(); }
    virtual bool isDeleted() const override { return AbstractModifyingItem::isDeleted(); }
    virtual IdentifiableItem* getParent() const override { return AbstractModifyingItem::getParent(); }
    virtual bool hasDeletedParent() const override { return AbstractModifyingItem::hasDeletedParent(); }
    virtual NonLinearBook* getCurrentNLB() const override { return AbstractModifyingItem::getCurrentNLB(); }
    virtual SearchResult* searchText(const SearchContract& contract) const override { return AbstractModifyingItem::searchText(contract); }

    virtual std::vector<Modification*> getModifications() const override { return AbstractModifyingItem::getModifications(); }
    virtual bool hasNoModifications() const override { return AbstractModifyingItem::hasNoModifications(); }
    virtual Modification* getModificationById(const std::string& modId) const override { return AbstractModifyingItem::getModificationById(modId); }

private:
    void applyLinkSortingOrder(const std::vector<std::string>& sortingOrder);
    void resizeNode(Orientation orientation, double deltaX, double deltaY);
    void writeLinkOrderFile(const FileManipulator* fileManipulator,
                           const std::string& nodeDir);
    void writeContent(const FileManipulator* fileManipulator,
                     const std::string& nodeDir,
                     NonLinearBookImpl* nonLinearBook);
    void writeCoords(const FileManipulator* fileManipulator,
                    const std::string& nodeDir);
    void writeLinks(const FileManipulator* fileManipulator,
                   const std::string& nodeDir);
    
    void readContent(const std::string& nodeDir);
    void readCoords(const std::string& nodeDir);
    void readLinks(const std::string& nodeDir);

    std::string m_defaultTagId;
    std::string m_stroke;
    std::string m_fill;
    std::string m_textColor;
    CoordsImpl* m_coords;
    std::vector<LinkImpl*> m_links;
    std::vector<std::string> m_containedObjIds;
    ObserverHandler* m_observerHandler;
};
