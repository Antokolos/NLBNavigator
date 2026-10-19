#pragma once
#include <map>
#include <memory>
#include <string>

/**
 * Настройки экспорта книги — аналог раздела <export> config.xml NLBB, файл nlb.json (пример —
 * nlb.json в корне репозитория, описание — docs/pack.md). Без файла — те же значения по
 * умолчанию, что в config.xml NLBB.
 */
class ExportConfig {
public:
    struct Texts { std::string act, inv, nouse; };

    ExportConfig();
    bool isConvertpng2jpg() const { return m_convertPng2Jpg; }
    int getQuality() const { return m_quality; }
    bool isGameForceDsc() const { return m_gameForceDsc; }
    int getNeedsActionCount() const { return m_needsActionCount; }
    /// Тексты по языку; нет такого языка — английские
    const Texts& getTexts(const std::string& lang) const;

private:
    friend class PropertyManager;
    bool m_convertPng2Jpg = true;
    int m_quality = 80;
    bool m_gameForceDsc = false;
    int m_needsActionCount = 3;
    std::map<std::string, Texts> m_texts;
};

class Config {
public:
    const ExportConfig& getExport() const { return m_export; }
    ExportConfig& getExport() { return m_export; }

private:
    ExportConfig m_export;
};

class Settings {
public:
    const Config& getDefaultConfig() const { return m_defaultConfig; }
    Config& getDefaultConfig() { return m_defaultConfig; }

private:
    Config m_defaultConfig;
};

class PropertyManager {
public:
    static const Settings& getSettings();

    /// Прочитать nlb.json (поля, которых нет, остаются прежними). false — файл не прочитан или
    /// не JSON (error — что не так)
    static bool loadConfig(const std::string& path, std::string& error);
    /// Найти и прочитать nlb.json: в текущем каталоге, затем в каталоге программы. Путь
    /// прочитанного файла или пусто
    static std::string loadDefaultConfig(const std::string& programDir);

    PropertyManager(const PropertyManager&) = delete;
    PropertyManager& operator=(const PropertyManager&) = delete;

private:
    PropertyManager() = default;
    static PropertyManager& getInstance();

    Settings m_settings;
};
