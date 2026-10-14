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

NonLinearBookFacade::NonLinearBookFacade(Author* author, VCSAdapter* vcsAdapter)
    : m_rootFacade(true)
    , m_parentFacade(nullptr)
    , m_author(author)
    , m_vcsAdapter(vcsAdapter)
    , m_undoManager(new UndoManager())
    , m_observerHandler(new ObserverHandler())
{
    // m_nlb will be initialized later via createNewBook() or load()
}

NonLinearBookFacade::NonLinearBookFacade(
    NonLinearBookFacade* parentFacade,
    Author* author,
    VCSAdapter* vcsAdapter,
    NonLinearBookImpl* nlb
) : m_rootFacade(false)
  , m_parentFacade(parentFacade)
  , m_author(author)
  , m_vcsAdapter(vcsAdapter)
  , m_nlb(nlb)
  , m_undoManager(new UndoManager())
  , m_observerHandler(new ObserverHandler())
{
}

void NonLinearBookFacade::createNewBook() {
    auto parentNLB = DummyNLB::singleton();
    m_nlb = new NonLinearBookImpl(parentNLB, new RootModulePage(parentNLB, nlb::Constants::MAIN_MODULE_NAME));
    notifyObservers();
}

NonLinearBookFacade* NonLinearBookFacade::createModuleFacade(const std::string& modulePageId) {
    auto page = m_nlb->getPageImplById(modulePageId);
    auto facade = new NonLinearBookFacade(this, m_author, m_vcsAdapter, page->getModuleImpl());
    m_moduleFacades.push_back(facade);
    notifyObservers();
    return facade;
}

