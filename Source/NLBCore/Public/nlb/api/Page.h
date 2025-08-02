#pragma once

#include "nlb/api/NodeItem.h"
#include "nlb/api/Theme.h"

class MultiLangString;
class NonLinearBook;

/*!
 * @class Page
 * @brief Interface for page objects in non-linear book
 * @extends NodeItem
 */
class Page : public NodeItem {
public:
    // Default constants
    static const std::string DEFAULT_IMAGE_FILE_NAME;
    static const std::string DEFAULT_SOUND_FILE_NAME;
    static const MultiLangString DEFAULT_TEXT;
    static const Theme DEFAULT_THEME;
    static const std::string DEFAULT_VARID;
    static const std::string DEFAULT_TVARID;
    static const MultiLangString DEFAULT_CAPTION;
    static const MultiLangString DEFAULT_TRAVERSE_TEXT;
    static const MultiLangString DEFAULT_AUTOWIRE_IN_TEXT;
    static const MultiLangString DEFAULT_AUTOWIRE_OUT_TEXT;
    static const bool DEFAULT_USE_CAPTION = false;
    static const bool DEFAULT_USE_MPL = false;
    static const bool DEFAULT_IMAGE_BACKGROUND = false;
    static const bool DEFAULT_IMAGE_ANIMATED = false;
    static const bool DEFAULT_SOUND_SFX = false;
    static const bool DEFAULT_AUTO_TRAVERSE = false;
    static const bool DEFAULT_AUTO_RETURN = false;
    static const bool DEFAULT_AUTO_IN = false;
    static const bool DEFAULT_NEEDS_ACTION = false;
    static const bool DEFAULT_AUTO_OUT = false;
    static const std::string DEFAULT_AUTOWIRE_IN_CONSTR_ID;
    static const std::string DEFAULT_AUTOWIRE_OUT_CONSTR_ID;
    static const bool DEFAULT_GLOBAL_AUTOWIRED = false;
    static const bool DEFAULT_NOSAVE = false;
    static const bool DEFAULT_AUTOS_FIRST = false;
    
    /*!
     * @brief Set to empty String.
     * This means that by default pages should not return to the parent module
     */
    static const MultiLangString DEFAULT_RETURN_TEXT;
    static const std::string DEFAULT_RETURN_PAGE_ID;
    static const std::string DEFAULT_MODULE_CONSTR_ID;
    static const bool DEFAULT_MODULE_EXTERNAL = false;

    // Image related methods
    virtual std::string getImageFileName() const = 0;
    virtual bool isImageBackground() const = 0;
    virtual bool isImageAnimated() const = 0;

    // Sound related methods
    virtual std::string getSoundFileName() const = 0;
    virtual bool isSoundSFX() const = 0;

    // Text and content methods
    virtual std::string getText() const = 0;
    virtual MultiLangString getTexts() const = 0;
    virtual Theme getTheme() const = 0;
    virtual Theme getEffectiveTheme() const = 0;
    virtual std::string getVarId() const = 0;
    virtual std::string getTimerVarId() const = 0;
    virtual std::string getCaption() const = 0;
    virtual std::string getNotes() const = 0;
    virtual MultiLangString getCaptions() const = 0;
    virtual bool isUseCaption() const = 0;
    virtual bool isUseMPL() const = 0;
    virtual bool isLeaf() const = 0;
    virtual bool isFinish() const = 0;

    // Traverse related methods
    virtual std::string getTraverseText() const = 0;
    virtual MultiLangString getTraverseTexts() const = 0;
    virtual bool isAutoTraverse() const = 0;
    virtual bool isAutoReturn() const = 0;

    // Return related methods
    virtual std::string getReturnText() const = 0;
    virtual MultiLangString getReturnTexts() const = 0;
    virtual std::string getReturnPageId() const = 0;

    /*!
     * @brief If false, then return link from module from this page should not be added
     * (truly end of the story)
     * @return true if should return, false otherwise
     */
    virtual bool shouldReturn() const = 0;

    // Module related methods
    virtual std::string getModuleConstrId() const = 0;
    virtual std::string getModuleName() const = 0;
    virtual bool isModuleExternal() const = 0;
    virtual std::shared_ptr<NonLinearBook> getModule() const = 0;

    // Autowire related methods
    virtual bool isAutowire() const = 0;
    virtual bool isGlobalAutowire() const = 0;
    virtual bool isNoSave() const = 0;
    virtual bool isAutosFirst() const = 0;
    virtual std::string getAutowireInText() const = 0;
    virtual MultiLangString getAutowireInTexts() const = 0;
    virtual std::string getAutowireOutText() const = 0;
    virtual MultiLangString getAutowireOutTexts() const = 0;
    virtual bool isAutoIn() const = 0;
    virtual bool isNeedsAction() const = 0;
    virtual bool isAutoOut() const = 0;
    virtual std::string getAutowireInConstrId() const = 0;
    virtual std::string getAutowireOutConstrId() const = 0;

    // Virtual destructor
    virtual ~Page() = default;
};