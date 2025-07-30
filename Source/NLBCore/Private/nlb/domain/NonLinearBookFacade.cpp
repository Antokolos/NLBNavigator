#include "nlb/domain/NonLinearBookFacade.h"

#include "nlb/api/NLBObserver.h"
#include "nlb/api/ModificationsTableModel.h"
#include "nlb/api/LinksTableModel.h"
#include "nlb/api/NodeItem.h"
#include "nlb/api/Constants.h"
#include "nlb/api/ProgressData.h"
#include "nlb/api/NonLinearBook.h"
#include "nlb/api/ModifyingItem.h"
#include "nlb/domain/NonLinearBookImpl.h"
#include "nlb/domain/ObserverHandler.h"
#include "nlb/domain/Clipboard.h"
#include "nlb/domain/PageImpl.h"
#include "nlb/domain/ObjImpl.h"
#include "nlb/domain/LinkImpl.h"
#include "nlb/domain/UndoManager.h"
#include "nlb/domain/CommandChainCommand.h"
#include "nlb/domain/MediaExportParameters.h"
#include "nlb/vcs/Author.h"
#include "nlb/vcs/VCSAdapter.h"

#include "nlb/util/FileManipulator.h"
#include "nlb/util/ResourceManager.h"
// #include "nlb/domain/Clipboard.h"
// #include "nlb/domain/UpdateLinkCoordsCommand.h"
// #include "nlb/domain/UpdateNodeCoordsCommand.h"
// #include "nlb/domain/ChangeContainerCommand.h"
#include "nlb/api/PartialProgressData.h"
#include "nlb/api/Page.h"
#include "nlb/api/Theme.h"
#include "nlb/util/StringHelper.h"
#include "nlb/api/DummyNLB.h"
#include "nlb/api/RootModulePage.h"
#include <cmath>

#include "nlb/domain/ChangeContainerCommand.h"
#include "nlb/domain/UpdateLinkCoordsCommand.h"
#include "nlb/domain/UpdateNodeCoordsCommand.h"
#include "nlb/util/UUID.h"

NonLinearBookFacade::NonLinearBookFacade(std::shared_ptr<Author> author, std::shared_ptr<VCSAdapter> vcsAdapter) 
    : m_rootFacade(true)
    , m_parentFacade(nullptr)
    , m_author(author)
    , m_vcsAdapter(vcsAdapter)
    , m_undoManager(std::make_shared<UndoManager>())
    , m_observerHandler(std::make_shared<ObserverHandler>())
{
    // m_nlb should be initialized later
}

NonLinearBookFacade::NonLinearBookFacade(
    std::shared_ptr<NonLinearBookFacade> parentFacade,
    std::shared_ptr<Author> author,
    std::shared_ptr<VCSAdapter> vcsAdapter,
    std::shared_ptr<NonLinearBookImpl> nlb
) : m_rootFacade(false)
  , m_parentFacade(parentFacade)
  , m_author(author)
  , m_vcsAdapter(vcsAdapter)
  , m_nlb(nlb)
  , m_undoManager(std::make_shared<UndoManager>())
  , m_observerHandler(std::make_shared<ObserverHandler>())
{
}

void NonLinearBookFacade::createNewBook() {
    auto parentNLB = DummyNLB::singleton();
    m_nlb = std::make_shared<NonLinearBookImpl>(parentNLB, std::make_shared<RootModulePage>(parentNLB, nlb::Constants::MAIN_MODULE_NAME));
    notifyObservers();
}

std::shared_ptr<NonLinearBookFacade> NonLinearBookFacade::createModuleFacade(const std::string& modulePageId) {
    auto page = m_nlb->getPageImplById(modulePageId);
    auto facade = std::make_shared<NonLinearBookFacade>(shared_from_this(), m_author, m_vcsAdapter, page->getModuleImpl());
    m_moduleFacades.push_back(facade);
    notifyObservers();
    return facade;
}