NonLinearBookFacade* NonLinearBookFacade::getMainFacade() {
    auto result = this;
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

void NonLinearBookFacade::pull(const std::string& userName, const std::string& password, ProgressData* progressData) {
    m_vcsAdapter->pull(userName, password, progressData);
    notifyObservers();
}

void NonLinearBookFacade::push(const std::string& userName, const std::string& password, ProgressData* progressData) {
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
    ModifyingItem* modifyingItem,
    ModificationsTableModel* modificationsTableModel
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
    Link* link,
    const std::string& linkVariableName,
    const std::string& linkConstraintValue,
    MultiLangString* linkText,
    MultiLangString* linkAltText,
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

void NonLinearBookFacade::updateNode(NodeItem* nodeToUpdate) {
    nodeToUpdate->notifyObservers();
    auto adjacentLinks = m_nlb->getAssociatedLinks(nodeToUpdate);
    for (auto& link : adjacentLinks) {
        link->notifyObservers();
    }
}

void NonLinearBookFacade::updateLink(Link* linkToUpdate) {
    linkToUpdate->notifyObservers();
}

void NonLinearBookFacade::updateObj(
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
    Link* link,
    float left,
    float top
) {
    auto command = new UpdateLinkCoordsCommand(m_nlb, link, left, top);
    m_undoManager->executeAndStore(command);
    notifyObservers();
}

void NonLinearBookFacade::updateLinkCoords(
    Link* link,
    float height
) {
    auto command = new UpdateLinkCoordsCommand(m_nlb, link, height);
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
    NodeItem* nodeItem,
    NodeItem::Orientation orientation,
    double deltaX,
    double deltaY
) {
    AbstractNodeItem* node = m_nlb->getPageImplById(nodeItem->getId());
    if (!node) {
        node = m_nlb->getObjImplById(nodeItem->getId());
    }
    auto adjacentLinks = m_nlb->getAssociatedLinks(nodeItem);
    auto command = node->createResizeNodeCommand(orientation, deltaX, deltaY, adjacentLinks);
    m_undoManager->executeAndStore(command);
    notifyObservers();
}

void NonLinearBookFacade::updateNodeCoords(
    NodeItem* nodeItem,
    const std::set<NodeItem*>& additionallyMovedItems,
    float deltaX,
    float deltaY
) {
    auto commandChain = new CommandChainCommand();
    updateNodeCoords(commandChain, nodeItem, deltaX, deltaY);
    for (const auto& additionalNodeItem : additionallyMovedItems) {
        updateNodeCoords(commandChain, additionalNodeItem, deltaX, deltaY);
    }
    m_undoManager->executeAndStore(commandChain);
    notifyObservers();
}

void NonLinearBookFacade::updateNodeCoords(
    CommandChainCommand* commandChain,
    NodeItem* nodeItem,
    float deltaX,
    float deltaY
) {
    auto command = new UpdateNodeCoordsCommand(m_nlb, nodeItem, deltaX, deltaY);
    commandChain->addCommand(command);
    offsetContainedObjects(commandChain, nodeItem, deltaX, deltaY);
}

void NonLinearBookFacade::offsetContainedObjects(
    CommandChainCommand* commandChain,
    NodeItem* container,
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
    AbstractNodeItem* prevContainer;
    if (!StringHelper::isEmpty(previousContainerId)) {
        prevContainer = m_nlb->getPageImplById(previousContainerId);
        if (!prevContainer) {
            prevContainer = m_nlb->getObjImplById(previousContainerId);
        }
    }

    AbstractNodeItem* newContainer;
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
    auto command = new CommandChainCommand();
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

std::string NonLinearBookFacade::addObserver(NLBObserver* observer) {
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

UndoManager* NonLinearBookFacade::getUndoManagerByItemId(const std::string& id) {
    auto it = m_undoManagersMap.find(id);
    if (it != m_undoManagersMap.end()) {
        return it->second;
    }
    
    UndoManager* result = new UndoManager();
    m_undoManagersMap[id] = result;
    return result;
}

Page* NonLinearBookFacade::createPage(float left, float top) {
    std::string id = NLBUUID::randomUUID();
    auto result = new PageImpl(m_nlb, left, top);
    result->setId(id);
    m_newPagesPool[id] = result;
    return result;
}

void NonLinearBookFacade::addPage(Page* page) {
    auto pageId = page->getId();
    auto pageImpl = m_newPagesPool[pageId];
    
    if (pageImpl) {
        m_newPagesPool.erase(pageId);
        auto command = m_nlb->createAddPageCommand(pageImpl);
        m_undoManager->executeAndStore(command);
        notifyObservers();
    }
}

Obj* NonLinearBookFacade::createObj(float left, float top) {
    std::string id = NLBUUID::randomUUID();
    auto result = new ObjImpl(m_nlb, left, top);
    result->setId(id);
    m_newObjsPool[id] = result;
    return result;
}

void NonLinearBookFacade::addObj(Obj* obj) {
    auto objId = obj->getId();
    auto objImpl = m_newObjsPool[objId];
    
    if (objImpl) {
        m_newObjsPool.erase(objId);
        auto command = m_nlb->createAddObjCommand(objImpl);
        m_undoManager->executeAndStore(command);
        notifyObservers();
    }
}

Link* NonLinearBookFacade::createLink(NodeItem* item, NodeItem* target) {
    std::string id = NLBUUID::randomUUID();
    auto result = new LinkImpl(item, target->getId());
    result->setId(id);
    m_newLinksPool[id] = result;
    return result;
}

void NonLinearBookFacade::addLink(Link* link) {
    auto linkId = link->getId();
    auto linkImpl = m_newLinksPool[linkId];
    
    if (linkImpl) {
        m_newLinksPool.erase(linkId);
        auto parent = link->getParent();
        AbstractNodeItem* nodeItem = m_nlb->getPageImplById(parent->getId());
        if (!nodeItem) {
            nodeItem = m_nlb->getObjImplById(parent->getId());
        }
        
        if (nodeItem) {
            // Используем команду для добавления ссылки (с undo support)
            auto command = nodeItem->createAddLinkCommand(linkImpl);
            m_undoManager->executeAndStore(command);
        }
        
        notifyObservers();
    }
}

bool NonLinearBookFacade::hasChanges() {
    // Проверяем основной undo manager
    if (canUndo() || canRedo()) {
        return true;
    }
    
    // Проверяем все undo managers элементов
    for (const auto& entry : m_undoManagersMap) {
        if (entry.second->canUndo() || entry.second->canRedo()) {
            return true;
        }
    }
    
    // Проверяем дочерние фасады модулей
    for (const auto& facade : m_moduleFacades) {
        if (facade->hasChanges()) {
            return true;
        }
    }
    
    return false;
}

void NonLinearBookFacade::saveNLB(bool create, ProgressData* progressData) {
    try {
        std::string rootDir = m_nlb->getRootDir();
        progressData->setProgressValue(5);
        progressData->setNoteText("Opening VCS repository...");
        
        // Проверяем и создаем директорию если нужно
        if (!FileUtils::exists(rootDir)) {
            if (!FileUtils::createDirectoryRecursive(rootDir)) {
                throw NLBIOException("Cannot create NLB root directory");
            }
            m_vcsAdapter->initRepo(FileUtils::normalizePath(rootDir));
        } else {
            if (create) {
                m_vcsAdapter->initRepo(FileUtils::normalizePath(rootDir));
            } else {
                m_vcsAdapter->openRepo(FileUtils::normalizePath(rootDir));
            }
        }
        
        progressData->setProgressValue(15);
        progressData->setNoteText("Saving Non-Linear Book...");
        
        // Создаем FileManipulator для работы с файлами
        auto fileManipulator = new FileManipulator(m_vcsAdapter, rootDir);
        
        // Вычисляем прогресс для страниц
        int effectivePagesCount = m_nlb->getEffectivePagesCountForSave();
        int startProgress = 25;
        int maxProgress = 85;
        double itemsCountPerIncrement = std::ceil(
            static_cast<double>(effectivePagesCount) / static_cast<double>(maxProgress - startProgress)
        );
        
        // Создаем частичный прогресс для детальной обратной связи
        auto partialProgressData = new PartialProgressData(
            progressData, startProgress, maxProgress, static_cast<int>(itemsCountPerIncrement)
        );
        
        // Сохраняем книгу
        m_nlb->save(fileManipulator, progressData, partialProgressData);
        
    } catch (const std::exception& e) {
        throw NLBIOException("Error while saving: " + std::string(e.what()));
    }
}

void NonLinearBookFacade::saveAs(const std::string& nlbFolder, ProgressData* progressData) {
    // Устанавливаем новый корневой путь
    m_nlb->setRootDir(nlbFolder);
    
    // Сохраняем как новую книгу (create = true)
    saveNLB(true, progressData);
    
    // Очищаем undo и пулы после успешного сохранения
    clearUndosAndPools();
    
    notifyObservers();
}

void NonLinearBookFacade::load(const std::string& path, ProgressData* progressData) {
    try {
        // Проверяем существование директории
        if (!FileUtils::exists(path)) {
            throw NLBIOException("Specified NLB root directory " + path + " does not exist");
        }
        
        progressData->setNoteText("Opening VCS repository...");
        progressData->setProgressValue(5);
        
        // Открываем VCS репозиторий
        m_vcsAdapter->openRepo(FileUtils::normalizePath(path));
        
        progressData->setNoteText("Loading book contents...");
        progressData->setProgressValue(15);
        
        // Очищаем текущие данные
        clear();
        
        // Загружаем книгу через NonLinearBookImpl
        if (!m_nlb->load(path, *progressData)) {
            throw NLBIOException("Failed to load book from: " + path);
        }
        
        progressData->setProgressValue(70);
        progressData->setNoteText("Prepare to drawing...");
        
        notifyObservers();
        
    } catch (const std::exception& e) {
        throw NLBIOException("Error while loading: " + std::string(e.what()));
    }
}

void NonLinearBookFacade::deleteNode(NodeItem* nodeToDelete) {
    // Получаем связанные ссылки, которые нужно будет удалить
    auto adjacentLinks = m_nlb->getAssociatedLinks(nodeToDelete);
    
    // Пытаемся найти узел как страницу
    auto page = m_nlb->getPageImplById(nodeToDelete->getId());
    
    NLBCommand* command;
    
    if (page) {
        // Это страница - создаем команду удаления страницы
        command = m_nlb->createDeletePageCommand(page, adjacentLinks);
    } else {
        // Это объект - создаем команду удаления объекта
        auto obj = m_nlb->getObjImplById(nodeToDelete->getId());
        if (obj) {
            command = m_nlb->createDeleteObjCommand(obj, adjacentLinks);
        } else {
            throw NLBConsistencyException("Node with id " + nodeToDelete->getId() + " not found");
        }
    }
    
    // Выполняем команду через undo manager
    if (command) {
        m_undoManager->executeAndStore(command);
        notifyObservers();
    }
}

void NonLinearBookFacade::save(bool create, ProgressData* progressData) {
    // Сохраняем книгу
    saveNLB(create, progressData);
    
    // Очищаем undo и пулы после успешного сохранения
    clearUndosAndPools();
    
    notifyObservers();
}

void NonLinearBookFacade::deleteLink(Link* link) {
    auto parent = link->getParent();
    AbstractNodeItem* nodeItem = m_nlb->getPageImplById(parent->getId());
    if (!nodeItem) {
        nodeItem = m_nlb->getObjImplById(parent->getId());
    }
    
    if (nodeItem) {
        // Создаем команду удаления ссылки
        auto command = nodeItem->createDeleteLinkCommand(link);
        m_undoManager->executeAndStore(command);
        notifyObservers();
    }
}

void NonLinearBookFacade::invalidateAssociatedLinks(NodeItem* nodeItem) {
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
