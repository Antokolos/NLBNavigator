set -euo pipefail
w=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
cd /workspace/NLBNavigator
source /workspace/nlb-developer-env-v1/activate.sh
export HOME="$w/unit-home" TMPDIR="$w/unit-tmp"
export XDG_CACHE_HOME="$HOME/.cache" XDG_CONFIG_HOME="$HOME/.config" XDG_DATA_HOME="$HOME/.local/share"

test "$(cat "$w/graphical-suite.exit")" = 0
test "$(git rev-parse HEAD)" = "$(cat "$w/tested-sha")"
bash "$w/check-pins.sh" "$w/pins-status-after" > "$w/pins-after"
cmp "$w/pins-before" "$w/pins-after"
git status --porcelain=v1 --untracked-files=all > "$w/worktree-after.log"
test ! -s "$w/worktree-after.log"
git diff --check origin/main...HEAD > "$w/diff-check.log" 2>&1
printf '0\n' > "$w/diff-check.exit"
python3 "$w/verify-c7.py" > "$w/verified-c7.log"
python3 "$w/results-summary.py" > "$w/routes-table.md"

cat "$w/report-intro.md" > "$w/REVIEW_REPORT.md"
jq -r '"Пары Wolfman — \(.routes)/22, контрольных точек — \(.checkpoints), сравнений текста — \(.text_pairs), проверок advance_until — \(.advance_pairs)."' "$w/verified-c7.json" >> "$w/REVIEW_REPORT.md"
printf '\n| Маршрут | Контрольные точки | Коды NLBNavigator / INSTEAD / сравнение |\n|---|---:|---|\n' >> "$w/REVIEW_REPORT.md"
cat "$w/routes-table.md" >> "$w/REVIEW_REPORT.md"

cat >> "$w/REVIEW_REPORT.md" <<'REPORT'

## Изоляция и воспроизводимость

Сборка, результаты, временные файлы и профили размещены в новом каталоге
под /workspace. Для каждого прогона созданы отдельные HOME, XDG, TMPDIR,
а для INSTEAD также собственные overlay и -appdata. Пути профилей во
всех парных прогонах проверены на принадлежность каталогам результатов.
Игровые репозитории использовались для чтения. SHA и чистота их рабочих
деревьев совпали до и после тестов. Рабочее дерево NLBNavigator тоже чистое.

Xvfb работал в режиме 1920×1080×24 с -noreset. Отдельная конфигурация
Openbox максимизирует окно без рамки. Использованы SDL_VIDEODRIVER=x11,
SDL_AUDIODRIVER=dummy, ограничения MAGICK_THREAD_LIMIT и OMP_NUM_THREADS
до 2 и PNG-сжатие без потерь. Штатный xvfb-run удалял только собственный
временный каталог авторизации, созданный внутри TMPDIR прогона.

[Команды](logs/commands.txt), [закреплённые SHA до](logs/pins-before),
[SHA после](logs/pins-after), [версии](logs/versions-runtime.log),
[параметры среды и метаданные](metadata.json), [контрольные суммы](SHA256SUMS.txt).
У больших файлов части .part-001, .part-002 и далее объединяются по
номеру. Границы строк и порядок не менялись.

Контекст для продолжения. Draft PR #73, issue #69 открыт, HEAD
8222b56b83d6a8ba6d2862c7e7f7f83bc62a6720. Сверить этот HEAD со штатным CI
и доказательствами по AGENTS.md. Решение о Ready и merge принимает Владелец.
REPORT
