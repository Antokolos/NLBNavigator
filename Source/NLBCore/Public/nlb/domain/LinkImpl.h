#pragma once

#include <string>
#include <memory>
#include "nlb/domain/AbstractModifyingItem.h"
#include "nlb/api/Link.h"
#include "nlb/util/MultiLangString.h"
#include "nlb/domain/CoordsImpl.h"
#include "nlb/domain/ObserverHandler.h"
#include "nlb/domain/SearchResult.h"
#include "nlb/api/SearchContract.h"
#include "nlb/util/FileManipulator.h"

class NodeItem;

class LinkImpl : public AbstractModifyingItem, public Link {
public:
    LinkImpl();
    explicit LinkImpl(const std::shared_ptr<NodeItem>& parent);
    LinkImpl(const std::shared_ptr<NodeItem>& parent, const std::shared_ptr<Link>& sourceLink);
    LinkImpl(const std::shared_ptr<NodeItem>& parent, const std::string& target);

    // From Link interface
    std::string getVarId() const override { return m_varId; }
    std::string getTarget() const override { return m_target; }
    std::string getText() const override;
    const MultiLangString& getTexts() const override;
    std::string getAltText() const override;
    const MultiLangString& getAltTexts() const override;
    std::string getConstrId() const override { return m_constrId; }
    std::string getStroke() const override { return m_stroke; }
    const Coords& getCoords() const override { return m_coords; }
    bool isAuto() const override { return m_auto; }
    bool isNeedsAction() const override { return m_needsAction; }
    bool isOnce() const override { return m_once; }
    bool isPositiveConstraint() const override { return m_isPositiveConstraint; }
    bool isObeyToModuleConstraint() const override { return m_isObeyToModuleConstraint; }
    bool isTraversalLink() const override { return m_isTraversalLink; }
    bool isReturnLink() const override { return m_isReturnLink; }
    bool isTechnical() const override { return DEFAULT_TECHNICAL; }

    // Additional methods
    void setVarId(const std::string& varId) { m_varId = varId; }
    void setTarget(const std::string& target) { m_target = target; }
    void setTexts(const MultiLangString& text) { m_text = text; }
    void setText(const std::string& text);
    void setAltTexts(const MultiLangString& altText) { m_altText = altText; }
    void setAltText(const std::string& altText);
    void setConstrId(const std::string& constrId) { m_constrId = constrId; }
    void setStroke(const std::string& stroke) { m_stroke = stroke; }
    void setAuto(bool auto_) { m_auto = auto_; }
    void setNeedsAction(bool needsAction) { m_needsAction = needsAction; }
    void setOnce(bool once) { m_once = once; }

    std::shared_ptr<SearchResult> searchText(const std::shared_ptr<SearchContract>& contract);
    
    void writeLink(const std::shared_ptr<FileManipulator>& fileManipulator,
                  const std::string& linksDir);
    void readLink(const std::string& linkDir);

    // Observer pattern methods
    std::string addObserver(const std::shared_ptr<NLBObserver>& observer);
    void removeObserver(const std::string& observerId);
    void notifyObservers();

private:
    static const std::string COORDS_DIR_NAME;
    static const std::string VARID_FILE_NAME;
    static const std::string TARGET_FILE_NAME;
    static const std::string TEXT_SUBDIR_NAME;
    static const std::string ALT_TEXT_SUBDIR_NAME;
    static const std::string CONSTRID_FILE_NAME;
    static const std::string STROKE_FILE_NAME;
    static const std::string AUTO_FILE_NAME;
    static const std::string NEEDS_ACTION_FILE_NAME;
    static const std::string ONCE_FILE_NAME;

    void writeCoords(const std::shared_ptr<FileManipulator>& fileManipulator,
                    const std::string& linkDir);
    void readCoords(const std::string& linkDir);

    std::string m_varId{DEFAULT_VAR_ID};
    std::string m_target{DEFAULT_TARGET};
    MultiLangString m_text{DEFAULT_TEXT};
    MultiLangString m_altText{DEFAULT_ALT_TEXT};
    std::string m_constrId{DEFAULT_CONSTR_ID};
    std::string m_stroke{DEFAULT_STROKE};
    CoordsImpl m_coords;
    bool m_auto{DEFAULT_AUTO};
    bool m_needsAction{DEFAULT_NEEDS_ACTION};
    bool m_once{DEFAULT_ONCE};
    ObserverHandler m_observerHandler;
    bool m_isPositiveConstraint{true};
    bool m_isObeyToModuleConstraint{true};
    bool m_isTraversalLink{false};
    bool m_isReturnLink{false};
};