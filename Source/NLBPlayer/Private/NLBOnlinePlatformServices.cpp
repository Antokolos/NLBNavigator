#include "NLBOnlinePlatformServices.h"

#include "Interfaces/OnlineAchievementsInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlineLeaderboardInterface.h"
#include "Interfaces/OnlineStatsInterface.h"
#include "OnlineStats.h"
#include "OnlineSubsystem.h"

namespace {
FString F(const std::string& V) { return UTF8_TO_TCHAR(V.c_str()); }

IOnlineSubsystem* Oss() { return IOnlineSubsystem::Get(); }

FUniqueNetIdPtr LocalUser(int32 UserNum) {
    IOnlineSubsystem* S = Oss();
    IOnlineIdentityPtr Identity = S ? S->GetIdentityInterface() : nullptr;
    return Identity.IsValid() ? Identity->GetUniquePlayerId(UserNum) : nullptr;
}
}

std::string FNLBOnlinePlatformServices::name() const {
    IOnlineSubsystem* S = Oss();
    return S ? std::string(TCHAR_TO_UTF8(*S->GetSubsystemName().ToString().ToLower())) : std::string("none");
}

bool FNLBOnlinePlatformServices::available() const {
    return LocalUser(UserNum).IsValid();
}

void FNLBOnlinePlatformServices::unlockAchievement(const std::string& apiName) {
    IOnlineSubsystem* S = Oss();
    FUniqueNetIdPtr User = LocalUser(UserNum);
    IOnlineAchievementsPtr Achievements = S ? S->GetAchievementsInterface() : nullptr;
    if (!User.IsValid() || !Achievements.IsValid()) return;
    FOnlineAchievementsWriteRef Write = MakeShared<FOnlineAchievementsWrite, ESPMode::ThreadSafe>();
    // ключи статистик — FString (перегрузки с FName в UE 5.5+ устарели и дают неоднозначность)
    const FString Key = F(apiName);
    Write->SetFloatStat(Key, 100.0f);  // 100% — открыто
    Achievements->WriteAchievements(*User, Write);
}

void FNLBOnlinePlatformServices::setStat(const std::string& apiName, double value) {
    IOnlineSubsystem* S = Oss();
    FUniqueNetIdPtr User = LocalUser(UserNum);
    IOnlineStatsPtr Stats = S ? S->GetStatsInterface() : nullptr;
    if (!User.IsValid() || !Stats.IsValid()) return;
    TArray<FOnlineStatsUserUpdatedStats> Updates;
    FOnlineStatsUserUpdatedStats& Update = Updates.Emplace_GetRef(User.ToSharedRef());
    const FOnlineStatValue Value = FMath::Floor(value) == value ? FOnlineStatValue(static_cast<int32>(value)) : FOnlineStatValue(static_cast<float>(value));
    const FString Key = F(apiName);
    Update.Stats.Add(*Key, FOnlineStatUpdate(Value, FOnlineStatUpdate::EOnlineStatModificationType::Set));
    Stats->UpdateStats(User.ToSharedRef(), Updates, FOnlineStatsUpdateStatsComplete());
}

void FNLBOnlinePlatformServices::submitScore(const std::string& leaderboard, int64_t score) {
    IOnlineSubsystem* S = Oss();
    FUniqueNetIdPtr User = LocalUser(UserNum);
    IOnlineLeaderboardsPtr Leaderboards = S ? S->GetLeaderboardsInterface() : nullptr;
    if (!User.IsValid() || !Leaderboards.IsValid()) return;
    const FString Board = F(leaderboard);
    FOnlineLeaderboardWrite Write;
    Write.LeaderboardNames.Add(FString(Board));  // не *TCHAR: у обёртки массива есть и Add(FName)
    Write.RatedStat = FString(Board);
    Write.SortMethod = ELeaderboardSort::Descending;
    Write.UpdateMethod = ELeaderboardUpdateMethod::KeepBest;
    Write.SetIntStat(Board, static_cast<int32>(score));
    Leaderboards->WriteLeaderboards(NAME_GameSession, *User, Write);
    bLeaderboardsDirty = true;
}

void FNLBOnlinePlatformServices::flush() {
    if (!bLeaderboardsDirty) return;
    IOnlineSubsystem* S = Oss();
    IOnlineLeaderboardsPtr Leaderboards = S ? S->GetLeaderboardsInterface() : nullptr;
    if (Leaderboards.IsValid()) Leaderboards->FlushLeaderboards(NAME_GameSession);
    bLeaderboardsDirty = false;
}
