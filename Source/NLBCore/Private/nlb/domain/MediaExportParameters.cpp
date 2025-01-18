#include "nlb/domain/MediaExportParameters.h"
#include "nlb/api/PropertyManager.h"

// Initialize static const members
const MediaExportParameters MediaExportParameters::s_noChange(Preset::NOCHANGE, false, 0);
const MediaExportParameters MediaExportParameters::s_compressed(Preset::COMPRESSED, true, 80);
const MediaExportParameters MediaExportParameters::s_default = MediaExportParameters::createDefault();

MediaExportParameters::MediaExportParameters(Preset preset, bool convertPNG2JPG, int quality)
    : m_preset(preset)
    , m_convertPNG2JPG(convertPNG2JPG)
    , m_quality(quality)
{
}

const MediaExportParameters& MediaExportParameters::fromPreset(Preset preset) {
    switch (preset) {
        case Preset::NOCHANGE:
            return s_noChange;
        case Preset::COMPRESSED:
            return s_compressed;
        default:
            return s_default;
    }
}

const MediaExportParameters& MediaExportParameters::getDefault() {
    return s_default;
}

MediaExportParameters MediaExportParameters::createDefault() {
    auto& config = PropertyManager::getSettings().getDefaultConfig();
    return MediaExportParameters(
        Preset::DEFAULT,
        config.getExport().isConvertpng2jpg(),
        config.getExport().getQuality()
    );
}

MediaExportParameters::Preset MediaExportParameters::getPreset() const {
    return m_preset;
}

bool MediaExportParameters::isConvertPNG2JPG() const {
    return m_convertPNG2JPG;
}

int MediaExportParameters::getQuality() const {
    return m_quality;
}