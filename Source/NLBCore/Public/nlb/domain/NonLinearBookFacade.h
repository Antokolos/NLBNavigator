#pragma once

#include <map>
#include <list>
#include <vector>
#include <set>
#include <memory>
#include "nlb/api/NLBObservable.h"
#include "nlb/api/MediaFile.h"
#include "nlb/api/Obj.h"
#include "nlb/api/LinksTableModel.h"
#include "nlb/domain/MediaExportParameters.h"

class NLBObserver;
class ModifyingItem;
class ModificationsTableModel;
class NodeItem;
class NonLinearBook;
class NonLinearBookImpl;
class Page;
class PageImpl;
class ObjImpl;
class Link;
class LinkImpl;
class UndoManager;
class CommandChainCommand;
class MediaExportParameters;
class Author;
class ObserverHandler;
class VCSAdapter;
class ProgressData;
class MultiLangString;

/*!
 * @brief Facade class for managing non-linear book operations
 */
class NonLinearBookFacade : public NLBObservable, public std::enable_shared_from_this<NonLinearBookFacade> {
private:
    std::shared_ptr<NonLinearBookImpl> m_nlb;
    std::map<std::string, std::shared_ptr<PageImpl>> m_newPagesPool;
    std::map<std::string, std::shared_ptr<ObjImpl>> m_newObjsPool;
    std::map<std::string, std::shared_ptr<LinkImpl>> m_newLinksPool;
    
    /*!
     * @brief Main undo manager
     */
    std::shared_ptr<UndoManager> m_undoManager;
    
    /*!
     * @brief Items' own undo managers
     */
    std::map<std::string, std::shared_ptr<UndoManager>> m_undoManagersMap;
    
    std::shared_ptr<ObserverHandler> m_observerHandler;
    std::shared_ptr<VCSAdapter> m_vcsAdapter;
    std::shared_ptr<Author> m_author;
    bool m_rootFacade;
    std::shared_ptr<NonLinearBookFacade> m_parentFacade;
    std::vector<std::shared_ptr<NonLinearBookFacade>> m_moduleFacades;

    std::shared_ptr<UndoManager> getUndoManagerByItemId(const std::string& id);
    void clearUndos();
    void clearPools();
    void exportMedia(const std::string& exportDir);
    void exportAdditionalMedia(const std::string& exportDir);
    void updateNodeCoords(std::shared_ptr<CommandChainCommand> commandChain, std::shared_ptr<NodeItem> nodeItem, float deltaX, float deltaY);
    void offsetContainedObjects(std::shared_ptr<CommandChainCommand> commandChain, std::shared_ptr<NodeItem> container, float deltaX, float deltaY);
    void saveNLB(bool create, std::shared_ptr<ProgressData> progressData);

public:
    NonLinearBookFacade(std::shared_ptr<Author> author, std::shared_ptr<VCSAdapter> vcsAdapter);
    NonLinearBookFacade(std::shared_ptr<NonLinearBookFacade> parentFacade, std::shared_ptr<Author> author, std::shared_ptr<VCSAdapter> vcsAdapter, std::shared_ptr<NonLinearBookImpl> nlb);
    virtual ~NonLinearBookFacade() = default;

    void createNewBook();
    std::shared_ptr<NonLinearBookFacade> createModuleFacade(const std::string& modulePageId);
    std::shared_ptr<NonLinearBookFacade> getMainFacade();
    std::shared_ptr<NonLinearBookFacade> getParentFacade() { return m_parentFacade; }
    std::shared_ptr<NonLinearBook> getNlb() { return std::static_pointer_cast<NonLinearBook>(m_nlb); }

    void clear();
    void clearUndosAndPools();
    void commit(const std::string& commitMessageText);
    void pull(const std::string& userName, const std::string& password, std::shared_ptr<ProgressData> progressData);
    void push(const std::string& userName, const std::string& password, std::shared_ptr<ProgressData> progressData);

    void exportToChoiceScript(const std::string& exportDir);
    void exportToQSPTextFile(const std::string& exportDir);
    void exportToURQTextFile(const std::string& exportDir);
    void exportToPDFFile(const std::string& exportFile);
    void exportToTXTFile(const std::string& exportDir);
    void exportToHTMLFile(const std::string& exportDir);
    void exportToJSIQFile(const std::string& exportDir);
    void exportToSTEADFile(const std::string& exportDir);
    void exportToVNSTEADFile(const std::string& exportDir);
    void exportToASMFile(const std::string& exportDir);

    void updateModifications(std::shared_ptr<ModifyingItem> modifyingItem, std::shared_ptr<ModificationsTableModel> modificationsTableModel);
    void setMediaFileConstrId(MediaFile::Type mediaType, const std::string& fileName, const std::string& constrId);
    void setMediaFileRedirect(MediaFile::Type mediaType, const std::string& fileName, const std::string& redirect);
    void setMediaFileFlag(MediaFile::Type mediaType, const std::string& fileName, bool flag);
    void setMediaFileExportParametersPreset(MediaFile::Type mediaType, const std::string& fileName, MediaExportParameters::Preset preset);

    void updateBookProperties(
        const std::string& license,
        std::shared_ptr<Theme> theme,
        const std::string& language,
        const std::string& title,
        const std::string& author,
        const std::string& version,
        const std::string& perfectGameAchievementName,
        bool fullAutowire,
        bool suppressMedia,
        bool suppressSound,
        bool propagateToSubmodules
    );

