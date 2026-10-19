#pragma once

#include <memory>
#include <string>
#include <vector>

#include "nlb/platform/PlatformBridge.h"

namespace nlb::platform {

/**
 * @brief Создать бэкенд площадки по имени (для nlb_sdl и Godot; в Unreal — OnlineSubsystem).
 *
 * - «log» или «log:<файл>» — журнал вызовов (отладка platform.ini), всегда есть;
 * - «steam» — Steamworks (если собрано с STEAMWORKS_SDK_DIR);
 * - «epic» — Epic Online Services (если собрано с EOS_SDK_DIR).
 *
 * Новая площадка (PlayStation, Switch, Xbox…) — новый класс IPlatformServices и строка здесь;
 * книга, мост и platform.ini не меняются.
 *
 * @param args аргументы командной строки (EOS берёт из них код входа от Epic Games Launcher)
 */
std::unique_ptr<IPlatformServices> createPlatformServices(const std::string& name, const PlatformConfig& config,
                                                         const std::vector<std::string>& args, std::string& error);

/// Какие площадки есть в этой сборке (для справки в --help)
std::vector<std::string> availablePlatforms();

}  // namespace nlb::platform
