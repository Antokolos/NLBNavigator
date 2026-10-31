#pragma once

#include <string>
#include <vector>

#include "nlb/presentation/ScreenModel.h"

namespace nlb::presentation {

/// ScreenModel в JSON — для тестов и диагностики (nlb_sdl --state-log; эталон модели экрана
/// Java-плеера NLB строится тем же форматом: имена полей — как в ScreenModel.h, без revision)
namespace json {
/// Строка JSON в кавычках с экранированием
std::string str(const std::string& s);
/// Абзацы текста: [{"runs":[{"kind","text","target"[,"image"]}],"joinPrevious"}]
std::string paragraphs(const std::vector<Paragraph>& ps);
/// Снимок экрана целиком (без порций VN — их считает GameScreen)
std::string screenModel(const ScreenModel& m);
}

}  // namespace nlb::presentation
