#pragma once

#include <deque>
#include <map>
#include <string>
#include <utility>

#include "nlb/platform/PlatformBridge.h"
#include "steam/steam_api.h"

namespace nlb::platform {

/**
 * @brief Steam (Steamworks SDK): достижения и статистика — ISteamUserStats, таблицы рекордов —
 * FindLeaderboard + UploadLeaderboardScore (лучший результат, KeepBest).
 *
 * Достижения, статистики и таблицы заводятся в Steamworks; их API-имена — в platform.ini.
 * Для запуска не из Steam (разработка) app_id из [steam] передаётся через SteamAppId, как
 * steam_appid.txt.
 */
class SteamPlatformServices : public IPlatformServices {
public:
    explicit SteamPlatformServices(const PlatformConfig& config);
    ~SteamPlatformServices() override;

    std::string name() const override { return "steam"; }
    std::string language() const override;
    bool available() const override { return m_ok; }
    const std::string& lastError() const { return m_error; }

    void unlockAchievement(const std::string& apiName) override;
    void setAchievementProgress(const std::string& apiName, int current, int max) override;
    void setStat(const std::string& apiName, double value) override;
    void submitScore(const std::string& leaderboard, int64_t score) override;
    void flush() override;
    void tick() override;

private:
    void findNext();
    void onFind(LeaderboardFindResult_t* result, bool ioFailure);

    bool m_ok = false;
    std::string m_error;
    std::map<std::string, SteamLeaderboard_t> m_boards;          ///< найденные таблицы
    std::deque<std::pair<std::string, int32>> m_pending;         ///< результаты до поиска таблицы
    std::string m_finding;                                       ///< таблица, которую ищем сейчас
    CCallResult<SteamPlatformServices, LeaderboardFindResult_t> m_findCall;
};

}  // namespace nlb::platform
