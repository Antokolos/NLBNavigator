#include "MediaFileImpl.h"

MediaFileImpl::MediaFileImpl(const std::string& fileName)
    : m_fileName(fileName)
    , m_flagged(false)
    , m_preset(MediaExportParameters::Preset::DEFAULT)
{}

std::string MediaFileImpl::getFileName() const {
    return m_fileName;
}

void MediaFileImpl::setFileName(const std::string& fileName) {
    m_fileName = fileName;
}

std::string MediaFileImpl::getRedirect() const {
    return m_redirect;
}

void MediaFileImpl::setRedirect(const std::string& redirect) {
    m_redirect = redirect;
}

std::string MediaFileImpl::getConstrId() const {
    return m_constrId;
}

void MediaFileImpl::setConstrId(const std::string& constrId) {
    m_constrId = constrId;
}

bool MediaFileImpl::isFlagged() const {
    return m_flagged;
}

void MediaFileImpl::setFlagged(bool flagged) {
    m_flagged = flagged;
}

MediaExportParameters MediaFileImpl::getMediaExportParameters() const {
    return MediaExportParameters::fromPreset(m_preset);
}

MediaExportParameters::Preset MediaFileImpl::getPreset() const {
    return m_preset;
}

void MediaFileImpl::setPreset(MediaExportParameters::Preset preset) {
    m_preset = preset;
}

bool MediaFileImpl::operator==(const MediaFileImpl& other) const {
    if (this == &other) return true;
    if (m_flagged != other.m_flagged) return false;
    if (m_fileName != other.m_fileName) return false;
    if (m_redirect != other.m_redirect) return false;
    if (m_constrId != other.m_constrId) return false;
    return m_preset == other.m_preset;
}

bool MediaFileImpl::operator<(const MediaFile& other) const {
    return m_fileName < other.getFileName();
}

std::size_t MediaFileImpl::hash() const {
    std::size_t result = std::hash<std::string>{}(m_fileName);
    result = 31 * result + std::hash<std::string>{}(m_redirect);
    result = 31 * result + std::hash<std::string>{}(m_constrId);
    result = 31 * result + std::hash<bool>{}(m_flagged);
    result = 31 * result + std::hash<int>{}(static_cast<int>(m_preset));
    return result;
}