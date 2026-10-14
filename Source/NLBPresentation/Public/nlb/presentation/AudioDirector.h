#pragma once

#include <string>
#include <vector>

#include "nlb/presentation/AudioBackend.h"
#include "nlb/domain/PlayerEngine.h"

namespace nlb::presentation {

/**
 * @brief Логика звука INSTEAD-экспорта поверх любого IAudioBackend. ЧЕРНОВИК ДЛЯ СОГЛАСОВАНИЯ.
 *
 *  - Music с той же дорожкой, что уже играет, — ничего не делать (переход по страницам
 *    с одной музыкой её не перезапускает);
 *  - Music с пустым именем (VOID в книге) — остановить музыку;
 *  - Sound — эффект поверх;
 *  - suppmed/suppsou книги и настройка «без звука» — ничего не играть;
 *  - после загрузки сохранения — играть музыку, сохранённую в нём.
 */
class AudioDirector {
public:
    explicit AudioDirector(IAudioBackend& backend);
    /// События движка после команды игрока или тика времени
    void apply(const std::vector<PlayerEngine::Event>& events);
    void setMuted(bool muted);
    const std::string& currentMusic() const { return m_currentMusic; }

private:
    IAudioBackend& m_backend;
    std::string m_currentMusic;
    bool m_muted = false;
};

}  // namespace nlb::presentation
