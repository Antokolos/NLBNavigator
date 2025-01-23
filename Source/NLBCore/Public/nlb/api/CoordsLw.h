#pragma once

#include "nlb/api/Coords.h"

/*!
 * \class CoordsLw
 * \brief Lightweight implementation of the Coords interface
 *
 * CoordsLw provides a basic implementation of the Coords interface,
 * representing rectangular coordinates with left, top, width, and height.
 */
class NLBCORE_API CoordsLw : public Coords {
public:
    /*!
     * \brief Default constructor initializing all coordinates to zero
     */
    CoordsLw();

    /*!
     * \brief Constructor with specified coordinates
     * \param left The left coordinate
     * \param top The top coordinate
     * \param width The width
     * \param height The height
     */
    CoordsLw(float left, float top, float width, float height);

    /*!
     * \brief Get the left coordinate
     * \return The left coordinate
     */
    float getLeft() const override;

    /*!
     * \brief Get the top coordinate
     * \return The top coordinate
     */
    float getTop() const override;

    /*!
     * \brief Get the width
     * \return The width
     */
    float getWidth() const override;

    /*!
     * \brief Get the height
     * \return The height
     */
    float getHeight() const override;

    /*!
     * \brief Set the left coordinate
     * \param left The new left coordinate
     */
    void setLeft(float left);

    /*!
     * \brief Set the top coordinate
     * \param top The new top coordinate
     */
    void setTop(float top);

    /*!
     * \brief Set the width
     * \param width The new width
     */
    void setWidth(float width);

    /*!
     * \brief Set the height
     * \param height The new height
     */
    void setHeight(float height);

    /*!
     * \brief Get a static reference to zero coordinates
     * \return Reference to a static CoordsLw with all coordinates set to zero
     */
    static Coords& getZeroCoords();

private:
    float m_left;     ///< Left coordinate
    float m_top;      ///< Top coordinate
    float m_width;    ///< Width
    float m_height;   ///< Height
};