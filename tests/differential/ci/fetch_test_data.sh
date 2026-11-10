#!/bin/bash
# Закреплённые тестовые данные в test-data/: книга wolfman с вложенными модулями, её экспорт INSTEAD
# и базовая тема INSTEAD. Ссылки — из окружения (GAME_REF, REFERENCE_REF, BASE_THEME_REF), как в
# workflow nlb-sdl-test.yml. Уже скачанный каталог (кэш CI) пропускается, если он на нужном коммите.
set -euo pipefail
: "${GAME_REF:?}" "${REFERENCE_REF:?}" "${BASE_THEME_REF:?}"
mkdir -p test-data

at_ref() {  # каталог $1 — git-клон на коммите $2 (полный SHA или его начало)
    [ -d "$1/.git" ] && case "$(git -C "$1" rev-parse HEAD)" in "$2"*) return 0;; esac
    return 1
}

if ! at_ref test-data/wolfman "$GAME_REF"; then
    rm -rf test-data/wolfman
    git clone --no-checkout --filter=blob:none https://github.com/nlbproject/wolfman.git test-data/wolfman
    git -C test-data/wolfman fetch --depth 1 origin "$GAME_REF"
    git -C test-data/wolfman checkout --detach FETCH_HEAD
    git -C test-data/wolfman submodule update --init --recursive
fi
git -C test-data/wolfman log -1 --format='NLB book commit: %H %s'
test -s test-data/wolfman/images/title.png
test -s test-data/wolfman/images/1.png

if ! at_ref test-data/instead-am_i_a_monster_vn "$REFERENCE_REF"; then
    rm -rf test-data/instead-am_i_a_monster_vn
    git clone --no-checkout --filter=blob:none \
        https://github.com/nlbproject/instead-am_i_a_monster_vn.git test-data/instead-am_i_a_monster_vn
    git -C test-data/instead-am_i_a_monster_vn fetch --depth 1 origin "$REFERENCE_REF"
    git -C test-data/instead-am_i_a_monster_vn checkout --detach FETCH_HEAD
    git -C test-data/instead-am_i_a_monster_vn submodule update --init --recursive
fi
git -C test-data/instead-am_i_a_monster_vn log -1 --format='INSTEAD reference commit: %H %s'
test -s test-data/instead-am_i_a_monster_vn/luapassing.lua
test -s test-data/instead-am_i_a_monster_vn/main.lua
test -d test-data/instead-am_i_a_monster_vn/images
test -d test-data/instead-am_i_a_monster_vn/themes

if ! at_ref test-data/instead "$BASE_THEME_REF"; then
    rm -rf test-data/instead
    git clone --no-checkout --filter=blob:none https://github.com/instead-hub/instead.git test-data/instead
    git -C test-data/instead sparse-checkout init --cone
    git -C test-data/instead sparse-checkout set themes/default
    git -C test-data/instead fetch --depth 1 origin "$BASE_THEME_REF"
    git -C test-data/instead checkout --detach FETCH_HEAD
fi
git -C test-data/instead log -1 --format='INSTEAD base theme commit: %H %s'
test -s test-data/instead/themes/default/theme.ini
