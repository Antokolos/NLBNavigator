#pragma once

#include <vector>

#include "nlb/domain/PlayerEngine.h"

/** События движка между UNLBScreenWidget и UNLBPlayerSubsystem (тип движка не виден в публичном заголовке) */
struct FNLBEngineEvents {
    const std::vector<PlayerEngine::Event>& Events;
};
