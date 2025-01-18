#pragma once
#include <memory>

class ExportConfig {
public:
    bool isConvertpng2jpg() const { return m_convertPng2Jpg; }
    int getQuality() const { return m_quality; }

private:
    bool m_convertPng2Jpg = true;  // default value
    int m_quality = 90;            // default value
};

class Config {
public:
    const ExportConfig& getExport() const { return m_export; }

private:
    ExportConfig m_export;
};

class Settings {
public:
    const Config& getDefaultConfig() const { return m_defaultConfig; }

private:
    Config m_defaultConfig;
};

class PropertyManager {
public:
    static const Settings& getSettings();
    
    // Make it non-constructible from outside
    PropertyManager(const PropertyManager&) = delete;
    PropertyManager& operator=(const PropertyManager&) = delete;

private:
    PropertyManager() = default;
    static PropertyManager& getInstance();
    
    Settings m_settings;
};