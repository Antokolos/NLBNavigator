#pragma once

#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include "nlb/domain/PlayerEngine.h"
#include "nlb/platform/PlatformServices.h"

namespace nlb::platform {

/**
 * @brief Настройки площадок: platform.ini (рядом с книгой — попадает и в пакет .nlbpack).
 *
 * @code
 * ; достижения: имя в книге (ACHIEVE) = API-имя площадки. Без строки — то же имя, что в книге
 * [achievements]
 * COLLECTOR = ACH_COLLECTOR
 * COLLECTOR@epic = collector          ; своё имя для одной площадки
 *
 * ; статистика: переменная книги = API-имя статистики (отправляется при изменении)
 * [stats]
 * money = STAT_MONEY
 *
 * ; таблицы рекордов: переменная = таблица; when = finish (по умолчанию, в конце игры) или change
 * [leaderboards]
 * score = LB_SCORE
 * score.when = change
 *
 * ; параметры площадок (бэкенд читает свои)
 * [steam]
 * app_id = 480
 * [epic]
 * product_id = …
 * @endcode
 */
class PlatformConfig {
public:
    static PlatformConfig load(const std::string& path);   ///< нет файла — пустые настройки
    static PlatformConfig parse(const std::string& text);

    struct Board { std::string variable; std::string apiName; bool onChange = false; };

    /// API-имя достижения книги для площадки (переопределение «ИМЯ@площадка», иначе общее,
    /// иначе — само имя из книги)
    std::string achievement(const std::string& platform, const std::string& nlbName) const;
    /// Статистики: переменная -> API-имя для площадки
    std::vector<std::pair<std::string, std::string>> stats(const std::string& platform) const;
    std::vector<Board> leaderboards(const std::string& platform) const;
    /// Параметр раздела площадки ([steam] app_id …)
    std::string value(const std::string& section, const std::string& key, const std::string& def = "") const;

private:
    std::string resolve(const std::string& section, const std::string& platform, const std::string& key) const;
    std::map<std::string, std::map<std::string, std::string>> m_sections;
};

/**
 * @brief Мост книги к площадкам: события движка и переменные -> вызовы IPlatformServices.
 *
 * Достижение книги (ACHIEVE, ACHMAX — событие Achievement) открывается на всех площадках;
 * переменные из [stats] отправляются, когда меняются; переменные из [leaderboards] — в таблицы
 * рекордов в конце игры или при изменении. Площадок может быть несколько сразу (например, в
 * сборке, которая сама определяет, откуда запущена).
 */
class PlatformBridge {
public:
    explicit PlatformBridge(PlatformConfig config) : m_config(std::move(config)) {}

    void add(std::unique_ptr<IPlatformServices> services);
    bool empty() const { return m_services.empty(); }
    const std::vector<std::unique_ptr<IPlatformServices>>& services() const { return m_services; }

    /// После команды: события (достижения) и состояние книги (статистика, рекорды, конец игры)
    void onCommand(const std::vector<PlayerEngine::Event>& events, const PlayerEngine& engine, bool finished);
    /// Каждый кадр: ответы площадок
    void tick();
    /// Новая игра: рекорды «в конце игры» можно отправить снова, значения перечитываются
    void reset();

private:
    std::optional<double> number(const PlayerEngine& engine, const std::string& variable) const;

    PlatformConfig m_config;
    std::vector<std::unique_ptr<IPlatformServices>> m_services;
    std::map<std::string, double> m_sent;  ///< «площадка|переменная» -> последнее отправленное
    bool m_finishSent = false;
};

/// Площадка-журнал: пишет вызовы в поток или файл. Для отладки platform.ini без Steam/EOS
/// и как образец бэкенда
class LogPlatformServices : public IPlatformServices {
public:
    explicit LogPlatformServices(const std::string& path, std::string platformName = "log");
    std::string name() const override { return m_name; }
    bool available() const override { return static_cast<bool>(m_out); }
    void unlockAchievement(const std::string& apiName) override;
    void setAchievementProgress(const std::string& apiName, int current, int max) override;
    void setStat(const std::string& apiName, double value) override;
    void submitScore(const std::string& leaderboard, int64_t score) override;
    void flush() override;

private:
    std::unique_ptr<std::ostream> m_out;
    std::string m_name;
};

}  // namespace nlb::platform