std::shared_ptr<NonLinearBookFacade> NonLinearBookFacade::getMainFacade() {
    auto result = shared_from_this();
    while (result->getParentFacade() != nullptr) {
        result = result->getParentFacade();
    }
    return result;
}

void NonLinearBookFacade::clear() {
    m_nlb->clear();
    if (m_rootFacade) {
        m_vcsAdapter->closeAdapter();
    }
    clearUndosAndPools();
}

void NonLinearBookFacade::clearUndosAndPools() {
    clearUndos();
    clearPools();
    for (auto facade : m_moduleFacades) {
        facade->clearUndosAndPools();
    }
    notifyObservers();
}

void NonLinearBookFacade::clearUndos() {
    m_undoManager->clear();
    for (auto& entry : m_undoManagersMap) {
        entry.second->clear();
    }
}

void NonLinearBookFacade::commit(const std::string& commitMessageText) {
    m_vcsAdapter->commit(commitMessageText);
    notifyObservers();
}

void NonLinearBookFacade::pull(const std::string& userName, const std::string& password, std::shared_ptr<ProgressData> progressData) {
    m_vcsAdapter->pull(userName, password, progressData);
    notifyObservers();
}

void NonLinearBookFacade::push(const std::string& userName, const std::string& password, std::shared_ptr<ProgressData> progressData) {
    m_vcsAdapter->push(userName, password, progressData);
    notifyObservers();
}

void NonLinearBookFacade::exportToChoiceScript(const std::string& exportDir) {
    std::string startupFile = exportDir + "/startup.txt";
    m_nlb->exportToChoiceScript(startupFile);
    std::string imagesExportDir = exportDir + "/" + NonLinearBook::IMAGES_DIR_NAME;
    m_nlb->exportImages(true, imagesExportDir);
}

void NonLinearBookFacade::exportToQSPTextFile(const std::string& exportDir) {
    std::string bookFile = exportDir + "/book.txt";
    m_nlb->exportToQSPTextFile(bookFile);
    std::string imagesExportDir = exportDir + "/" + NonLinearBook::IMAGES_DIR_NAME;
    m_nlb->exportImages(true, imagesExportDir);
}

void NonLinearBookFacade::exportToURQTextFile(const std::string& exportDir) {
    std::string bookFile = exportDir + "/book.qst";
    m_nlb->exportToURQTextFile(bookFile);
}

void NonLinearBookFacade::exportToPDFFile(const std::string& exportFile) {
    m_nlb->exportToPDFFile(exportFile);
}

void NonLinearBookFacade::exportToTXTFile(const std::string& exportDir) {
    std::string bookFile = exportDir + "/book.txt";
    m_nlb->exportToTXTFile(bookFile);
}

void NonLinearBookFacade::exportToHTMLFile(const std::string& exportDir) {
    std::string htmlFile = exportDir + "/index.html";
    m_nlb->exportToHTMLFile(htmlFile);
    std::string imagesExportDir = exportDir + "/" + NonLinearBook::IMAGES_DIR_NAME;
    m_nlb->exportImages(true, imagesExportDir);
}

void NonLinearBookFacade::exportToJSIQFile(const std::string& exportDir) {
    std::string xmlFile = exportDir + "/example.xml";
    m_nlb->exportToJSIQFile(xmlFile);
}

void NonLinearBookFacade::exportToSTEADFile(const std::string& exportDir) {
    std::string luaFile = exportDir + "/main.lua";
    m_nlb->exportToSTEADFile(luaFile);
    exportMedia(exportDir);
    exportAdditionalMedia(exportDir);
}

void NonLinearBookFacade::exportToVNSTEADFile(const std::string& exportDir) {
    std::string luaFile = exportDir + "/main.lua";
    m_nlb->exportToVNSTEADFile(luaFile);
    exportMedia(exportDir);
    exportAdditionalMedia(exportDir);
}

void NonLinearBookFacade::exportMedia(const std::string& exportDir) {
    std::string imagesExportDir = exportDir + "/" + NonLinearBook::IMAGES_DIR_NAME;
    m_nlb->exportImages(true, imagesExportDir);
    std::string soundExportDir = exportDir + "/" + NonLinearBook::SOUND_DIR_NAME;
    m_nlb->exportSound(true, soundExportDir);
}

