#pragma once

#include <functional>
#include <string>
#include <vector>

#include "nlb/platform/PlatformBridge.h"

#include "eos_sdk.h"
#include "eos_auth.h"
#include "eos_connect.h"
#include "eos_achievements.h"
#include "eos_stats.h"

namespace nlb::platform {

/**
 * @brief Epic Online Services (EOS SDK, C API): достижения — EOS_Achievements, статистика —
 * EOS_Stats_IngestStat. Таблицы рекордов EOS строятся на сервере из статистик, поэтому результат
 * в таблицу — это значение статистики с именем из [leaderboards] (в Dev Portal таблица
 * настраивается на эту статистику).
 *
 * Параметры — в разделе [epic] platform.ini: product_id, sandbox_id, deployment_id, client_id,
 * client_secret (обязательны), product_name, product_version, encryption_key (необязательны).
 * Вход: код обмена от Epic Games Launcher (-AUTH_PASSWORD=… с -AUTH_TYPE=exchangecode); для
 * разработки — Developer Authentication Tool: dev_host (например, localhost:6547) и
 * dev_credential в [epic]. Пока игрок не вошёл, вызовы копятся и уходят после входа.
 */
class EosPlatformServices : public IPlatformServices {
public:
    EosPlatformServices(const PlatformConfig& config, const std::vector<std::string>& args);
    ~EosPlatformServices() override;

    std::string name() const override { return "epic"; }
    std::string language() const override { return m_language; }
    bool available() const override { return m_user != nullptr; }
    const std::string& lastError() const { return m_error; }

    void unlockAchievement(const std::string& apiName) override;
    void setStat(const std::string& apiName, double value) override;
    void submitScore(const std::string& leaderboard, int64_t score) override;
    void tick() override;

private:
    void run(std::function<void()> action);  ///< сейчас, если игрок вошёл, иначе — после входа
    void ingest(const std::string& stat, int32_t amount);

    static void EOS_CALL onAuthLogin(const EOS_Auth_LoginCallbackInfo* data);
    static void EOS_CALL onConnectLogin(const EOS_Connect_LoginCallbackInfo* data);
    static void EOS_CALL onCreateUser(const EOS_Connect_CreateUserCallbackInfo* data);
    void loggedIn(EOS_ProductUserId user);

    EOS_HPlatform m_platform = nullptr;
    bool m_initialized = false;
    EOS_ProductUserId m_user = nullptr;
    std::vector<std::function<void()>> m_queue;
    std::string m_error;
    std::string m_language;  ///< -epiclocale от Epic Games Launcher
};

}  // namespace nlb::platform
