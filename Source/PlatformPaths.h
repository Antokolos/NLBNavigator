#pragma once

#include <filesystem>

/**
 * Пути, зависящие от платформы. Вынесены из main.cpp в отдельную единицу трансляции:
 * windows.h конфликтует с именами cparse (INT, BOOL), которые builtin-features.inc
 * вносит в глобальное пространство через using namespace cparse.
 */

#include <string>
#include <vector>

/// Аргументы командной строки в UTF-8. В Windows argv приходит в ANSI-кодировке (CP1251 и т.п.),
/// поэтому аргументы берутся из UTF-16 командной строки — иначе кириллица в путях ломается.
std::vector<std::string> utf8Args(int argc, char** argv);

/// Домашний каталог пользователя; пустой путь, если не определён. В Windows — через _wgetenv:
/// getenv вернул бы USERPROFILE в ANSI-кодировке, и для имени пользователя на кириллице
/// std::filesystem::path бросал бы исключение (падение при старте).
std::filesystem::path homeDir();
