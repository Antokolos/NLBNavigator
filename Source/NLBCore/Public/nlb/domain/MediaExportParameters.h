#pragma once

#include <memory>

class PropertyManager; // Forward declaration

class MediaExportParameters {
public:
    enum class Preset {
        CUSTOM,
        DEFAULT,
        NOCHANGE,
        COMPRESSED
    };
    
    MediaExportParameters() 
            : m_preset(Preset::DEFAULT)
            , m_convertPNG2JPG(false)
            , m_quality(90) {
    }
    
    MediaExportParameters(Preset preset, bool convertPNG2JPG, int quality);
    
    MediaExportParameters(const MediaExportParameters& other)
        : m_preset(other.m_preset)
        , m_convertPNG2JPG(other.m_convertPNG2JPG)
        , m_quality(other.m_quality) {
    }
    
    MediaExportParameters& operator=(const MediaExportParameters& other) {
        if (this != &other) {
            m_preset = other.m_preset;
            m_convertPNG2JPG = other.m_convertPNG2JPG;
            m_quality = other.m_quality;
        }
        return *this;
    }
    
    static const MediaExportParameters& fromPreset(Preset preset);
    static const MediaExportParameters& getDefault();
    
    Preset getPreset() const;
    bool isConvertPNG2JPG() const;
    int getQuality() const;

private:
    static MediaExportParameters createDefault();
    
    Preset m_preset;
    bool m_convertPNG2JPG;
    int m_quality;

    // Static members
    static const MediaExportParameters s_noChange;
    static const MediaExportParameters s_compressed;
    static const MediaExportParameters s_default;
};