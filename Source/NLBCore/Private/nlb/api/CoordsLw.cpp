#include "nlb/api/CoordsLw.h"

CoordsLw::CoordsLw() 
    : m_left(0.0f)
    , m_top(0.0f)
    , m_width(0.0f)
    , m_height(0.0f) {
}

CoordsLw::CoordsLw(float left, float top, float width, float height)
    : m_left(left)
    , m_top(top)
    , m_width(width)
    , m_height(height) {
}

float CoordsLw::getLeft() const {
    return m_left;
}

float CoordsLw::getTop() const {
    return m_top;
}

float CoordsLw::getWidth() const {
    return m_width;
}

float CoordsLw::getHeight() const {
    return m_height;
}

void CoordsLw::setLeft(float left) {
    m_left = left;
}

void CoordsLw::setTop(float top) {
    m_top = top;
}

void CoordsLw::setWidth(float width) {
    m_width = width;
}

void CoordsLw::setHeight(float height) {
    m_height = height;
}

Coords& CoordsLw::getZeroCoords() {
    static CoordsLw zeroCoords;
    return zeroCoords;
}
