#include "PlatformFactory.h"

#ifdef NLB_WITH_STEAMWORKS
#include "SteamPlatformServices.h"
#endif
#ifdef NLB_WITH_EOS
#include "EosPlatformServices.h"
#endif

namespace nlb::platform {

std::unique_ptr<IPlatformServices> createPlatformServices(const std::string& name, const PlatformConfig& config,
                                                         const std::vector<std::string>& args, std::string& error) {
    (void)config; (void)args;
    if (name == "log" || name.rfind("log:", 0) == 0) {
        return std::make_unique<LogPlatformServices>(name.size() > 4 ? name.substr(4) : "-");
    }
#ifdef NLB_WITH_STEAMWORKS
    if (name == "steam") {
        auto s = std::make_unique<SteamPlatformServices>(config);
        if (!s->available()) error = "Steam: " + s->lastError();
        return s;
    }
#endif
#ifdef NLB_WITH_EOS
    if (name == "epic") {
        auto s = std::make_unique<EosPlatformServices>(config, args);
        if (!s->lastError().empty()) error = "Epic: " + s->lastError();
        return s;
    }
#endif
    error = "площадка «" + name + "» не поддерживается этой сборкой";
    return nullptr;
}

std::vector<std::string> availablePlatforms() {
    std::vector<std::string> out{"log"};
#ifdef NLB_WITH_STEAMWORKS
    out.push_back("steam");
#endif
#ifdef NLB_WITH_EOS
    out.push_back("epic");
#endif
    return out;
}

}  // namespace nlb::platform
