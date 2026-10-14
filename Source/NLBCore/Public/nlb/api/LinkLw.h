#pragma once

#include "Link.h"
#include "Page.h"
#include <vector>
#include <string>
#include <memory>

/*!
 * @brief Implementation of Link interface
 */
class LinkLw : public Link {
public:
    enum class Type {
        Traverse,
        Return,
        AutowiredIn,
        AutowiredOut
    };

    /*!
     * @brief Constructs a new LinkLw object
     * @param type Link type
     * @param target Target identifier
     * @param parent Parent page
     * @param text Link text
     * @param altText Alternative text
     * @param constrId Constraint identifier
     * @param varId Variable identifier (has meaning only for MPL links)
     * @param auto Auto flag
     * @param needsAction Needs action flag
     * @param once Once flag
     * @param positiveConstraint Positive constraint flag
     * @param shouldObeyToModuleConstraint Module constraint flag
     * @param mplLinkId MPL link identifier
     * @param modifications List of modifications
     */
    LinkLw(
        Type type,
        const std::string& target,
        Page* parent,
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
        const std::vector<Modification*>* modifications
    );

    // Default destructor
    virtual ~LinkLw() = default;

    // Copy constructor and assignment operator
    LinkLw(const LinkLw&) = delete;
    LinkLw& operator=(const LinkLw&) = delete;

    // Interface implementation
    std::string getVarId() const override;
    std::string getTarget() const override;
    std::string getText() const override;
    const MultiLangString& getTexts() const override;
    std::string getAltText() const override;
    const MultiLangString& getAltTexts() const override;
    std::string getConstrId() const override;
    bool isPositiveConstraint() const override;
    bool isObeyToModuleConstraint() const override;
    bool isTraversalLink() const override;
    bool isReturnLink() const override;
    bool isTechnical() const override;
    std::string getStroke() const override;
    const Coords& getCoords() const override;
    bool isAuto() const override;
    bool isNeedsAction() const override;
    bool isOnce() const override;
    virtual std::vector<Modification*> getModifications() const;
    bool hasNoModifications() const;
    Modification* getModificationById(const std::string& modId) const;
    std::string getId() const override;
    std::string getFullId() const override;
    bool isDeleted() const override;
    IdentifiableItem* getParent() const override;
    bool hasDeletedParent() const override;
    NonLinearBook* getCurrentNLB() const override;
    virtual SearchResult* searchText(const SearchContract& contract) const override;
    std::string addObserver(NLBObserver* observer) override;
    void removeObserver(const std::string& observerId) override;
    void notifyObservers() override;

private:
    Type m_type;
    std::string m_target;
    std::string m_mplLinkId;
    IdentifiableItem* m_parent;
    MultiLangString* m_text;
    MultiLangString* m_altText;
    std::string m_constrId;
    std::string m_varId;
    bool m_auto;
    bool m_needsAction;
    bool m_once;
    bool m_positiveConstraint;
    bool m_shouldObeyToModuleConstraint;
    std::vector<Modification*> m_modifications;
    mutable Coords* m_coords;
};
