#pragma once

#include <filesystem>

/**
 * Пути, зависящие от платформы. Вынесены из main.cpp в отдельную единицу трансляции:
 * windows.h конфликтует с именами cparse (INT, BOOL), которые builtin-features.inc
 * вносит в глобальное пространство через using namespace cparse.
 */

/// Путь к каталогу книги (argv[1]). В Windows argv приходит в ANSI-кодировке (CP1251 и т.п.),
/// поэтому аргумент берётся из UTF-16 командной строки — иначе кириллица в пути ломается.
std::filesystem::path bookPathArg(const char* narrowArg);

/// Домашний каталог пользователя; пустой путь, если не определён. В Windows — через _wgetenv:
/// getenv вернул бы USERPROFILE в ANSI-кодировке, и для имени пользователя на кириллице
/// std::filesystem::path бросал бы исключение (падение при старте).
std::filesystem::path homeDir();
