#!/bin/sh
# Исходники SDL3 и библиотек к нему для сборки под Android (android/deps).
# Те же версии, с которыми проверен nlb_sdl на ПК. Запускать из Git Bash или MSYS2 один раз
# (и после смены версий здесь).
set -e
cd "$(dirname "$0")"
mkdir -p deps
cd deps
fetch() {  # каталог тег
    if [ -d "$1/.git" ]; then
        echo "$1: уже есть"
    else
        git clone --depth 1 --branch "$2" "https://github.com/libsdl-org/$1.git" "$1"
    fi
}
fetch SDL release-3.4.18
fetch SDL_image release-3.4.8
fetch SDL_mixer release-3.2.4
fetch SDL_ttf release-3.2.2
# FreeType для SDL_ttf (встроенный, подмодуль)
git -C SDL_ttf submodule update --init --depth 1 external/freetype
echo "Готово: android/deps"
