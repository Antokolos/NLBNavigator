#pragma once

#include <string>
#include "nlb/api/Coords.h"
#include "nlb/util/FileManipulator.h"

/*!
 * @brief Implementation of the Coords interface for storing and manipulating coordinates
 */
class CoordsImpl : public Coords {
public:
    /*!
     * @brief Default constructor needed for serialization
     */
    CoordsImpl();

    /*!
     * @brief Gets the left coordinate
     * @return Left coordinate value
     */
    float getLeft() const;

    /*!
     * @brief Sets the left coordinate
     * @param left New left coordinate value
     */
    void setLeft(float left);

    /*!
     * @brief Gets the top coordinate
     * @return Top coordinate value
     */
    float getTop() const;

    /*!
     * @brief Sets the top coordinate
     * @param top New top coordinate value
     */
    void setTop(float top);

    /*!
     * @brief Gets the width
     * @return Width value
     */
    float getWidth() const;

    /*!
     * @brief Sets the width
     * @param width New width value
     */
    void setWidth(float width);

    /*!
     * @brief Gets the height
     * @return Height value
     */
    float getHeight() const;

    /*!
     * @brief Sets the height
     * @param height New height value
     */
    void setHeight(float height);

    /*!
     * @brief Writes coordinates to files
     * @param fileManipulator File manipulation utility
     * @param coordsDir Directory to write the coordinates to
     * @throw NLBIOException On I/O errors
     * @throw NLBFileManipulationException On file manipulation errors
     * @throw NLBVCSException On version control errors
     */
    void writeCoords(FileManipulator& fileManipulator, const std::string& coordsDir);

    /*!
     * @brief Reads coordinates from files
     * @param coordsDir Directory to read the coordinates from
     * @throw NLBIOException On I/O errors
     */
    void read(const std::string& coordsDir);

private:
    static const std::string LEFT_FILE_NAME;
    static const std::string TOP_FILE_NAME;
    static const std::string WIDTH_FILE_NAME;
    static const std::string HEIGHT_FILE_NAME;

    float m_left = 0.0f;
    float m_top = 0.0f;
    float m_width = 0.0f;
    float m_height = 0.0f;
};