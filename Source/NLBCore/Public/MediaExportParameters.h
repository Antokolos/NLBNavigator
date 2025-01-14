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

    static const MediaExportParameters& fromPreset(Preset preset);
    static const MediaExportParameters& getDefault();

    // Made public to match Java implementation
    MediaExportParameters(Preset preset, bool convertPNG2JPG, int quality);
    
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