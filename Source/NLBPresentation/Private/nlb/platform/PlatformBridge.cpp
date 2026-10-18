#include "nlb/platform/PlatformBridge.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>

#include "nlb/util/FileUtils.h"
#include "shunting-yard.h"

namespace nlb::platform {

namespace {
std::string trim(const std::string& s) {
    const auto a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    return s.substr(a, s.find_last_not_of(" \t\r\n") - a + 1);
}
}

// ============================================================================ PlatformConfig

PlatformConfig PlatformConfig::load(const std::string& path) {
    std::string text;
    if (path.empty() || !FileUtils::exists(path) || !FileUtils::readFile(path, text)) return PlatformConfig{};
    return parse(text);
}

PlatformConfig PlatformConfig::parse(const std::string& text) {
    PlatformConfig c;
    std::istringstream in(text);
    std::string section;
    for (std::string line; std::getline(in, line);) {
        const auto comment = line.find(';');
        if (comment != std::string::npos) line = line.substr(0, comment);
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        if (line.front() == '[' && line.back() == ']') { section = trim(line.substr(1, line.size() - 2)); continue; }
        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        c.m_sections[section][trim(line.substr(0, eq))] = trim(line.substr(eq + 1));
    }
    return c;
}

std::string PlatformConfig::resolve(const std::string& section, const std::string& platform, const std::string& key) const {
    auto s = m_sections.find(section);
    if (s == m_sections.end()) return "";
    auto special = s->second.find(key + "@" + platform);
    if (special != s->second.end()) return special->second;
    auto common = s->second.find(key);
    return common == s->second.end() ? "" : common->second;
}

std::string PlatformConfig::achievement(const std::string& platform, const std::string& nlbName) const {
    const std::string mapped = resolve("achievements", platform, nlbName);
    return mapped.empty() ? nlbName : mapped;
}

std::vector<std::pair<std::string, std::string>> PlatformConfig::stats(const std::string& platform) const {
    std::vector<std::pair<std::string, std::string>> out;
    auto s = m_sections.find("stats");
    if (s == m_sections.end()) return out;
    for (const auto& [key, value] : s->second) {
        if (key.find('@') != std::string::npos) continue;
        const std::string api = resolve("stats", platform, key);
        if (!api.empty()) out.emplace_back(key, api);
    }
    return out;
}

std::vector<PlatformConfig::Board> PlatformConfig::leaderboards(const std::string& platform) const {
    std::vector<Board> out;
    auto s = m_sections.find("leaderboards");
    if (s == m_sections.end()) return out;
    for (const auto& [key, value] : s->second) {
        if (key.find('@') != std::string::npos || key.find('.') != std::string::npos) continue;
        const std::string api = resolve("leaderboards", platform, key);
        if (api.empty()) continue;
        out.push_back({key, api, resolve("leaderboards", platform, key + ".when") == "change"});
    }
    return out;
}

std::string PlatformConfig::value(const std::string& section, const std::string& key, const std::string& def) const {
    auto s = m_sections.find(section);
    if (s == m_sections.end()) return def;
    auto v = s->second.find(key);
    return v == s->second.end() ? def : v->second;
}

// ============================================================================ PlatformBridge

void PlatformBridge::add(std::unique_ptr<IPlatformServices> services) {
    if (services) m_services.push_back(std::move(services));
}

std::optional<double> PlatformBridge::number(const PlayerEngine& engine, const std::string& variable) const {
    try {
        const cparse::packToken v = engine.getVariable(variable);
        if (v->type & cparse::NUM) return v.asDouble();
        if (v->type == cparse::BOOL) return v.asBool() ? 1.0 : 0.0;
    } catch (...) {
    }
    return std::nullopt;
}

void PlatformBridge::onCommand(const std::vector<PlayerEngine::Event>& events, const PlayerEngine& engine, bool finished) {
    if (m_services.empty()) return;
    bool changed = false;
    for (const auto& e : events) {
        if (e.kind != PlayerEngine::Event::Kind::Achievement || e.id.empty()) continue;
        for (auto& s : m_services) s->unlockAchievement(m_config.achievement(s->name(), e.id));
        changed = true;
    }
    for (auto& s : m_services) {
        for (const auto& [variable, api] : m_config.stats(s->name())) {
            const auto v = number(engine, variable);
            const std::string key = s->name() + "|stat|" + variable;
            if (!v || (m_sent.count(key) && m_sent[key] == *v)) continue;
            m_sent[key] = *v;
            s->setStat(api, *v);
            changed = true;
        }
        for (const auto& board : m_config.leaderboards(s->name())) {
            const auto v = number(engine, board.variable);
            if (!v) continue;
            const std::string key = s->name() + "|board|" + board.variable;
            const bool send = board.onChange ? (!m_sent.count(key) || m_sent[key] != *v) : (finished && !m_finishSent);
            if (!send) continue;
            m_sent[key] = *v;
            s->submitScore(board.apiName, static_cast<int64_t>(std::llround(*v)));
            changed = true;
        }
    }
    if (finished) m_finishSent = true;
    if (changed) for (auto& s : m_services) s->flush();
}

std::string PlatformBridge::language() const {
    for (const auto& s : m_services) {
        const std::string l = s->language();
        if (!l.empty()) return l;
    }
    return "";
}

void PlatformBridge::tick() {
    for (auto& s : m_services) s->tick();
}

void PlatformBridge::reset() {
    m_sent.clear();
    m_finishSent = false;
}

// ============================================================================ LogPlatformServices

LogPlatformServices::LogPlatformServices(const std::string& path, std::string platformName) : m_name(std::move(platformName)) {
    if (path.empty() || path == "-") {
        m_out = std::make_unique<std::ostream>(std::cerr.rdbuf());
    } else {
        auto f = std::make_unique<std::ofstream>(FileUtils::nativePath(path), std::ios::binary | std::ios::trunc);
        if (*f) m_out = std::move(f);
    }
}

void LogPlatformServices::unlockAchievement(const std::string& apiName) {
    if (m_out) *m_out << "[" << m_name << "] achievement " << apiName << "\n";
}

void LogPlatformServices::setAchievementProgress(const std::string& apiName, int current, int max) {
    if (m_out) *m_out << "[" << m_name << "] progress " << apiName << " " << current << "/" << max << "\n";
}

void LogPlatformServices::setStat(const std::string& apiName, double value) {
    if (m_out) *m_out << "[" << m_name << "] stat " << apiName << " = " << value << "\n";
}

void LogPlatformServices::submitScore(const std::string& leaderboard, int64_t score) {
    if (m_out) *m_out << "[" << m_name << "] score " << leaderboard << " = " << score << "\n";
}

void LogPlatformServices::flush() {
    if (m_out) m_out->flush();
}

}  // namespace nlb::platform
