#pragma once

#include "CoreMinimal.h"
#include "nlb/platform/PlatformServices.h"

/**
 * Площадка в Unreal — через OnlineSubsystem: достижения (IOnlineAchievements), статистика
 * (IOnlineStats), таблицы рекордов (IOnlineLeaderboards). Какая это площадка — Steam, EOS,
 * PlayStation, Switch, Xbox, — решает конфигурация проекта ([OnlineSubsystem] DefaultPlatformService
 * в DefaultEngine.ini и плагин OnlineSubsystem<Площадка>), поэтому консольные OSS подходят без
 * изменений в коде. Имя для переопределений в platform.ini — имя подсистемы в нижнем регистре:
 * steam, eos, ps5, switch, gdk…
 */
class FNLBOnlinePlatformServices : public nlb::platform::IPlatformServices {
public:
    explicit FNLBOnlinePlatformServices(int32 LocalUserNum = 0) : UserNum(LocalUserNum) {}

    std::string name() const override;
    bool available() const override;
    void unlockAchievement(const std::string& apiName) override;
    void setStat(const std::string& apiName, double value) override;
    void submitScore(const std::string& leaderboard, int64_t score) override;
    void flush() override;

private:
    int32 UserNum;
    bool bLeaderboardsDirty = false;
};