void NonLinearBookFacade::exportAdditionalMedia(const std::string& exportDir) {
    ResourceManager::exportBundledFiles(exportDir);
}

void NonLinearBookFacade::exportToASMFile(const std::string& exportDir) {
    std::string asmFile = exportDir + "/book.sm";
    m_nlb->exportToASMFile(asmFile);
}

void NonLinearBookFacade::updateModifications(
    std::shared_ptr<ModifyingItem> modifyingItem,
    std::shared_ptr<ModificationsTableModel> modificationsTableModel
) {
    auto command = m_nlb->createUpdateModificationsCommand(modifyingItem, modificationsTableModel);
    getUndoManagerByItemId(modifyingItem->getId() + nlb::Constants::MODIFICATIONS_UNDO_ID_POSTFIX)->executeAndStore(command);
    notifyObservers();
}

void NonLinearBookFacade::setMediaFileConstrId(
    MediaFile::Type mediaType,
    const std::string& fileName,
    const std::string& constrId
) {
    m_nlb->setMediaFileConstrId(mediaType, fileName, constrId);
}

void NonLinearBookFacade::setMediaFileRedirect(
    MediaFile::Type mediaType,
    const std::string& fileName,
    const std::string& redirect
) {
    m_nlb->setMediaFileRedirect(mediaType, fileName, redirect);
}

void NonLinearBookFacade::setMediaFileFlag(
    MediaFile::Type mediaType,
    const std::string& fileName,
    bool flag
) {
    m_nlb->setMediaFileFlag(mediaType, fileName, flag);
}

void NonLinearBookFacade::setMediaFileExportParametersPreset(
    MediaFile::Type mediaType,
    const std::string& fileName,
    MediaExportParameters::Preset preset
) {
    m_nlb->setMediaFileExportParametersPreset(mediaType, fileName, preset);
}

void NonLinearBookFacade::updateBookProperties(
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
) {
    auto command = m_nlb->createUpdateBookPropertiesCommand(
        license, theme, language, title, author, version,
        perfectGameAchievementName, fullAutowire, suppressMedia,
        suppressSound, propagateToSubmodules
    );
    m_undoManager->executeAndStore(command);
    notifyObservers();
}

void NonLinearBookFacade::updatePage(
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
) {
    auto command = m_nlb->createUpdatePageCommand(
        page, imageFileName, imageBackground, imageAnimated,
        soundFileName, soundSFX, pageVariableName, pageTimerVariableName,
        pageDefTagVariableValue, pageText, pageCaptionText, theme,
        useCaption, useMPL, moduleName, moduleExternal, traverseText,
        autoTraverse, autoReturn, returnText, returnPageId,
        moduleConsraintVariableName, autowire, autowireInText,
        autowireOutText, autoIn, needsAction, autoOut,
        autowireInConstraint, autowireOutConstraint, globalAutowire,
        noSave, autosFirst, linksTableModel
    );
    getUndoManagerByItemId(page->getId())->executeAndStore(command);
    notifyObservers();
}

void NonLinearBookFacade::updateLink(
    std::shared_ptr<Link> link,
    const std::string& linkVariableName,
    const std::string& linkConstraintValue,
    std::shared_ptr<MultiLangString> linkText,
    std::shared_ptr<MultiLangString> linkAltText,
    bool autoFlag,
    bool once
) {
    /* TODO: Compilation error
    auto command = m_nlb->createUpdateLinkCommand(
        link, linkVariableName, linkConstraintValue,
        linkText, linkAltText, autoFlag, once
    );
    getUndoManagerByItemId(link->getId())->executeAndStore(command);
    */
    notifyObservers();
}

void NonLinearBookFacade::updateNode(std::shared_ptr<NodeItem> nodeToUpdate) {
    nodeToUpdate->notifyObservers();
    auto adjacentLinks = m_nlb->getAssociatedLinks(nodeToUpdate);
    for (auto& link : adjacentLinks) {
        link->notifyObservers();
    }
}