    void updatePage(
        std::shared_ptr<Page> page,
        const std::string& imageFileName,
        bool imageBackground,
        bool imageAnimated,
        const std::string& soundFileName,
        bool soundSFX,
        const std::string& pageVariableName,
        const std::string& pageTimerVariableName,
        const std::string& pageDefTagVariableValue,
        std::shared_ptr<MultiLangString> pageText,
        std::shared_ptr<MultiLangString> pageCaptionText,
        std::shared_ptr<Theme> theme,
        bool useCaption,
        bool useMPL,
        const std::string& moduleName,
        bool moduleExternal,
        std::shared_ptr<MultiLangString> traverseText,
        bool autoTraverse,
        bool autoReturn,
        std::shared_ptr<MultiLangString> returnText,
        const std::string& returnPageId,
        const std::string& moduleConsraintVariableName,
        bool autowire,
        std::shared_ptr<MultiLangString> autowireInText,
        std::shared_ptr<MultiLangString> autowireOutText,
        bool autoIn,
        bool needsAction,
        bool autoOut,
        const std::string& autowireInConstraint,
        const std::string& autowireOutConstraint,
        bool globalAutowire,
        bool noSave,
        bool autosFirst,
        std::shared_ptr<LinksTableModel> linksTableModel
    );

    void updateLink(
        std::shared_ptr<Link> link,
        const std::string& linkVariableName,
        const std::string& linkConstraintValue,
        std::shared_ptr<MultiLangString> linkText,
        std::shared_ptr<MultiLangString> linkAltText,
        bool autoFlag,
        bool once
    );

    void updateNode(std::shared_ptr<NodeItem> nodeToUpdate);
    void updateLink(std::shared_ptr<Link> linkToUpdate);

    void updateObj(
        std::shared_ptr<Obj> obj,
        const std::string& objVariableName,
        const std::string& objDefTagVariableValue,
        const std::string& objConstraintValue,
        const std::string& objCommonToName,
        const std::string& objName,
        const std::string& imageFileName,
        const std::string& soundFileName,
        bool soundSFX,
        bool animatedImage,
        bool suppressDsc,
        std::shared_ptr<MultiLangString> objDisp,
        std::shared_ptr<MultiLangString> objText,
        std::shared_ptr<MultiLangString> objActText,
        std::shared_ptr<MultiLangString> objNouseText,
        bool objIsGraphical,
        bool objIsShowOnCursor,
        bool objIsPreserved,
        bool objIsLoadOnce,
        bool objIsCollapsable,
        const std::string& offset,
        Obj::MovementDirection movementDirection,
        Obj::Effect effect,
        int startFrame,
        int maxFrame,
        int preloadFrames,
        int pauseFrames,
        Obj::CoordsOrigin coordsOrigin,
        bool objIsClearUnderTooltip,
        bool objIsActOnKey,
        bool objIsCacheText,
        bool objIsLooped,
        bool objIsNoRedrawOnAct,
        const std::string& objMorphOver,
        const std::string& objMorphOut,
        bool objIsTakable,
        bool objIsCallback,
        bool imageInScene,
        bool imageInInventory
    );

    void updateLinkCoords(std::shared_ptr<Link> link, float left, float top);
    void updateLinkCoords(std::shared_ptr<Link> link, float height);

    void addImageFile(const std::string& imageFile, const std::string& imageFileName = "");
    void addSoundFile(const std::string& soundFile, const std::string& soundFileName = "");
    void removeImageFile(const std::string& imageFileName);
    void removeSoundFile(const std::string& soundFileName);

    void resizeNode(std::shared_ptr<NodeItem> nodeItem, NodeItem::Orientation orientation, double deltaX, double deltaY);
    void updateNodeCoords(std::shared_ptr<NodeItem> nodeItem, const std::set<std::shared_ptr<NodeItem>>& additionallyMovedItems, float deltaX, float deltaY);
    void changeContainer(const std::string& previousContainerId, const std::string& newContainerId, const std::string& objId);
    void changeStartPoint(const std::string& startPoint);

    void cut(const std::vector<std::string>& pageIds, const std::vector<std::string>& objIds);
    void copy(const std::vector<std::string>& pageIds, const std::vector<std::string>& objIds);
    void paste();

    std::shared_ptr<Page> createPage(float left, float top);
    void addPage(std::shared_ptr<Page> page);
    std::shared_ptr<Obj> createObj(float left, float top);
    void addObj(std::shared_ptr<Obj> obj);
    std::shared_ptr<Link> createLink(std::shared_ptr<NodeItem> item, std::shared_ptr<NodeItem> target);
    void addLink(std::shared_ptr<Link> link);

    bool hasChanges();
    void save(bool create, std::shared_ptr<ProgressData> progressData);
    void saveAs(const std::string& nlbFolder, std::shared_ptr<ProgressData> progressData);
    void load(const std::string& path, std::shared_ptr<ProgressData> progressData);

    void deleteNode(std::shared_ptr<NodeItem> nodeToDelete);
    void deleteLink(std::shared_ptr<Link> link);
    void invalidateAssociatedLinks(std::shared_ptr<NodeItem> nodeItem);
    void updateAllViews();

    bool canUndo();
    void undo();
    bool canUndo(const std::string& id);
    void undo(const std::string& id);
    bool canRedo();
    void redo();
    bool canRedo(const std::string& id);
    void redo(const std::string& id);
    void redoAll(const std::string& id);

    // NLBObservable interface implementation
    virtual std::string addObserver(std::shared_ptr<NLBObserver> observer) override;
    virtual void removeObserver(const std::string& observerId) override;
    virtual void notifyObservers() override;
};