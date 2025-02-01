#include "nlb/domain/LinkImpl.h"
#include "nlb/api/NodeItem.h"
#include "nlb/api/NonLinearBook.h"
#include "nlb/util/FileUtils.h"
#include "nlb/exception/NLBExceptions.h"

const std::string LinkImpl::COORDS_DIR_NAME = "coords";
const std::string LinkImpl::VARID_FILE_NAME = "varid";
const std::string LinkImpl::TARGET_FILE_NAME = "target";
const std::string LinkImpl::TEXT_SUBDIR_NAME = "text";
const std::string LinkImpl::ALT_TEXT_SUBDIR_NAME = "alt_text";
const std::string LinkImpl::CONSTRID_FILE_NAME = "constrid";
const std::string LinkImpl::STROKE_FILE_NAME = "stroke";
const std::string LinkImpl::AUTO_FILE_NAME = "auto";
const std::string LinkImpl::NEEDS_ACTION_FILE_NAME = "needsact";
const std::string LinkImpl::ONCE_FILE_NAME = "once";

LinkImpl::LinkImpl() : AbstractModifyingItem() {}

LinkImpl::LinkImpl(const std::shared_ptr<NodeItem>& parent)
    : AbstractModifyingItem(parent->getCurrentNLB()) {
    setParent(parent);
}

LinkImpl::LinkImpl(const std::shared_ptr<NodeItem>& parent, const std::shared_ptr<Link>& sourceLink)
    : AbstractModifyingItem(parent->getCurrentNLB()) {
    setId(sourceLink->getId());
    setDeleted(sourceLink->isDeleted());
    setParent(parent);

    for (const auto& modification : sourceLink->getModifications()) {
        addModification(std::make_shared<ModificationImpl>(modification, std::enable_shared_from_this<AbstractModifyingItem>::shared_from_this(), parent->getCurrentNLB()));
    }

    m_varId = sourceLink->getVarId();
    m_target = sourceLink->getTarget();
    m_text = sourceLink->getTexts();
    m_altText = sourceLink->getAltTexts();
    m_constrId = sourceLink->getConstrId();
    m_stroke = sourceLink->getStroke();
    m_coords.setLeft(sourceLink->getCoords().getLeft());
    m_coords.setTop(sourceLink->getCoords().getTop());
    m_coords.setWidth(sourceLink->getCoords().getWidth());
    m_coords.setHeight(sourceLink->getCoords().getHeight());
    m_isPositiveConstraint = sourceLink->isPositiveConstraint();
    m_isObeyToModuleConstraint = sourceLink->isObeyToModuleConstraint();
    m_isTraversalLink = sourceLink->isTraversalLink();
    m_isReturnLink = sourceLink->isReturnLink();
    m_auto = sourceLink->isAuto();
    m_needsAction = sourceLink->isNeedsAction();
    m_once = sourceLink->isOnce();
}

LinkImpl::LinkImpl(const std::shared_ptr<NodeItem>& parent, const std::string& target)
    : LinkImpl(parent) {
    m_target = target;
}

std::string LinkImpl::getText() const {
    return m_text.get(AbstractIdentifiableItem::getCurrentNLB()->getLanguage());
}

const MultiLangString& LinkImpl::getTexts() const {
    return m_text;
}

std::string LinkImpl::getAltText() const {
    return m_altText.get(AbstractIdentifiableItem::getCurrentNLB()->getLanguage());
}

const MultiLangString& LinkImpl::getAltTexts() const {
    return m_altText;
}

void LinkImpl::setText(const std::string& text) {
    m_text.put(AbstractIdentifiableItem::getCurrentNLB()->getLanguage(), text);
}

void LinkImpl::setAltText(const std::string& altText) {
    m_altText.put(AbstractIdentifiableItem::getCurrentNLB()->getLanguage(), altText);
}

std::shared_ptr<SearchResult> LinkImpl::searchText(const SearchContract& contract) const {
    auto result = AbstractIdentifiableItem::searchText(contract);
    if (result) {
        return result;
    }
    
    if (textMatches(m_text, contract)) {
        result = std::make_shared<SearchResult>();
        result->setId(AbstractModifyingItem::getId());
        result->setInformation(getText());
        return result;
    }
    
    if (textMatches(m_altText, contract)) {
        result = std::make_shared<SearchResult>();
        result->setId(AbstractModifyingItem::getId());
        result->setInformation(getAltText());
        return result;
    }
    
    return nullptr;
}

void LinkImpl::writeCoords(const std::shared_ptr<FileManipulator>& fileManipulator,
                          const std::string& linkDir) {
    const std::string coordsDir = linkDir + "/" + COORDS_DIR_NAME;
    fileManipulator->createDir(coordsDir,
        "Cannot create link text block coords directory for link with Id = " + AbstractModifyingItem::getId());
    m_coords.writeCoords(*fileManipulator, coordsDir);
}

