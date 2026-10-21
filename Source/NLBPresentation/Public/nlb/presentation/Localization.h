#pragma once

#include <string>
#include <vector>

namespace nlb::presentation {

/**
 * @brief Язык игры и тексты интерфейса проигрывателя (меню, сообщения, подсказки).
 *
 * Язык книги — один из тех, что в ней есть (тексты страниц на языке «ru», «en»…). Какой включить,
 * решает chooseLanguage по списку предпочтений: выбор игрока в меню (сохраняется), язык лаунчера
 * (Steam, Epic Games Store), язык системы; если ни одного из них в книге нет — английский, а если
 * нет и его — язык книги по умолчанию.
 *
 * Интерфейс проигрывателя переведён на русский и английский; для остальных языков книги — по-
 * английски. Новый язык интерфейса — ещё один столбец в Localization.cpp.
 */
enum class Ui {
    Menu, SaveGame, SaveForbidden, LoadGame, Restart, SoundOff, SoundOn, SoundUnavailable,
    ChoicesScreen, ChoicesScreenVnOnly, ChoicesInText, FullscreenOff, FullscreenOn, Language,
    Quit, BackToGame, QuitQuestion, QuitYes, QuitNo, SaveToSlot, LoadFromSlot, Back, EmptySlot,
    GameOver, ExternalGameMissing, Failed, ItemSelected, Achievement, TextSize,
    Count_
};

/// Текст интерфейса на языке lang (нет перевода — английский)
std::string uiText(Ui key, const std::string& lang);

/// «ru_RU», «en-US», «RU» -> «ru» (язык без региона, строчными)
std::string normalizeLanguage(const std::string& code);

/// Название языка на нём самом: ru -> «Русский», en -> «English»; неизвестный — код
std::string languageName(const std::string& code);

/// Выбрать язык книги: первый из предпочтений, который в ней есть, иначе английский, иначе
/// bookDefault. Языки сравниваются без региона
std::string chooseLanguage(const std::vector<std::string>& available, const std::vector<std::string>& preferred,
                           const std::string& bookDefault);

/// Язык из названия, которое даёт Steam (GetCurrentGameLanguage: «russian», «schinese»…)
std::string languageFromSteam(const std::string& steamName);

/// Язык из аргументов командной строки: --lang xx, --lang=xx, -lang=xx, -culture=xx и
/// -epiclocale=xx (так язык передаёт Epic Games Store); пусто — не задан
std::string languageFromArgs(const std::vector<std::string>& args);

}  // namespace nlb::presentation