void NonLinearBookFacade::updateLink(std::shared_ptr<Link> linkToUpdate) {
    linkToUpdate->notifyObservers();
}

void NonLinearBookFacade::updateObj(
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
) {
    /* TODO: compilation error
    auto command = m_nlb->createUpdateObjCommand(
        obj, objVariableName, objDefTagVariableValue, objConstraintValue,
        objCommonToName, objName, imageFileName, soundFileName, soundSFX,
        animatedImage, suppressDsc, objDisp, objText, objActText,
        objNouseText, objIsGraphical, objIsShowOnCursor, objIsPreserved,
        objIsLoadOnce, objIsCollapsable, offset, movementDirection,
        effect, startFrame, maxFrame, preloadFrames, pauseFrames,
        coordsOrigin, objIsClearUnderTooltip, objIsActOnKey, objIsCacheText,
        objIsLooped, objIsNoRedrawOnAct, objMorphOver, objMorphOut,
        objIsTakable, objIsCallback, imageInScene, imageInInventory
    );
    getUndoManagerByItemId(obj->getId())->executeAndStore(command);
    */
    notifyObservers();
}

void NonLinearBookFacade::updateLinkCoords(
    std::shared_ptr<Link> link,
    float left,
    float top
) {
    auto command = std::make_shared<UpdateLinkCoordsCommand>(m_nlb, link, left, top);
    m_undoManager->executeAndStore(command);
    notifyObservers();
}

void NonLinearBookFacade::updateLinkCoords(
    std::shared_ptr<Link> link,
    float height
) {
    auto command = std::make_shared<UpdateLinkCoordsCommand>(m_nlb, link, height);
    m_undoManager->executeAndStore(command);
    notifyObservers();
}

void NonLinearBookFacade::addImageFile(
    const std::string& imageFile,
    const std::string& imageFileName
) {
    auto rootDir = m_nlb->getRootDir();
    if (!rootDir.empty()) {
        FileManipulator fileManipulator(m_vcsAdapter, rootDir);
        m_nlb->copyAndAddImageFile(fileManipulator, imageFile, imageFileName);
    } else {
        throw NLBConsistencyException("NLB root dir is undefined");
    }
}

void NonLinearBookFacade::addSoundFile(
    const std::string& soundFile,
    const std::string& soundFileName
) {
    auto rootDir = m_nlb->getRootDir();
    if (!rootDir.empty()) {
        FileManipulator fileManipulator(m_vcsAdapter, rootDir);
        m_nlb->copyAndAddSoundFile(fileManipulator, soundFile, soundFileName);
    } else {
        throw NLBConsistencyException("NLB root dir is undefined");
    }
}

void NonLinearBookFacade::removeImageFile(const std::string& imageFileName) {
    auto rootDir = m_nlb->getRootDir();
    if (!rootDir.empty()) {
        FileManipulator fileManipulator(m_vcsAdapter, rootDir);
        m_nlb->removeImageFile(fileManipulator, imageFileName);
    } else {
        throw NLBConsistencyException("NLB root dir is undefined");
    }
}

void NonLinearBookFacade::removeSoundFile(const std::string& soundFileName) {
    auto rootDir = m_nlb->getRootDir();
    if (!rootDir.empty()) {
        FileManipulator fileManipulator(m_vcsAdapter, rootDir);
        m_nlb->removeSoundFile(fileManipulator, soundFileName);
    } else {
        throw NLBConsistencyException("NLB root dir is undefined");
    }
}

void NonLinearBookFacade::resizeNode(
    std::shared_ptr<NodeItem> nodeItem,
    NodeItem::Orientation orientation,
    double deltaX,
    double deltaY
) {
    std::shared_ptr<AbstractNodeItem> node = m_nlb->getPageImplById(nodeItem->getId());
    if (!node) {
        node = m_nlb->getObjImplById(nodeItem->getId());
    }
    auto adjacentLinks = m_nlb->getAssociatedLinks(nodeItem);
    auto command = node->createResizeNodeCommand(orientation, deltaX, deltaY, adjacentLinks);
    m_undoManager->executeAndStore(command);
    notifyObservers();
}

