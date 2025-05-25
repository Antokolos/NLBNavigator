#pragma once

#include "nlb/api/Page.h"
#include "nlb/api/PartialProgressData.h"
#include "nlb/domain/AbstractNodeItem.h"
#include "nlb/util/MultiLangString.h"
#include "nlb/domain/NonLinearBookImpl.h"
#include <string>
#include <vector>

class PageImpl : public AbstractNodeItem, public Page, 
                            public std::enable_shared_from_this<PageImpl> {
public:
    /*!
     * @brief Default constructor
     * Needed for JAXB conversion, do not use for other purposes
     */
    PageImpl();

    /*!
     * @brief Constructor with current NonLinearBook
     * @param currentNLB Current NonLinearBook context
     */
    explicit PageImpl(std::shared_ptr<NonLinearBook> currentNLB);

    /*!
     * @brief Constructor with coordinates and current NonLinearBook
     * @param currentNLB Current NonLinearBook context
     * @param left Left coordinate
     * @param top Top coordinate
     */
    PageImpl(std::shared_ptr<NonLinearBook> currentNLB, float left, float top);

    /*!
     * @brief Copy constructor
     * @param source Source Page to copy
     * @param currentNLB Current NonLinearBook context
     * @param overwriteTheme Flag to overwrite theme
     */
    PageImpl(const std::shared_ptr<Page>& source, 
             std::shared_ptr<NonLinearBook> currentNLB, 
             bool overwriteTheme);

    // Getters and setters for various page attributes
    void setImageFileName(const std::string& imageFileName);
    std::string getImageFileName() const override;

    void setImageBackground(bool imageBackground);
    bool isImageBackground() const override;

    void setImageAnimated(bool imageAnimated);
    bool isImageAnimated() const override;

    void setSoundFileName(const std::string& soundFileName);
    std::string getSoundFileName() const override;

    bool isSoundSFX() const override;
    void setSoundSFX(bool soundSFX);

    void setText(const std::string& text);
    std::string getText() const override;
    MultiLangString getTexts() const override;

    Theme getTheme() const override;
    Theme getEffectiveTheme() const override;
    void setTheme(Theme theme);
    void setTexts(const MultiLangString& text);

    std::string getVarId() const override;
    std::string getTimerVarId() const override;
    void setVarId(const std::string& varId);
    void setTimerVarId(const std::string& timerVarId);

    std::string getModuleConstrId() const override;
    void setModuleConstrId(const std::string& moduleConstrId);

    std::string getCaption() const override;
    std::string getNotes() const override;
    MultiLangString getCaptions() const override;
    void setCaptions(const MultiLangString& caption);
    void setCaption(const std::string& caption);

    bool isUseCaption() const override;
    bool isUseMPL() const override;

    bool isLeaf() const override;
    bool isFinish() const override;

    std::string getTraverseText() const override;
    MultiLangString getTraverseTexts() const override;
    void setTraverseTexts(const MultiLangString& traverseText);

    bool isAutoTraverse() const override;
    void setAutoTraverse(bool autoTraverse);

    bool isAutoReturn() const override;
    void setAutoReturn(bool autoReturn);

    std::string getReturnText() const override;
    MultiLangString getReturnTexts() const override;
    void setReturnTexts(const MultiLangString& returnText);

    std::string getReturnPageId() const override;
    void setReturnPageId(const std::string& returnPageId);

    bool shouldReturn() const override;

    std::string getModuleName() const override;
    void setModuleName(const std::string& moduleName);

    bool isModuleExternal() const override;
    void setModuleExternal(bool moduleExternal);

    void setTraverseText(const std::string& traverseText);
    void setReturnText(const std::string& returnText);

    std::shared_ptr<NonLinearBook> getModule() const override;
    std::shared_ptr<NonLinearBookImpl> getModuleImpl() const {return m_module; };

    void setAutoIn(bool autoIn);
    void setNeedsAction(bool needsAction);
    void setAutoOut(bool autoOut);
    void setAutowireInConstrId(const std::string& autowireInConstrId);
    void setAutowireOutConstrId(const std::string& autowireOutConstrId);

    bool isAutowire() const override;
    bool isGlobalAutowire() const override;
    void setGlobalAutoWired(bool globalAutoWired);

    bool isNoSave() const override;
    void setNoSave(bool noSave);

    bool isAutosFirst() const override;
    void setAutosFirst(bool autosFirst);

    std::string getAutowireInText() const override;
    MultiLangString getAutowireInTexts() const override;
    void setAutowireInText(const std::string& autowireInText);
    void setAutowireInTexts(const MultiLangString& autowireInText);

    std::string getAutowireOutText() const override;
    MultiLangString getAutowireOutTexts() const override;
    void setAutowireOutText(const std::string& autowireOutText);
    void setAutowireOutTexts(const MultiLangString& autowireOutText);

    bool isAutoIn() const override;
    bool isNeedsAction() const override;
    bool isAutoOut() const override;
    std::string getAutowireInConstrId() const override;
    std::string getAutowireOutConstrId() const override;

    void writePage(std::shared_ptr<FileManipulator> fileManipulator,
                   const std::string& pagesDir,
                   std::shared_ptr<NonLinearBookImpl> nonLinearBook,
                   std::shared_ptr<PartialProgressData> partialProgressData);

    void readPage(const std::string& pageDir);

    /*!
     * @brief Create a filtered clone with variable substitutions
     * @param objIdsToBeExcluded Object IDs to exclude
     * @param linkIdsToBeExcluded Link IDs to exclude
     * @param linksToBeAdded Links to be added
     * @param visitedVars Variables to substitute
     * @return New filtered PageImpl
     */
    std::shared_ptr<PageImpl> createFilteredCloneWithSubstitutions(
        const std::vector<std::string>& objIdsToBeExcluded,
        const std::vector<std::string>& linkIdsToBeExcluded,
        const std::vector<std::shared_ptr<Link>>& linksToBeAdded,
        std::map<std::string, std::shared_ptr<void>> visitedVars
    );

    // Методы из IdentifiableItem
    virtual std::string getId() const override { return AbstractIdentifiableItem::getId(); }
    virtual std::string getFullId() const override { return AbstractNodeItem::getFullId(); }
    virtual bool isDeleted() const override { return AbstractNodeItem::isDeleted(); }
    virtual std::shared_ptr<IdentifiableItem> getParent() const override { return AbstractNodeItem::getParent(); }
    virtual bool hasDeletedParent() const override { return AbstractNodeItem::hasDeletedParent(); }
    virtual std::shared_ptr<NonLinearBook> getCurrentNLB() const override { return AbstractNodeItem::getCurrentNLB(); }
    virtual std::shared_ptr<SearchResult> searchText(const SearchContract& contract) const override { return AbstractNodeItem::searchText(contract); }

    // Методы из ModifyingItem
    virtual std::vector<std::shared_ptr<Modification>> getModifications() const override { return AbstractNodeItem::getModifications(); }
    virtual bool hasNoModifications() const override { return AbstractNodeItem::hasNoModifications(); }
    virtual std::shared_ptr<Modification> getModificationById(const std::string& modId) const override { return AbstractNodeItem::getModificationById(modId); }

    // Методы из NodeItem
    virtual std::string getDefaultTagId() const override { return AbstractNodeItem::getDefaultTagId(); }
    virtual std::string getStroke() const override { return AbstractNodeItem::getStroke(); }
    virtual std::string getFill() const override { return AbstractNodeItem::getFill(); }
    virtual std::string getTextColor() const override { return AbstractNodeItem::getTextColor(); }
    virtual std::vector<std::string> getContainedObjIds() const override { return AbstractNodeItem::getContainedObjIds(); }
    virtual std::shared_ptr<Coords> getCoords() const override { return AbstractNodeItem::getCoords(); }
    virtual std::vector<std::shared_ptr<Link>> getLinks() const override { return AbstractNodeItem::getLinks(); }
    virtual std::shared_ptr<Link> getLinkById(const std::string& id) const override { return AbstractNodeItem::getLinkById(id); }
    virtual std::string getExternalHierarchy() const override { return AbstractNodeItem::getExternalHierarchy(); }

    virtual std::string addObserver(std::shared_ptr<NLBObserver> observer) override { return AbstractNodeItem::addObserver(observer); }
    virtual void removeObserver(const std::string& observerId) override { AbstractNodeItem::removeObserver(observerId); }
    virtual void notifyObservers() override { AbstractNodeItem::notifyObservers(); }

private:
    // Private helper methods
    void init();
    void resetDefaultModuleName();
    void replaceVariablesInLinks(std::map<std::string, std::shared_ptr<void>> visitedVars);
    std::string generateObjText(const std::vector<std::string>& objIdsToBeExcluded, 
                                std::map<std::string, std::shared_ptr<void>> visitedVars);

    // Constants for file and directory names
    static const std::string TEXT_SUBDIR_NAME;
    static const std::string IMAGE_FILE_NAME;
    static const std::string IMAGEBG_FILE_NAME;
    static const std::string IMGANIM_FILE_NAME;
    static const std::string SOUND_FILE_NAME;
    static const std::string SOUND_SFX_FILE_NAME;
    static const std::string THEME_FILE_NAME;
    static const std::string VARID_FILE_NAME;
    static const std::string TVARID_FILE_NAME;
    static const std::string CAPTION_SUBDIR_NAME;
    static const std::string USE_CAPT_FILE_NAME;
    static const std::string USE_MPL_FILE_NAME;
    static const std::string MODULE_SUBDIR_NAME;
    static const std::string MODNAME_FILE_NAME;
    static const std::string EXTMOD_FILE_NAME;
    static const std::string TRAVTEXT_FILE_NAME;
    static const std::string AUTOTRAV_FILE_NAME;
    static const std::string AUTORET_FILE_NAME;
    static const std::string RETTEXT_SUBDIR_NAME;
    static const std::string RETPAGE_FILE_NAME;
    static const std::string MODCNSID_FILE_NAME;
    static const std::string AUTO_IN_TEXT_SUBDIR_NAME;
    static const std::string AUTO_OUT_TEXT_SUBDIR_NAME;
    static const std::string AUTO_IN_FILE_NAME;
    static const std::string NEEDS_ACTION_FILE_NAME;
    static const std::string AUTO_OUT_FILE_NAME;
    static const std::string AUTOWIRE_IN_CONSTRID_FILE_NAME;
    static const std::string AUTOWIRE_OUT_CONSTRID_FILE_NAME;
    static const std::string NOSAVE_FILE_NAME;
    static const std::string AUTOS_FIRST_FILE_NAME;
    static const std::string GLOBAL_AUTOWIRE_FILE_NAME;

    static const std::string DEFAULT_MODULE_NAME_FORMAT;


    // Attributes corresponding to Java implementation
    std::string m_imageFileName;
    bool m_imageBackground;
    bool m_imageAnimated;
    std::string m_soundFileName;
    bool m_soundSFX;
    std::string m_varId;
    std::string m_timerVarId;
    MultiLangString m_caption;
    bool m_useCaption;
    bool m_useMPL;
    MultiLangString m_text;
    Theme m_theme;
    std::string m_moduleName;
    bool m_moduleExternal;
    std::string m_defaultModuleName;
    MultiLangString m_traverseText;
    bool m_autoTraverse;
    bool m_autoReturn;
    MultiLangString m_returnText;
    std::string m_returnPageId;
    std::string m_moduleConstrId;

    std::shared_ptr<NonLinearBookImpl> m_module;

    MultiLangString m_autowireInText;
    MultiLangString m_autowireOutText;

    bool m_autoIn;
    bool m_needsAction;
    bool m_autoOut;
    std::string m_autowireInConstrId;
    std::string m_autowireOutConstrId;

    bool m_globalAutoWired;
    bool m_noSave;
    bool m_autosFirst;
};