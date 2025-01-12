#pragma once

#include <string>

/*!
 * @class TextChunk
 * @brief Represents a chunk of text with type information
 */
class NLBCORE_API TextChunk {
public:
    static const std::string ACTION_TEXT_DEF;

    /*!
     * @brief The ChunkType enum represents possible type of the text block
     */
    enum class ChunkType {
        TEXT,       ///< Plain text
        VARIABLE,   ///< Variable name
        ACTION_TEXT, ///< Action text
        NEWLINE     ///< New line marker
    };

    /*!
     * @brief Default constructor
     */
    TextChunk() = default;

    /*!
     * @brief Gets the text content
     * @return Text content
     */
    const std::string& getText() const { return m_text; }

    /*!
     * @brief Sets the text content
     * @param text New text content
     */
    void setText(const std::string& text) { m_text = text; }

    /*!
     * @brief Gets the chunk type
     * @return Chunk type
     */
    ChunkType getType() const { return m_type; }

    /*!
     * @brief Sets the chunk type
     * @param type New chunk type
     */
    void setType(ChunkType type) { m_type = type; }

private:
    std::string m_text;
    ChunkType m_type{ChunkType::TEXT}; ///< Default initialization to TEXT
};