void NonLinearBookFacade::updateNodeCoords(
    std::shared_ptr<NodeItem> nodeItem,
    const std::set<std::shared_ptr<NodeItem>>& additionallyMovedItems,
    float deltaX,
    float deltaY
) {
    auto commandChain = std::make_shared<CommandChainCommand>();
    updateNodeCoords(commandChain, nodeItem, deltaX, deltaY);
    for (const auto& additionalNodeItem : additionallyMovedItems) {
        updateNodeCoords(commandChain, additionalNodeItem, deltaX, deltaY);
    }
    m_undoManager->executeAndStore(commandChain);
    notifyObservers();
}

void NonLinearBookFacade::updateNodeCoords(
    std::shared_ptr<CommandChainCommand> commandChain,
    std::shared_ptr<NodeItem> nodeItem,
    float deltaX,
    float deltaY
) {
    auto command = std::make_shared<UpdateNodeCoordsCommand>(m_nlb, nodeItem, deltaX, deltaY);
    commandChain->addCommand(command);
    offsetContainedObjects(commandChain, nodeItem, deltaX, deltaY);
}

void NonLinearBookFacade::offsetContainedObjects(
    std::shared_ptr<CommandChainCommand> commandChain,
    std::shared_ptr<NodeItem> container,
    float deltaX,
    float deltaY
) {
    for (const auto& nodeId : container->getContainedObjIds()) {
        auto node = m_nlb->getObjById(nodeId);
        auto nodeCoords = node->getCoords();
        updateNodeCoords(commandChain, node, deltaX, deltaY);
        offsetContainedObjects(commandChain, node, deltaX, deltaY);
    }
}

void NonLinearBookFacade::changeContainer(
    const std::string& previousContainerId,
    const std::string& newContainerId,
    const std::string& objId
) {
    std::shared_ptr<AbstractNodeItem> prevContainer;
    if (!StringHelper::isEmpty(previousContainerId)) {
        prevContainer = m_nlb->getPageImplById(previousContainerId);
        if (!prevContainer) {
            prevContainer = m_nlb->getObjImplById(previousContainerId);
        }
    }

    std::shared_ptr<AbstractNodeItem> newContainer;
    if (!StringHelper::isEmpty(newContainerId)) {
        newContainer = m_nlb->getPageImplById(newContainerId);
        if (!newContainer) {
            newContainer = m_nlb->getObjImplById(newContainerId);
        }
    }

    if ((prevContainer == nullptr && newContainer != nullptr) ||
        (prevContainer != nullptr && newContainer == nullptr) ||
        (prevContainer != nullptr && prevContainer->getId() != newContainer->getId())) {
        auto obj = m_nlb->getObjImplById(objId);
        /* TODO:   LNK2019: unresolved external symbol "public: __cdecl ChangeContainerCommand::ChangeContainerCommand(class std::shared_ptr<class AbstractNodeItem>,class std::shared_ptr<class AbstractNodeItem>,class std::shared_ptr<class ObjImpl>)" (??0ChangeContainerCommand@@QEAA@V?$shared_ptr@VAbstractNodeItem@@@std@@0V?$shared_ptr@VObjImpl@@@2@@Z) referenced in function "public: void __cdecl NonLinearBookFacade::changeContainer(class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > const &,class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > const &,class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > const &)" (?changeContainer@NonLinearBookFacade@@QEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@00@Z)
        auto command = std::make_shared<ChangeContainerCommand>(prevContainer, newContainer, obj);
        m_undoManager->executeAndStore(command);
        */
        notifyObservers();
    }
}

void NonLinearBookFacade::changeStartPoint(const std::string& startPoint) {
    auto command = m_nlb->createChangeStartPointCommand(startPoint);
    m_undoManager->executeAndStore(command);
    notifyObservers();
}

