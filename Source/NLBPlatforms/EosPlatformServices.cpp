#include "EosPlatformServices.h"

#include <iostream>

#include "nlb/presentation/Localization.h"

namespace nlb::platform {

namespace {
/// -NAME=value из командной строки (так Epic Games Launcher передаёт код входа)
std::string argValue(const std::vector<std::string>& args, const std::string& name) {
    const std::string prefix = "-" + name + "=";
    for (const auto& a : args) {
        if (a.size() > prefix.size() && a.compare(0, prefix.size(), prefix) == 0) return a.substr(prefix.size());
    }
    return "";
}
}

EosPlatformServices::EosPlatformServices(const PlatformConfig& config, const std::vector<std::string>& args) {
    m_language = nlb::presentation::normalizeLanguage(argValue(args, "epiclocale"));
    const std::string productName = config.value("epic", "product_name", "NLBNavigator");
    const std::string productVersion = config.value("epic", "product_version", "1.0");
    EOS_InitializeOptions init{};
    init.ApiVersion = EOS_INITIALIZE_API_LATEST;
    init.ProductName = productName.c_str();
    init.ProductVersion = productVersion.c_str();
    const EOS_EResult initResult = EOS_Initialize(&init);
    if (initResult != EOS_EResult::EOS_Success && initResult != EOS_EResult::EOS_AlreadyConfigured) {
        m_error = std::string("EOS_Initialize: ") + EOS_EResult_ToString(initResult);
        return;
    }
    m_initialized = initResult == EOS_EResult::EOS_Success;

    const std::string productId = config.value("epic", "product_id"), sandboxId = config.value("epic", "sandbox_id"),
                      deploymentId = config.value("epic", "deployment_id"), clientId = config.value("epic", "client_id"),
                      clientSecret = config.value("epic", "client_secret"), encryptionKey = config.value("epic", "encryption_key");
    if (productId.empty() || sandboxId.empty() || deploymentId.empty() || clientId.empty()) {
        m_error = "в [epic] platform.ini нужны product_id, sandbox_id, deployment_id, client_id, client_secret";
        return;
    }
    EOS_Platform_Options po{};
    po.ApiVersion = EOS_PLATFORM_OPTIONS_API_LATEST;
    po.ProductId = productId.c_str();
    po.SandboxId = sandboxId.c_str();
    po.DeploymentId = deploymentId.c_str();
    po.ClientCredentials.ClientId = clientId.c_str();
    po.ClientCredentials.ClientSecret = clientSecret.c_str();
    po.EncryptionKey = encryptionKey.empty() ? nullptr : encryptionKey.c_str();
    po.bIsServer = EOS_FALSE;
    m_platform = EOS_Platform_Create(&po);
    if (!m_platform) {
        m_error = "EOS_Platform_Create не удался";
        return;
    }

    // Вход: код обмена из Epic Games Launcher, а без него — Developer Authentication Tool
    EOS_Auth_Credentials creds{};
    creds.ApiVersion = EOS_AUTH_CREDENTIALS_API_LATEST;
    const std::string exchange = argValue(args, "AUTH_PASSWORD");
    const std::string devHost = config.value("epic", "dev_host"), devCredential = config.value("epic", "dev_credential");
    if (!exchange.empty()) {
        creds.Type = EOS_ELoginCredentialType::EOS_LCT_ExchangeCode;
        creds.Token = exchange.c_str();
    } else if (!devHost.empty() && !devCredential.empty()) {
        creds.Type = EOS_ELoginCredentialType::EOS_LCT_Developer;
        creds.Id = devHost.c_str();
        creds.Token = devCredential.c_str();
    } else {
        m_error = "нет кода входа (-AUTH_PASSWORD от Epic Games Launcher) и не заданы dev_host/dev_credential";
        return;
    }
    EOS_Auth_LoginOptions lo{};
    lo.ApiVersion = EOS_AUTH_LOGIN_API_LATEST;
    lo.Credentials = &creds;
    lo.ScopeFlags = EOS_EAuthScopeFlags::EOS_AS_BasicProfile;
    EOS_Auth_Login(EOS_Platform_GetAuthInterface(m_platform), &lo, this, &EosPlatformServices::onAuthLogin);
}

EosPlatformServices::~EosPlatformServices() {
    if (m_platform) EOS_Platform_Release(m_platform);
    if (m_initialized) EOS_Shutdown();
}

void EOS_CALL EosPlatformServices::onAuthLogin(const EOS_Auth_LoginCallbackInfo* data) {
    auto* self = static_cast<EosPlatformServices*>(data->ClientData);
    if (data->ResultCode != EOS_EResult::EOS_Success) {
        self->m_error = std::string("вход в Epic: ") + EOS_EResult_ToString(data->ResultCode);
        std::cerr << "EOS: " << self->m_error << std::endl;
        return;
    }
    // Достижения и статистика EOS привязаны к ProductUserId — вход в Connect по токену Epic
    EOS_HAuth auth = EOS_Platform_GetAuthInterface(self->m_platform);
    EOS_Auth_CopyUserAuthTokenOptions co{};
    co.ApiVersion = EOS_AUTH_COPYUSERAUTHTOKEN_API_LATEST;
    EOS_Auth_Token* token = nullptr;
    if (EOS_Auth_CopyUserAuthToken(auth, &co, data->LocalUserId, &token) != EOS_EResult::EOS_Success || !token) {
        self->m_error = "EOS_Auth_CopyUserAuthToken не удался";
        return;
    }
    EOS_Connect_Credentials cc{};
    cc.ApiVersion = EOS_CONNECT_CREDENTIALS_API_LATEST;
    cc.Type = EOS_EExternalCredentialType::EOS_ECT_EPIC;
    cc.Token = token->AccessToken;
    EOS_Connect_LoginOptions clo{};
    clo.ApiVersion = EOS_CONNECT_LOGIN_API_LATEST;
    clo.Credentials = &cc;
    EOS_Connect_Login(EOS_Platform_GetConnectInterface(self->m_platform), &clo, self, &EosPlatformServices::onConnectLogin);
    EOS_Auth_Token_Release(token);
}

void EOS_CALL EosPlatformServices::onConnectLogin(const EOS_Connect_LoginCallbackInfo* data) {
    auto* self = static_cast<EosPlatformServices*>(data->ClientData);
    if (data->ResultCode == EOS_EResult::EOS_Success) {
        self->loggedIn(data->LocalUserId);
    } else if (data->ResultCode == EOS_EResult::EOS_InvalidUser && data->ContinuanceToken) {
        // первый вход в эту игру — создать пользователя продукта
        EOS_Connect_CreateUserOptions cu{};
        cu.ApiVersion = EOS_CONNECT_CREATEUSER_API_LATEST;
        cu.ContinuanceToken = data->ContinuanceToken;
        EOS_Connect_CreateUser(EOS_Platform_GetConnectInterface(self->m_platform), &cu, self, &EosPlatformServices::onCreateUser);
    } else {
        self->m_error = std::string("EOS Connect: ") + EOS_EResult_ToString(data->ResultCode);
        std::cerr << "EOS: " << self->m_error << std::endl;
    }
}

void EOS_CALL EosPlatformServices::onCreateUser(const EOS_Connect_CreateUserCallbackInfo* data) {
    auto* self = static_cast<EosPlatformServices*>(data->ClientData);
    if (data->ResultCode == EOS_EResult::EOS_Success) self->loggedIn(data->LocalUserId);
    else self->m_error = std::string("EOS CreateUser: ") + EOS_EResult_ToString(data->ResultCode);
}

void EosPlatformServices::loggedIn(EOS_ProductUserId user) {
    m_user = user;
    m_error.clear();
    auto queue = std::move(m_queue);
    for (auto& action : queue) action();
}

void EosPlatformServices::run(std::function<void()> action) {
    if (!m_platform) return;
    if (m_user) action();
    else m_queue.push_back(std::move(action));
}

void EosPlatformServices::unlockAchievement(const std::string& apiName) {
    run([this, apiName] {
        const char* ids[] = {apiName.c_str()};
        EOS_Achievements_UnlockAchievementsOptions o{};
        o.ApiVersion = EOS_ACHIEVEMENTS_UNLOCKACHIEVEMENTS_API_LATEST;
        o.UserId = m_user;
        o.AchievementIds = ids;
        o.AchievementsCount = 1;
        EOS_Achievements_UnlockAchievements(EOS_Platform_GetAchievementsInterface(m_platform), &o, nullptr,
            [](const EOS_Achievements_OnUnlockAchievementsCompleteCallbackInfo* d) {
                if (d->ResultCode != EOS_EResult::EOS_Success) std::cerr << "EOS: достижение не открыто: " << EOS_EResult_ToString(d->ResultCode) << std::endl;
            });
    });
}

void EosPlatformServices::ingest(const std::string& stat, int32_t amount) {
    run([this, stat, amount] {
        EOS_Stats_IngestData d{};
        d.ApiVersion = EOS_STATS_INGESTDATA_API_LATEST;
        d.StatName = stat.c_str();
        d.IngestAmount = amount;
        EOS_Stats_IngestStatOptions o{};
        o.ApiVersion = EOS_STATS_INGESTSTAT_API_LATEST;
        o.LocalUserId = m_user;
        o.TargetUserId = m_user;
        o.Stats = &d;
        o.StatsCount = 1;
        EOS_Stats_IngestStat(EOS_Platform_GetStatsInterface(m_platform), &o, nullptr,
            [](const EOS_Stats_IngestStatCompleteCallbackInfo* r) {
                if (r->ResultCode != EOS_EResult::EOS_Success) std::cerr << "EOS: статистика не принята: " << EOS_EResult_ToString(r->ResultCode) << std::endl;
            });
    });
}

void EosPlatformServices::setStat(const std::string& apiName, double value) {
    // как статистика сводится (сумма, максимум, последнее) — задаётся в Dev Portal
    ingest(apiName, static_cast<int32_t>(value));
}

void EosPlatformServices::submitScore(const std::string& leaderboard, int64_t score) {
    ingest(leaderboard, static_cast<int32_t>(score));  // таблица EOS — по статистике с этим именем
}

void EosPlatformServices::tick() {
    if (m_platform) EOS_Platform_Tick(m_platform);
}

}  // namespace nlb::platform
