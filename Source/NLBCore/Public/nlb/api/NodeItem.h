#pragma once

#include <string>
#include <list>
#include <memory>

#include "ModifyingItem.h"
#include "NLBObservable.h"

// Forward declarations
class Coords;
class Link;

/*! 
 * @interface NodeItem
 * @brief Interface for node items that can be modified and observed
 * @extends ModifyingItem
 * @extends NLBObservable
 */
class NodeItem : public ModifyingItem, public NLBObservable {
public:
    // Constants
    static const std::string DEFAULT_STROKE;
    static const std::string DEFAULT_FILL;
    static const std::string DEFAULT_TEXTCOLOR;
    static const std::string DEFAULT_LNKORDER;
    static const std::string DEFAULT_CONTENT;
    static const std::string DEFAULT_TAG_ID;

    /*!
     * @brief Resize bars orientations
     */
    enum class Orientation {
        TOP,    //!< Top orientation
        BOTTOM, //!< Bottom orientation
        LEFT,   //!< Left orientation
        RIGHT   //!< Right orientation
    };

    // Size constants
    static const int DEFAULT_NODE_WIDTH = 100;
    static const int DEFAULT_NODE_HEIGHT = 50;
    static const int MIN_NODE_WIDTH = 70;
    static const int MIN_NODE_HEIGHT = 35;

    /*! 
     * @brief Gets the default tag ID
     * @return std::string The default tag ID
     */
    virtual std::string getDefaultTagId() const = 0;

    /*!
     * @brief Gets the stroke color
     * @return std::string The stroke color
     */
    virtual std::string getStroke() const = 0;

    /*!
     * @brief Gets the fill color
     * @return std::string The fill color
     */
    virtual std::string getFill() const = 0;

    /*!
     * @brief Gets the text color
     * @return std::string The text color
     */
    virtual std::string getTextColor() const = 0;

    /*!
     * @brief Gets the vector of contained object IDs
     * @return std::vector<std::string> vector of contained object IDs
     */
    virtual std::vector<std::string> getContainedObjIds() const = 0;

    /*!
     * @brief Gets the coordinates
     * @return Coords The coordinates object
     */
    virtual Coords* getCoords() const = 0;

    /*!
     * @brief Gets all links
     * @return std::vector<Link*> Vector of links
     */
    virtual std::vector<Link*> getLinks() const = 0;

    /*!
     * @brief Gets a link by its ID
     * @param linkId The ID of the link to find
     * @return Link* The found link or nullptr if not found
     */
    virtual Link* getLinkById(const std::string& linkId) const = 0;
    
    /*!
     * @brief Gets the external hierarchy path
     * @details Returns path-like expression, e.g. 'module1/module2' or empty string.
     *          The result is not empty for nodes which were imported from external modules
     *          and shows the position of the current node in the external modules hierarchy.
     * @return std::string Path-like hierarchy expression, e.g. 'module1/module2' or empty string
     */
    virtual std::string getExternalHierarchy() const = 0;

    /*!
     * @brief Virtual destructor
     */
    virtual ~NodeItem() = default;
};
