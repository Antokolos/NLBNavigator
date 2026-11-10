#!/bin/bash
# Копия экспорта INSTEAD со служебными командами (исходный экспорт не меняется).
# Использование: ./make_overlay.sh <каталог экспорта> <каталог копии> [rnd]
#   rnd — дописать ещё и детерминированную случайность (instead_rnd_append.lua)
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
SRC="$1"; DST="$2"
rm -rf "$DST" && cp -r "$SRC" "$DST" && rm -rf "$DST/.git"
cat "$HERE/instead_main_append.lua" >> "$DST/main.lua"
if [ "${3:-}" = "rnd" ]; then cat "$HERE/instead_rnd_append.lua" >> "$DST/main.lua"; fi
echo "Копия: $DST"
