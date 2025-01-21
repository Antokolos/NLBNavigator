#include "nlb/domain/CoordsImpl.h"
#include <filesystem>
#include <stdexcept>

const std::string CoordsImpl::LEFT_FILE_NAME = "left";
const std::string CoordsImpl::TOP_FILE_NAME = "top";
const std::string CoordsImpl::WIDTH_FILE_NAME = "width";
const std::string CoordsImpl::HEIGHT_FILE_NAME = "height";

CoordsImpl::CoordsImpl() = default;

float CoordsImpl::getLeft() const {
    return m_left;
}

void CoordsImpl::setLeft(float left) {
    m_left = left;
}

float CoordsImpl::getTop() const {
    return m_top;
}

void CoordsImpl::setTop(float top) {
    m_top = top;
}

float CoordsImpl::getWidth() const {
    return m_width;
}

void CoordsImpl::setWidth(float width) {
    m_width = width;
}

float CoordsImpl::getHeight() const {
    return m_height;
}

void CoordsImpl::setHeight(float height) {
    m_height = height;
}

void CoordsImpl::writeCoords(FileManipulator& fileManipulator, const std::string& coordsDir) {
    fileManipulator.writeRequiredString(coordsDir, LEFT_FILE_NAME, std::to_string(m_left));
    fileManipulator.writeRequiredString(coordsDir, TOP_FILE_NAME, std::to_string(m_top));
    fileManipulator.writeRequiredString(coordsDir, WIDTH_FILE_NAME, std::to_string(m_width));
    fileManipulator.writeRequiredString(coordsDir, HEIGHT_FILE_NAME, std::to_string(m_height));
}

void CoordsImpl::read(const std::string& coordsDir) {
    try {
        std::filesystem::path coordsDirPath = std::filesystem::canonical(coordsDir);
        
        m_left = std::stof(FileManipulator::getRequiredFileAsString(
            coordsDir, 
            LEFT_FILE_NAME,
            "Error reading left coords file; dir = " + coordsDirPath.string()
        ));
        
        m_top = std::stof(FileManipulator::getRequiredFileAsString(
            coordsDir,
            TOP_FILE_NAME,
            "Error reading top coords file; dir = " + coordsDirPath.string()
        ));
        
        m_width = std::stof(FileManipulator::getRequiredFileAsString(
            coordsDir,
            WIDTH_FILE_NAME,
            "Error reading width coords file; dir = " + coordsDirPath.string()
        ));
        
        m_height = std::stof(FileManipulator::getRequiredFileAsString(
            coordsDir,
            HEIGHT_FILE_NAME,
            "Error reading height coords file; dir = " + coordsDirPath.string()
        ));
    }
    catch (const std::filesystem::filesystem_error& e) {
        throw NLBIOException("Filesystem error occurred: " + std::string(e.what()));
    }
    catch (const std::exception& e) {
        throw NLBIOException("Error occurred while reading coordinates: " + std::string(e.what()));
    }
}