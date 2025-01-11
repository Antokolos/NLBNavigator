#pragma once

#include <string>
#include "ModifyingItem.h"
#include "MultiLangString.h"
#include "Coords.h"
#include "Constants.h"

class Link : public ModifyingItem {
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

    virtual std::string getVarId() const = 0;
    virtual std::string getTarget() const = 0;
    virtual std::string getText() const = 0;
    virtual const MultiLangString& getTexts() const = 0;
    virtual std::string getAltText() const = 0;
    virtual const MultiLangString& getAltTexts() const = 0;
    virtual std::string getConstrId() const = 0;
    virtual std::string getStroke() const = 0;
    virtual const Coords& getCoords() const = 0;
    virtual bool isAuto() const = 0;
    virtual bool isNeedsAction() const = 0;
    virtual bool isOnce() const = 0;

    /*!
     * @brief If true, constraint is fulfilled if its expression is true.
     *        If false, constraint is fulfilled if its expression is false
     *        (equivalent of boolean NOT operator)
     */
    virtual bool isPositiveConstraint() const = 0;

    /*!
     * @brief If true, module constraint also constrains this link, in addition to link's own constraint.
     *        If false, module constraint should not be applied.
     * @return true if module constraint should be applied, false otherwise
     */
    virtual bool isObeyToModuleConstraint() const = 0;

    virtual bool isTraversalLink() const = 0;
    virtual bool isReturnLink() const = 0;
    virtual bool isTechnical() const = 0;
};

// Определение статических констант
inline const std::string Link::DEFAULT_VAR_ID = nlb::Constants::EMPTY_STRING;
inline const std::string Link::DEFAULT_TARGET = nlb::Constants::EMPTY_STRING;
inline const MultiLangString Link::DEFAULT_TEXT = MultiLangString::createDefaultLinkText();
inline const MultiLangString Link::DEFAULT_ALT_TEXT = MultiLangString::createEmptyText();
inline const std::string Link::DEFAULT_CONSTR_ID = nlb::Constants::EMPTY_STRING;
inline const std::string Link::DEFAULT_STROKE = "0000FF";
inline const bool Link::DEFAULT_AUTO = false;
inline const bool Link::DEFAULT_NEEDS_ACTION = false;
inline const bool Link::DEFAULT_ONCE = false;
inline const bool Link::DEFAULT_TECHNICAL = false;