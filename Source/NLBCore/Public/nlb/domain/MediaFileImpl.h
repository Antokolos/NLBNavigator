#pragma once

#include <string>
#include <ostream>
#include "nlb/api/MediaFile.h"
#include "nlb/domain/MediaExportParameters.h"

class MediaFileImpl : public MediaFile {
public:
    explicit MediaFileImpl(const std::string& fileName);
    
    // MediaFile interface implementation
    std::string getFileName() const override;
    std::string getRedirect() const override;
    std::string getConstrId() const override;
    bool isFlagged() const override;
    const MediaExportParameters &getMediaExportParameters() const override;
    bool operator<(const MediaFile& other) const override;
    
    // Строковое представление объекта
    std::string toString() const;
    
    // Additional methods
    void setFileName(const std::string& fileName);
    void setRedirect(const std::string& redirect);
    void setConstrId(const std::string& constrId);
    void setFlagged(bool flagged);
    MediaExportParameters::Preset getPreset() const;
    void setPreset(MediaExportParameters::Preset preset);
    
    // C++ equivalents of Java Object methods
    bool operator==(const MediaFileImpl& other) const;
    std::size_t hash() const;

private:
    std::string m_fileName;
    std::string m_redirect;
    std::string m_constrId;
    bool m_flagged;
    MediaExportParameters::Preset m_preset;
};

// Перегрузка оператора вывода в поток
inline std::ostream& operator<<(std::ostream& os, const MediaFileImpl& media) {
    os << media.toString();
    return os;
}

// Custom hash function for std::unordered_map/set support
namespace std {
    template<>
    struct hash<MediaFileImpl> {
        std::size_t operator()(const MediaFileImpl& media) const {
            return media.hash();
        }
    };
}