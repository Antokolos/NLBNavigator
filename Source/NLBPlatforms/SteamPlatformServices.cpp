#include "SteamPlatformServices.h"

#include <cmath>
#include <cstdlib>

#include "nlb/presentation/Localization.h"

namespace nlb::platform {

namespace {
template <typename T>
auto requestStats(T* stats, int) -> decltype(stats->RequestCurrentStats(), void()) { stats->RequestCurrentStats(); }
template <typename T>
void requestStats(T*, long) {}
}

SteamPlatformServices::SteamPlatformServices(const PlatformConfig& config) {
    // Запуск не из клиента Steam (разработка): app_id из platform.ini, как steam_appid.txt
    const std::string appId = config.value("steam", "app_id");
    if (!appId.empty() && !std::getenv("SteamAppId")) {
#ifdef _WIN32
        _putenv_s("SteamAppId", appId.c_str());
        _putenv_s("SteamGameId", appId.c_str());
#else
        setenv("SteamAppId", appId.c_str(), 0);
        setenv("SteamGameId", appId.c_str(), 0);
#endif
    }
    m_ok = SteamAPI_Init();
    if (!m_ok) {
        m_error = "SteamAPI_Init не удался (Steam не запущен или нет app_id)";
        return;
    }
    // Статистику игрока до SDK 1.61 нужно запросить явно, в новых RequestCurrentStats убран
    // (она приходит сама) — вызываем, только если метод есть в этом SDK
    requestStats(SteamUserStats(), 0);
}

SteamPlatformServices::~SteamPlatformServices() {
    if (m_ok) {
        SteamUserStats()->StoreStats();
        SteamAPI_Shutdown();
    }
}

std::string SteamPlatformServices::language() const {
    // язык игры, выбранный в клиенте Steam («russian», «english»…)
    return m_ok ? nlb::presentation::languageFromSteam(SteamApps()->GetCurrentGameLanguage()) : std::string();
}

void SteamPlatformServices::unlockAchievement(const std::string& apiName) {
    if (m_ok) SteamUserStats()->SetAchievement(apiName.c_str());
}

void SteamPlatformServices::setAchievementProgress(const std::string& apiName, int current, int max) {
    if (m_ok) SteamUserStats()->IndicateAchievementProgress(apiName.c_str(), static_cast<uint32>(current), static_cast<uint32>(max));
}

void SteamPlatformServices::setStat(const std::string& apiName, double value) {
    if (!m_ok) return;
    // тип статистики задан в Steamworks: целое — INT, иначе FLOAT
    if (std::floor(value) == value) SteamUserStats()->SetStat(apiName.c_str(), static_cast<int32>(value));
    else SteamUserStats()->SetStat(apiName.c_str(), static_cast<float>(value));
}

void SteamPlatformServices::submitScore(const std::string& leaderboard, int64_t score) {
    if (!m_ok) return;
    m_pending.emplace_back(leaderboard, static_cast<int32>(score));
    findNext();
}

void SteamPlatformServices::findNext() {
    // отправить то, для чего таблица уже найдена; искать — по одной таблице за раз
    while (!m_pending.empty()) {
        auto it = m_boards.find(m_pending.front().first);
        if (it == m_boards.end()) break;
        SteamUserStats()->UploadLeaderboardScore(it->second, k_ELeaderboardUploadScoreMethodKeepBest,
                                                 m_pending.front().second, nullptr, 0);
        m_pending.pop_front();
    }
    if (m_pending.empty() || !m_finding.empty()) return;
    m_finding = m_pending.front().first;
    m_findCall.Set(SteamUserStats()->FindLeaderboard(m_finding.c_str()), this, &SteamPlatformServices::onFind);
}

void SteamPlatformServices::onFind(LeaderboardFindResult_t* result, bool ioFailure) {
    const std::string name = m_finding;
    m_finding.clear();
    if (!ioFailure && result && result->m_bLeaderboardFound) {
        m_boards[name] = result->m_hSteamLeaderboard;
    } else {
        // таблицы нет в Steamworks — её результаты выбрасываем, чтобы не застрять
        for (auto it = m_pending.begin(); it != m_pending.end();) it = it->first == name ? m_pending.erase(it) : it + 1;
    }
    findNext();
}

void SteamPlatformServices::flush() {
    if (m_ok) SteamUserStats()->StoreStats();
}

void SteamPlatformServices::tick() {
    if (m_ok) SteamAPI_RunCallbacks();
}

}  // namespace nlb::platform
