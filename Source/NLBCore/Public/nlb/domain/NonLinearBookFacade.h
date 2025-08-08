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

#include "nlb/util/FileUtils.h"
#include "nlb/api/PartialProgressData.h"
#include "nlb/exception/NLBExceptions.h"

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
class NonLinearBookFacade : public NLBObservable {
private:
    NonLinearBookImpl* m_nlb;
    std::map<std::string, PageImpl*> m_newPagesPool;
    std::map<std::string, ObjImpl*> m_newObjsPool;
    std::map<std::string, LinkImpl*> m_newLinksPool;
    
    /*!
     * @brief Main undo manager
     */
    UndoManager* m_undoManager;
    
    /*!
     * @brief Items' own undo managers
     */
    std::map<std::string, UndoManager*> m_undoManagersMap;
    
    ObserverHandler* m_observerHandler;
    VCSAdapter* m_vcsAdapter;
    Author* m_author;
    bool m_rootFacade;
    NonLinearBookFacade* m_parentFacade;
    std::vector<NonLinearBookFacade*> m_moduleFacades;

    UndoManager* getUndoManagerByItemId(const std::string& id);
    void clearUndos();
    void clearPools();
    void exportMedia(const std::string& exportDir);
    void exportAdditionalMedia(const std::string& exportDir);
    void updateNodeCoords(CommandChainCommand* commandChain, NodeItem* nodeItem, float deltaX, float deltaY);
    void offsetContainedObjects(CommandChainCommand* commandChain, NodeItem* container, float deltaX, float deltaY);
    void saveNLB(bool create, ProgressData* progressData);

public:
    NonLinearBookFacade(Author* author, VCSAdapter* vcsAdapter);
    NonLinearBookFacade(NonLinearBookFacade* parentFacade, Author* author, VCSAdapter* vcsAdapter, NonLinearBookImpl* nlb);
    virtual ~NonLinearBookFacade() = default;

    void createNewBook();
    NonLinearBookFacade* createModuleFacade(const std::string& modulePageId);
    NonLinearBookFacade* getMainFacade();
    NonLinearBookFacade* getParentFacade() { return m_parentFacade; }
    NonLinearBook* getNlb() { return (NonLinearBook*) (m_nlb); }

    void clear();
    void clearUndosAndPools();
    void commit(const std::string& commitMessageText);
    void pull(const std::string& userName, const std::string& password, ProgressData* progressData);
    void push(const std::string& userName, const std::string& password, ProgressData* progressData);

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

    void updateModifications(ModifyingItem* modifyingItem, ModificationsTableModel* modificationsTableModel);
    void setMediaFileConstrId(MediaFile::Type mediaType, const std::string& fileName, const std::string& constrId);
    void setMediaFileRedirect(MediaFile::Type mediaType, const std::string& fileName, const std::string& redirect);
    void setMediaFileFlag(MediaFile::Type mediaType, const std::string& fileName, bool flag);
    void setMediaFileExportParametersPreset(MediaFile::Type mediaType, const std::string& fileName, MediaExportParameters::Preset preset);

    void updateBookProperties(
        const std::string& license,
        Theme theme,
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
        Page* page,
        const std::string& imageFileName,
        bool imageBackground,
        bool imageAnimated,
        const std::string& soundFileName,
        bool soundSFX,
        const std::string& pageVariableName,
        const std::string& pageTimerVariableName,
        const std::string& pageDefTagVariableValue,
        MultiLangString* pageText,
        MultiLangString* pageCaptionText,
        Theme theme,
        bool useCaption,
        bool useMPL,
        const std::string& moduleName,
        bool moduleExternal,
        MultiLangString* traverseText,
        bool autoTraverse,
        bool autoReturn,
        MultiLangString* returnText,
        const std::string& returnPageId,
        const std::string& moduleConsraintVariableName,
        bool autowire,
        MultiLangString* autowireInText,
        MultiLangString* autowireOutText,
        bool autoIn,
        bool needsAction,
        bool autoOut,
        const std::string& autowireInConstraint,
        const std::string& autowireOutConstraint,
        bool globalAutowire,
        bool noSave,
        bool autosFirst,
        LinksTableModel* linksTableModel
    );

    void updateLink(
        Link* link,
        const std::string& linkVariableName,
        const std::string& linkConstraintValue,
        MultiLangString* linkText,
        MultiLangString* linkAltText,
        bool autoFlag,
        bool once
    );

    void updateNode(NodeItem* nodeToUpdate);
    void updateLink(Link* linkToUpdate);

    void updateObj(
        Obj* obj,
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
        MultiLangString* objDisp,
        MultiLangString* objText,
        MultiLangString* objActText,
        MultiLangString* objNouseText,
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

    void updateLinkCoords(Link* link, float left, float top);
    void updateLinkCoords(Link* link, float height);

    void addImageFile(const std::string& imageFile, const std::string& imageFileName = "");
    void addSoundFile(const std::string& soundFile, const std::string& soundFileName = "");
    void removeImageFile(const std::string& imageFileName);
    void removeSoundFile(const std::string& soundFileName);

    void resizeNode(NodeItem* nodeItem, NodeItem::Orientation orientation, double deltaX, double deltaY);
    void updateNodeCoords(NodeItem* nodeItem, const std::set<NodeItem*>& additionallyMovedItems, float deltaX, float deltaY);
    void changeContainer(const std::string& previousContainerId, const std::string& newContainerId, const std::string& objId);
    void changeStartPoint(const std::string& startPoint);

    void cut(const std::vector<std::string>& pageIds, const std::vector<std::string>& objIds);
    void copy(const std::vector<std::string>& pageIds, const std::vector<std::string>& objIds);
    void paste();

    Page* createPage(float left, float top);
    void addPage(Page* page);
    Obj* createObj(float left, float top);
    void addObj(Obj* obj);
    Link* createLink(NodeItem* item, NodeItem* target);
    void addLink(Link* link);

    bool hasChanges();
    void save(bool create, ProgressData* progressData);
    void saveAs(const std::string& nlbFolder, ProgressData* progressData);
    void load(const std::string& path, ProgressData* progressData);

    void deleteNode(NodeItem* nodeToDelete);
    void deleteLink(Link* link);
    void invalidateAssociatedLinks(NodeItem* nodeItem);
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
    virtual std::string addObserver(NLBObserver* observer) override;
    virtual void removeObserver(const std::string& observerId) override;
    virtual void notifyObservers() override;
};