void NonLinearBookFacade::cut(
    const std::vector<std::string>& pageIds,
    const std::vector<std::string>& objIds
) {
    auto command = std::make_shared<CommandChainCommand>();
    command->addCommand(m_nlb->createCopyCommand(pageIds, objIds));
    command->addCommand(m_nlb->createDeleteCommand(pageIds, objIds));
    m_undoManager->executeAndStore(command);
    notifyObservers();
}

void NonLinearBookFacade::copy(
    const std::vector<std::string>& pageIds,
    const std::vector<std::string>& objIds
) {
    auto command = m_nlb->createCopyCommand(pageIds, objIds);
    m_undoManager->executeAndStore(command);
    notifyObservers();
}

void NonLinearBookFacade::paste() {
    auto nlbToPaste = Clipboard::singleton().getNonLinearBook();
    if (nlbToPaste) {
        auto command = m_nlb->createPasteCommand(nlbToPaste);
        m_undoManager->executeAndStore(command);
        notifyObservers();
    }
}

void NonLinearBookFacade::clearPools()
{
    m_newPagesPool.clear();
    m_newObjsPool.clear();
    m_newLinksPool.clear();
}

std::string NonLinearBookFacade::addObserver(std::shared_ptr<NLBObserver> observer) {
    return m_observerHandler ? m_observerHandler->addObserver(observer) : nlb::Constants::EMPTY_STRING;
}

void NonLinearBookFacade::removeObserver(const std::string& observerId) {
    if (m_observerHandler) {
        m_observerHandler->removeObserver(observerId);
    }
}

void NonLinearBookFacade::notifyObservers() {
    if (m_observerHandler) {
        m_observerHandler->notifyObservers();
    }
}

std::shared_ptr<UndoManager> NonLinearBookFacade::getUndoManagerByItemId(const std::string& id) {
    auto it = m_undoManagersMap.find(id);
    if (it != m_undoManagersMap.end()) {
        return it->second;
    }
    
    std::shared_ptr<UndoManager> result = std::make_shared<UndoManager>();
    m_undoManagersMap[id] = result;
    return result;
}

std::shared_ptr<Page> NonLinearBookFacade::createPage(float left, float top) {
    std::string id = NLBUUID::randomUUID();
    auto result = std::make_shared<PageImpl>(m_nlb, left, top);
    result->setId(id);
    m_newPagesPool[id] = result;
    return result;
}

void NonLinearBookFacade::addPage(std::shared_ptr<Page> page) {
    auto pageId = page->getId();
    auto pageImpl = m_newPagesPool[pageId];
    
    if (pageImpl) {
        m_newPagesPool.erase(pageId);
        auto command = m_nlb->createAddPageCommand(pageImpl);
        m_undoManager->executeAndStore(command);
        notifyObservers();
    }
}

std::shared_ptr<Obj> NonLinearBookFacade::createObj(float left, float top) {
    std::string id = NLBUUID::randomUUID();
    auto result = std::make_shared<ObjImpl>(m_nlb, left, top);
    result->setId(id);
    m_newObjsPool[id] = result;
    return result;
}

void NonLinearBookFacade::addObj(std::shared_ptr<Obj> obj) {
    auto objId = obj->getId();
    auto objImpl = m_newObjsPool[objId];
    
    if (objImpl) {
        m_newObjsPool.erase(objId);
        auto command = m_nlb->createAddObjCommand(objImpl);
        m_undoManager->executeAndStore(command);
        notifyObservers();
    }
}

std::shared_ptr<Link> NonLinearBookFacade::createLink(std::shared_ptr<NodeItem> item, std::shared_ptr<NodeItem> target) {
    std::string id = NLBUUID::randomUUID();
    auto result = std::make_shared<LinkImpl>(item, target->getId());
    result->setId(id);
    m_newLinksPool[id] = result;
    return result;
}

