#pragma once

#include <cstdint>
#include <string>

namespace nlb::platform {

/**
 * @brief Сервисы игровой площадки: достижения, статистика, таблицы рекордов.
 *
 * Один интерфейс на все площадки — Steam, Epic Games Store, а позже PlayStation, Nintendo Switch,
 * Xbox и т. п.: у каждой своя реализация (бэкенд), а книга о них не знает. Что именно отправлять,
 * решает PlatformBridge по файлу platform.ini (какое достижение книги — какое достижение площадки,
 * какая переменная — какая статистика или таблица рекордов).
 *
 * Все имена — «API-имена» площадки (как их завели в Steamworks, в Epic Dev Portal и т. д.).
 * Вызовы не блокируют: бэкенд может копить их, пока площадка не готова (вход игрока в EOS,
 * поиск таблицы в Steam), и отправлять в tick().
 */
class IPlatformServices {
public:
    virtual ~IPlatformServices() = default;

    /// Короткое имя площадки для platform.ini: steam, epic, log, ps5, switch, … (переопределения
    /// вида «ИМЯ@steam = …» выбираются по нему)
    virtual std::string name() const = 0;

    /// Площадка работает (SDK запущен, игрок вошёл). Недоступный бэкенд просто молчит
    virtual bool available() const = 0;

    /// Открыть достижение
    virtual void unlockAchievement(const std::string& apiName) = 0;

    /// Прогресс достижения (Steam: IndicateAchievementProgress; где нет — можно ничего не делать)
    virtual void setAchievementProgress(const std::string& apiName, int current, int max) {
        (void)apiName; (void)current; (void)max;
    }

    /// Статистика: текущее значение (целое — если без дробной части)
    virtual void setStat(const std::string& apiName, double value) = 0;

    /// Результат в таблицу рекордов (лучший результат площадка хранит сама)
    virtual void submitScore(const std::string& leaderboard, int64_t score) = 0;

    /// Сохранить накопленное (Steam: StoreStats) — после пачки изменений
    virtual void flush() {}

    /// Обработать ответы площадки (Steam: SteamAPI_RunCallbacks, EOS: EOS_Platform_Tick) —
    /// вызывается каждый кадр
    virtual void tick() {}
};

}  // namespace nlb::platform
