#include "nlb/api/PropertyManager.h"

#include "nlb/util/FileUtils.h"
#include "nlb/util/Json.h"

ExportConfig::ExportConfig() {
    // как config.xml NLBB
    m_texts["en"] = {"Nothing happens.", "Hm... This is strange thing...", "Does not work..."};
    m_texts["ru"] = {"Ничего не происходит.", "Хм... Странная штука...", "Не сработает..."};
}

const ExportConfig::Texts& ExportConfig::getTexts(const std::string& lang) const {
    auto it = m_texts.find(lang);
    if (it == m_texts.end()) it = m_texts.find("en");
    return it->second;
}

PropertyManager& PropertyManager::getInstance() {
    static PropertyManager instance;
    return instance;
}

const Settings& PropertyManager::getSettings() {
    return getInstance().m_settings;
}

bool PropertyManager::loadConfig(const std::string& path, std::string& error) {
    std::string text;
    if (!FileUtils::readFile(path, text)) {
        error = "не удалось прочитать " + path;
        return false;
    }
    auto root = Json::parse(text, error);
    if (!root || !root->isObject()) {
        if (error.empty()) error = "ожидался объект JSON";
        error = path + ": " + error;
        return false;
    }
    const Json* exp = root->get("export");
    if (!exp) return true;
    ExportConfig& e = getInstance().m_settings.getDefaultConfig().getExport();
    if (const Json* v = exp->get("convertpng2jpg")) e.m_convertPng2Jpg = v->asBool(e.m_convertPng2Jpg);
    if (const Json* v = exp->get("quality")) e.m_quality = static_cast<int>(v->asNumber(e.m_quality));
    if (const Json* v = exp->get("game-forcedsc")) e.m_gameForceDsc = v->asBool(e.m_gameForceDsc);
    if (const Json* v = exp->get("needs-action-count")) e.m_needsActionCount = static_cast<int>(v->asNumber(e.m_needsActionCount));
    if (const Json* texts = exp->get("texts")) {
        for (const auto& [lang, t] : texts->object()) {
            ExportConfig::Texts& dst = e.m_texts[lang];
            if (const Json* v = t->get("game-act")) dst.act = v->asString(dst.act);
            if (const Json* v = t->get("game-inv")) dst.inv = v->asString(dst.inv);
            if (const Json* v = t->get("game-nouse")) dst.nouse = v->asString(dst.nouse);
        }
    }
    if (e.m_quality < 1) e.m_quality = 1;
    if (e.m_quality > 100) e.m_quality = 100;
    return true;
}

std::string PropertyManager::loadDefaultConfig(const std::string& programDir) {
    for (const std::string& dir : {std::string("."), programDir}) {
        if (dir.empty()) continue;
        const std::string path = FileUtils::combinePath(dir, "nlb.json");
        std::string error;
        if (FileUtils::exists(path) && loadConfig(path, error)) return path;
    }
    return "";
}