void LinkImpl::readCoords(const std::string& linkDir) {
    const std::string coordsDir = linkDir + "/" + COORDS_DIR_NAME;
    if (!FileUtils::exists(coordsDir)) {
        throw NLBIOException(
            "Invalid NLB structure: coords directory does not exist for link with Id = " + AbstractModifyingItem::getId()
        );
    }
    m_coords.read(coordsDir);
}

void LinkImpl::writeLink(const std::shared_ptr<FileManipulator>& fileManipulator,
                        const std::string& linksDir) {
    const std::string linkDir = linksDir + "/" + AbstractModifyingItem::getId();
    
    if (AbstractIdentifiableItem::isDeleted()) {
        fileManipulator->deleteFileOrDir(linkDir);
    } else {
        fileManipulator->createDir(linkDir,
            "Cannot create NLB link directory for link with Id = " + AbstractModifyingItem::getId());

        fileManipulator->writeOptionalString(linkDir, VARID_FILE_NAME, m_varId, DEFAULT_VAR_ID);
        fileManipulator->writeOptionalString(linkDir, TARGET_FILE_NAME, m_target, DEFAULT_TARGET);
        fileManipulator->writeOptionalMultiLangString(
            linkDir + "/" + TEXT_SUBDIR_NAME,
            m_text,
            DEFAULT_TEXT
        );
        fileManipulator->writeOptionalMultiLangString(
            linkDir + "/" + ALT_TEXT_SUBDIR_NAME,
            m_altText,
            DEFAULT_ALT_TEXT
        );
        fileManipulator->writeOptionalString(linkDir, CONSTRID_FILE_NAME, m_constrId, DEFAULT_CONSTR_ID);
        fileManipulator->writeOptionalString(linkDir, STROKE_FILE_NAME, m_stroke, DEFAULT_STROKE);
        fileManipulator->writeOptionalString(
            linkDir,
            AUTO_FILE_NAME,
            std::to_string(m_auto),
            std::to_string(DEFAULT_AUTO)
        );
        fileManipulator->writeOptionalString(
            linkDir,
            NEEDS_ACTION_FILE_NAME,
            std::to_string(m_needsAction),
            std::to_string(DEFAULT_NEEDS_ACTION)
        );
        fileManipulator->writeOptionalString(
            linkDir,
            ONCE_FILE_NAME,
            std::to_string(m_once),
            std::to_string(DEFAULT_ONCE)
        );

        writeCoords(fileManipulator, linkDir);
        writeModOrderFile(fileManipulator, linkDir);
        writeModifications(fileManipulator, linkDir);
    }
}

void LinkImpl::readLink(const std::string& linkDir) {
    setId(FileUtils::getFileName(linkDir));
    
    m_varId = FileManipulator::getOptionalFileAsString(
        linkDir,
        VARID_FILE_NAME,
        DEFAULT_VAR_ID
    );

    m_target = FileManipulator::getRequiredFileAsString(
        linkDir,
        TARGET_FILE_NAME,
        "Error while reading link target for link with Id = " + AbstractModifyingItem::getId()
    );

    m_text = FileManipulator::readOptionalMultiLangString(
        linkDir + "/" + TEXT_SUBDIR_NAME,
        DEFAULT_TEXT
    );

    m_altText = FileManipulator::readOptionalMultiLangString(
        linkDir + "/" + ALT_TEXT_SUBDIR_NAME,
        DEFAULT_ALT_TEXT
    );

    m_constrId = FileManipulator::getOptionalFileAsString(
        linkDir,
        CONSTRID_FILE_NAME,
        DEFAULT_CONSTR_ID
    );

    m_stroke = FileManipulator::getOptionalFileAsString(
        linkDir,
        STROKE_FILE_NAME,
        DEFAULT_STROKE
    );

    m_auto = FileManipulator::getOptionalFileAsString(
        linkDir,
        AUTO_FILE_NAME,
        std::to_string(DEFAULT_AUTO)
    ) == "true";

    m_needsAction = FileManipulator::getOptionalFileAsString(
        linkDir,
        NEEDS_ACTION_FILE_NAME,
        std::to_string(DEFAULT_NEEDS_ACTION)
    ) == "true";

    m_once = FileManipulator::getOptionalFileAsString(
        linkDir,
        ONCE_FILE_NAME,
        std::to_string(DEFAULT_ONCE)
    ) == "true";

    readCoords(linkDir);
    readModifications(linkDir);
}

std::string LinkImpl::addObserver(std::shared_ptr<NLBObserver> observer) {
    return m_observerHandler.addObserver(observer);
}

void LinkImpl::removeObserver(const std::string& observerId) {
    m_observerHandler.removeObserver(observerId);
}

void LinkImpl::notifyObservers() {
    m_observerHandler.notifyObservers();
}