void NonLinearBookFacade::addLink(std::shared_ptr<Link> link) {
    auto linkId = link->getId();
    auto linkImpl = m_newLinksPool[linkId];
    
    if (linkImpl) {
        m_newLinksPool.erase(linkId);
        auto parent = link->getParent();
        std::shared_ptr<AbstractNodeItem> nodeItem = m_nlb->getPageImplById(parent->getId());
        if (!nodeItem) {
            nodeItem = m_nlb->getObjImplById(parent->getId());
        }
        
        nodeItem->addLink(linkImpl);
        notifyObservers();
    }
}

bool NonLinearBookFacade::hasChanges() {
    // TODO: Implementation needed
    return false;
}

void NonLinearBookFacade::saveNLB(bool create, std::shared_ptr<ProgressData> progressData) {
    // Implementation for saving NLB
    // TODO: Implementation needed
}

void NonLinearBookFacade::save(bool create, std::shared_ptr<ProgressData> progressData) {
    saveNLB(create, progressData);
    notifyObservers();
}

void NonLinearBookFacade::saveAs(const std::string& nlbFolder, std::shared_ptr<ProgressData> progressData) {
    // TODO: Implementation needed
    notifyObservers();
}

void NonLinearBookFacade::load(const std::string& path, std::shared_ptr<ProgressData> progressData) {
    clear();
    // TODO: Implementation needed
    m_nlb->load(path, *progressData);
    notifyObservers();
}

void NonLinearBookFacade::deleteNode(std::shared_ptr<NodeItem> nodeToDelete) {
    // TODO: Compilation error fix
    // auto command = m_nlb->createDeleteNodeCommand(nodeToDelete);
    // m_undoManager->executeAndStore(command);
    notifyObservers();
}

void NonLinearBookFacade::deleteLink(std::shared_ptr<Link> link) {
    auto parent = link->getParent();
    std::shared_ptr<AbstractNodeItem> nodeItem = m_nlb->getPageImplById(parent->getId());
    if (!nodeItem) {
        nodeItem = m_nlb->getObjImplById(parent->getId());
    }
    
    // auto command = nodeItem->createDeleteLinkCommand(link);
    // m_undoManager->executeAndStore(command);
    notifyObservers();
}

void NonLinearBookFacade::invalidateAssociatedLinks(std::shared_ptr<NodeItem> nodeItem) {
    auto associatedLinks = m_nlb->getAssociatedLinks(nodeItem);
    for (const auto& link : associatedLinks) {
        link->notifyObservers();
    }
    notifyObservers();
}

void NonLinearBookFacade::updateAllViews() {
    notifyObservers();
    for (auto facade : m_moduleFacades) {
        facade->updateAllViews();
    }
}

bool NonLinearBookFacade::canUndo() {
    return m_undoManager->canUndo();
}

void NonLinearBookFacade::undo() {
    if (canUndo()) {
        m_undoManager->undo();
        notifyObservers();
    }
}

bool NonLinearBookFacade::canUndo(const std::string& id) {
    auto undoManager = getUndoManagerByItemId(id);
    return undoManager->canUndo();
}

void NonLinearBookFacade::undo(const std::string& id) {
    auto undoManager = getUndoManagerByItemId(id);
    if (undoManager->canUndo()) {
        undoManager->undo();
        notifyObservers();
    }
}

bool NonLinearBookFacade::canRedo() {
    return m_undoManager->canRedo();
}

void NonLinearBookFacade::redo() {
    if (canRedo()) {
        m_undoManager->redo();
        notifyObservers();
    }
}

bool NonLinearBookFacade::canRedo(const std::string& id) {
    auto undoManager = getUndoManagerByItemId(id);
    return undoManager->canRedo();
}

void NonLinearBookFacade::redo(const std::string& id) {
    auto undoManager = getUndoManagerByItemId(id);
    if (undoManager->canRedo()) {
        undoManager->redo();
        notifyObservers();
    }
}

void NonLinearBookFacade::redoAll(const std::string& id) {
    auto undoManager = getUndoManagerByItemId(id);
    undoManager->redoAll();
    notifyObservers();
}