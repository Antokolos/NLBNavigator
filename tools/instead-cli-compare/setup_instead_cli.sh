#!/bin/bash
# Сборка консольного INSTEAD (instead-cli, instead-hub) с Lua 5.1 и заглушками графики.
# Использование: ./setup_instead_cli.sh [каталог]   (по умолчанию ./instead-cli-build)
# Нужны: git, make, gcc, pkg-config, liblua5.1-0-dev (Debian/Ubuntu: apt install liblua5.1-0-dev lua5.1)
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
DEST="${1:-$PWD/instead-cli-build}"
mkdir -p "$DEST" && cd "$DEST"
[ -d instead-cli ] || git clone --depth 1 https://github.com/instead-hub/instead-cli.git
cd instead-cli
git submodule update --init --depth 1 instead
# по умолчанию Makefile собирает с luajit; переключаем на lua5.1
sed -i 's/pkg-config --cflags luajit/pkg-config --cflags lua5.1/; s/pkg-config --libs luajit/pkg-config --libs lua5.1/' Makefile
make -j4
make install || true   # метапарсер (другой подмодуль) не нужен, его ошибка не мешает
# tiny2/tiny3 — консольная прослойка STEAD; make install их не кладёт
cp tiny.lua release/stead/
cp instead/src/tiny/tiny2.lua release/stead/stead2/
cp instead/src/tiny/tiny3.lua release/stead/stead3/
T=release/stead/stead2/tiny2.lua
if ! grep -q "NLBNavigator: сверка с экспортом" "$T"; then
    cat "$HERE/tiny2_append.lua" >> "$T"
fi
# тема игры: экспорт NLB ищет тему по имени '.<имя>'
sed -i "s|instead_theme_name = function() return 'default' end|instead_theme_name = function() return '.default' end  -- тема игры: '.<имя>'|" "$T"
grep -q "return '.default' end" "$T" || echo "ВНИМАНИЕ: заглушка instead_theme_name в tiny2.lua не найдена — поправьте вручную, она должна возвращать '.default'"
echo "Готово: $DEST/instead-cli/release/instead-cli"
