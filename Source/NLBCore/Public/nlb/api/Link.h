#pragma once

#include <string>
#include "nlb/api/ModifyingItem.h"
#include "nlb/api/Constants.h"
#include "nlb/api/NLBObservable.h"

// Forward declarations
class MultiLangString;
class Coords;

/*!
 * @brief Interface for link functionality
 */
class Link : public ModifyingItem, public NLBObservable {
public:
    static const std::string DEFAULT_VAR_ID;
    static const std::string DEFAULT_TARGET;
    static const MultiLangString DEFAULT_TEXT;
    static const MultiLangString DEFAULT_ALT_TEXT;
    static const std::string DEFAULT_CONSTR_ID;
    static const std::string DEFAULT_STROKE;
    static const bool DEFAULT_AUTO;
    static const bool DEFAULT_NEEDS_ACTION;
    static const bool DEFAULT_ONCE;
    static const bool DEFAULT_TECHNICAL;

    virtual ~Link() = default;

    /*!
     * @brief Gets the variable identifier
     * @return Variable identifier string 
     */
    virtual std::string getVarId() const = 0;

    /*!
     * @brief Gets the target identifier
     * @return Target identifier string
     */
    virtual std::string getTarget() const = 0;

    /*!
     * @brief Gets the link text
     * @return Link text string
     */
    virtual std::string getText() const = 0;

    /*!
     * @brief Gets the multilanguage texts
     * @return Reference to MultiLangString object containing texts
     */
    virtual const MultiLangString& getTexts() const = 0;

    /*!
     * @brief Gets the alternative text
     * @return Alternative text string
     */
    virtual std::string getAltText() const = 0;

    /*!
     * @brief Gets the multilanguage alternative texts
     * @return Reference to MultiLangString object containing alternative texts
     */
    virtual const MultiLangString& getAltTexts() const = 0;

    /*!
     * @brief Gets the constraint identifier
     * @return Constraint identifier string
     */
    virtual std::string getConstrId() const = 0;

    /*!
     * @brief Gets the stroke color
     * @return Stroke color string
     */
    virtual std::string getStroke() const = 0;

    /*!
     * @brief Gets the coordinates
     * @return Reference to Coords object
     */
    virtual const Coords& getCoords() const = 0;

    /*!
     * @brief Checks if link is automatic
     * @return true if automatic, false otherwise
     */
    virtual bool isAuto() const = 0;

    /*!
     * @brief Checks if link needs action
     * @return true if action needed, false otherwise
     */
    virtual bool isNeedsAction() const = 0;

    /*!
     * @brief Checks if link is one-time use
     * @return true if one-time use, false otherwise
     */
    virtual bool isOnce() const = 0;

    /*!
     * @brief If true, constraint is fulfilled if its expression is true.
     *        If false, constraint is fulfilled if its expression is false
     *        (equivalent of boolean NOT operator)
     * @return true for positive constraint, false for negative
     */
    virtual bool isPositiveConstraint() const = 0;

    /*!
     * @brief If true, module constraint also constrains this link, in addition to link's own constraint.
     *        If false, module constraint should not be applied.
     * @return true if module constraint should be applied, false otherwise
     */
    virtual bool isObeyToModuleConstraint() const = 0;

    /*!
     * @brief Checks if this is a traversal link
     * @return true if traversal link, false otherwise
     */
    virtual bool isTraversalLink() const = 0;

    /*!
     * @brief Checks if this is a return link
     * @return true if return link, false otherwise
     */
    virtual bool isReturnLink() const = 0;

    /*!
     * @brief Checks if this is a technical link
     * @return true if technical link, false otherwise
     */
    virtual bool isTechnical() const = 0;
};