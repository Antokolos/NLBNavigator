#include "nlb/api/LinkLw.h"
#include "nlb/api/Page.h"
#include "nlb/api/Modification.h"
#include "nlb/util/MultiLangString.h"
#include "nlb/api/Coords.h"
#include "nlb/api/CoordsLw.h"
#include "nlb/util/StringHelper.h"
#include "nlb/api/Constants.h"
#include "nlb/domain/ModificationImpl.h"
#include "nlb/api/NonLinearBook.h"

LinkLw::LinkLw(
    Type type,
    const std::string& target,
    std::shared_ptr<Page> parent,
    const MultiLangString& text,
    const MultiLangString& altText,
    const std::string& constrId,
    const std::string& varId,
    bool autoFlag,
    bool needsAction,
    bool once,
    bool positiveConstraint,
    bool shouldObeyToModuleConstraint,
    const std::string& mplLinkId,
    const std::vector<std::shared_ptr<Modification>>* modifications
) : m_type(type),
    m_target(target),
    m_parent(parent),
    m_text(std::make_shared<MultiLangString>(text)),
    m_altText(std::make_shared<MultiLangString>(altText)),
    m_constrId(constrId),
    m_varId(varId),
    m_auto(autoFlag),
    m_needsAction(needsAction),
    m_once(once),
    m_positiveConstraint(positiveConstraint),
    m_shouldObeyToModuleConstraint(shouldObeyToModuleConstraint)
{
    bool mplLink = !StringHelper::isEmpty(mplLinkId);
    if (mplLink) {
        m_mplLinkId = mplLinkId;
        if (modifications != nullptr) {
            m_modifications.insert(m_modifications.end(), modifications->begin(), modifications->end());
        }
    } else {
        m_mplLinkId = nlb::Constants::EMPTY_STRING;
        if (m_type == Type::AutowiredOut || (m_type == Type::AutowiredIn && !parent->isAutowire())) {
            auto modification = std::make_shared<ModificationImpl>();
            modification->setType(Modification::Type::ASSIGN);
            modification->setParent(IdentifiableItem::shared_from_this());
            modification->setVarId(
                (m_type == Type::AutowiredIn) ? 
                target + "_" + parent->getId() : 
                parent->getId() + "_" + target
            );
            modification->setExprId(
                (m_type == Type::AutowiredIn) ? 
                NonLinearBook::TRUE_VARID : 
                NonLinearBook::FALSE_VARID
            );
            m_modifications.push_back(modification);
        }
    }
}

std::string LinkLw::getVarId() const {
    return m_varId;
}

std::string LinkLw::getTarget() const {
    return m_target;
}

std::string LinkLw::getText() const {
    return m_text->get(m_parent->getCurrentNLB()->getLanguage());
}

const MultiLangString& LinkLw::getTexts() const {
    return *m_text;
}

std::string LinkLw::getAltText() const {
    return m_altText->get(m_parent->getCurrentNLB()->getLanguage());
}

const MultiLangString& LinkLw::getAltTexts() const {
    return *m_altText;
}

std::string LinkLw::getConstrId() const {
    return m_constrId;
}

bool LinkLw::isPositiveConstraint() const {
    return m_positiveConstraint;
}

bool LinkLw::isObeyToModuleConstraint() const {
    return m_shouldObeyToModuleConstraint;
}

bool LinkLw::isTraversalLink() const {
    return m_type == Type::Traverse;
}

bool LinkLw::isReturnLink() const {
    return m_type == Type::Return;
}

bool LinkLw::isTechnical() const {
    return isAuto() || m_type == Type::AutowiredIn || m_type == Type::AutowiredOut;
}

std::string LinkLw::getStroke() const {
    return nlb::Constants::EMPTY_STRING;
}

const Coords& LinkLw::getCoords() const {
    if (!m_coords) {
        m_coords = std::make_shared<CoordsLw>();
    }
    return *m_coords;
}

bool LinkLw::isAuto() const {
    return m_auto;
}

bool LinkLw::isNeedsAction() const {
    return m_needsAction;
}

bool LinkLw::isOnce() const {
    return m_once;
}

std::vector<std::shared_ptr<Modification>> LinkLw::getModifications() const {
    return m_modifications;
}

bool LinkLw::hasNoModifications() const {
    if (!m_modifications.empty()) {
        for (const auto& modification : m_modifications) {
            if (!modification->isDeleted()) {
                return false;
            }
        }
    }
    return true;
}

std::shared_ptr<Modification> LinkLw::getModificationById(const std::string& modId) const {
    for (const auto& modification : m_modifications) {
        if (modification->getId() == modId) {
            return modification;
        }
    }
    return nullptr;
}

std::string LinkLw::getId() const {
    return m_parent->getId() + "_" + m_target + "_" + m_mplLinkId + "_" + 
           (m_type == Type::Traverse ? "Traverse" :
            m_type == Type::Return ? "Return" :
            m_type == Type::AutowiredIn ? "AutowiredIn" : "AutowiredOut");
}

std::string LinkLw::getFullId() const {
    std::vector<std::string> temp = {m_parent->getId(), getId()};
    return StringHelper::formatSequence(temp);
}

bool LinkLw::isDeleted() const {
    return false;
}

std::shared_ptr<IdentifiableItem> LinkLw::getParent() const {
    return m_parent;
}

bool LinkLw::hasDeletedParent() const {
    std::shared_ptr<const IdentifiableItem> item = IdentifiableItem::shared_from_this();
    while ((item = item->getParent()) != nullptr) {
        if (item->isDeleted()) {
            return true;
        }
    }
    return false;
}

std::shared_ptr<NonLinearBook> LinkLw::getCurrentNLB() const {
    return m_parent->getCurrentNLB();
}

std::shared_ptr<SearchResult> LinkLw::searchText(const SearchContract& contract) const {
    return nullptr;
}

std::string LinkLw::addObserver(std::shared_ptr<NLBObserver> observer) {
    return nlb::Constants::EMPTY_STRING;
}

void LinkLw::removeObserver(const std::string& observerId) {
}

void LinkLw::notifyObservers() {
}