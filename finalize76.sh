set -euo pipefail
w=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
cd "$w/repo"
source /workspace/nlb-developer-env-v1/activate.sh
export HOME="$w/home" TMPDIR="$w/tmp" XDG_CONFIG_HOME="$w/home/config" XDG_CACHE_HOME="$w/home/cache" XDG_DATA_HOME="$w/home/data"

test "$(git rev-parse HEAD)" = "$(cat "$w/tested-sha")"
GIT_OPTIONAL_LOCKS=0 git status --porcelain=v1 --untracked-files=all > "$w/logs/worktree-after.log"
test ! -s "$w/logs/worktree-after.log"
git diff --check origin/main...HEAD > "$w/logs/diff-check-final.log"
printf '0\n' > "$w/logs/diff-check-final.exit"
bash "$w/check-pins.sh" "$w/pins-after" > "$w/logs/pins-after.txt"
cmp "$w/logs/pins-before.txt" "$w/logs/pins-after.txt"
python3 "$w/verify-c7.py" > "$w/logs/verify-c7.json"
python3 "$w/verify76.py" > "$w/route-table.txt"

cat "$w/REVIEW_INTRO.md" > "$w/REVIEW_REPORT.md"
jq -r '"\nМатрица D1–D5 завершилась с кодом 0. Семантических проверок в основной матрице — \(.matrix.full), в двух сценариях choices — \(.matrix.choices). [Основная матрица](matrix/full-semantic.json).\n\nПар Wolfman — \(.routes)/22, контрольных точек — \(.checkpoints)."' "$w/logs/totals.json" >> "$w/REVIEW_REPORT.md"
printf '\n| Маршрут | Точки | Коды SDL / INSTEAD / сравнение |\n|---|---:|---|\n' >> "$w/REVIEW_REPORT.md"
sed -n '/^|/p' "$w/route-table.txt" >> "$w/REVIEW_REPORT.md"
{
    printf '\n'
    if jq -e 'map(.attempts.results == [0,0,0]) | all' "$w/logs/routes.json" >/dev/null; then
        printf 'Все 22 пары прошли с первой попытки.\n'
    else
        printf 'Первые ошибки и повторы сохранены в таблице попыток.\n'
    fi
    printf '[Все фактические коды по попыткам](logs/routes.json).\n'
} >> "$w/REVIEW_REPORT.md"

cat >> "$w/REVIEW_REPORT.md" <<'REPORT'

## Условия проверки

Рабочее дерево создано отдельным checkout в NLB-Developer от проверенного
`origin/main`. Использованы собственные HOME/XDG/TMPDIR для процессов,
канонические пути и новые каталоги результатов. У INSTEAD отдельные
overlay и appdata. Сборка и прогоны использовали исходные закреплённые данные:
[до](logs/pins-before.txt) и [после](logs/pins-after.txt) совпали как SHA,
так и чистота рабочих деревьев. Игровые версии, UE, runner и CI сохранены.

Графика — Xvfb 1920×1080×24 `-noreset`, отдельный Openbox, X11 и фиктивное
аудиоустройство. Локальные Windows GUI-сценарии пропущены. Штатный
компаратор кадров используется дополнительно к строгому вердикту раннеров.

История попыток сохранена. В ранней фикстуре CTest был незафиксирован
порядок ссылок. Две временные спецификации маршрутов не прошли валидацию:
первая задавала одинаковый индекс порции в соседних шагах меню, вторая
ссылалась на уже непредыдущий кадр после вставки записи. Успешные
D1–D3 повторно проверены по исходным результатам, а продолжение матрицы
выполнено с исправленной временной спецификацией.
[Журнал первой фикстуры](logs/ctest-red-fixture.log) ·
[Валидация индекса](logs/matrix-red-validator.log) ·
[Валидация ссылки на кадр](logs/matrix-full-validator.log).

[Метаданные](metadata.json) · [Версии](logs/versions.log) ·
[Контрольные суммы](SHA256SUMS.txt). Номера .part-001, .part-002 и далее
обозначают части текстового файла. Объединять нужно в порядке номеров.

Handoff. Анализ, исправление и локальная регрессия #76 выполнены на
`3d312618b77f3ec62f97e9bdc7b56ed585ce2254`. Сопоставить этот SHA с Draft PR
и штатным CI. Пользоваться первичными доказательствами в отдельной ветке
по AGENTS.md. Решения о Ready, merge и закрытии issue принимает Владелец.
REPORT
python3 - "$w" <<'PY'
from pathlib import Path
import hashlib
import json
import sys

w = Path(sys.argv[1])
metadata = {
    "repository": "Antokolos/NLBNavigator", "issue": 76,
    "tested_head": (w / "tested-sha").read_text().strip(),
    "base": "aa5f15e7517b38295879eeb473e3bf04df7d1230",
    "red_evidence": (w / "red-evidence-sha").read_text().strip(),
    "pins": dict(line.split() for line in (w / "logs/pins-after.txt").read_text().splitlines()),
    "platform": {"os": "Debian 13.7 trixie amd64", "xvfb": "1920×1080×24 -noreset",
                 "sdl_videodriver": "x11", "sdl_audiodriver": "dummy",
                 "magick_thread_limit": 2, "omp_num_threads": 2, "lp_num_threads": 2,
                 "openbox": "3.6.1"},
    "versions": {"instead": "3.5.2", "sdl": "3.4.18", "sdl_image": "3.2.4", "sdl_ttf": "3.2.2",
                 "sdl_mixer": "3.2.4", "lua": "LuaJIT 2.1.0+openresty20250117-2",
                 "cmake": "3.31.6", "gcc": "14.2.0", "python": "3.12.14"},
    "sha256": {name: hashlib.sha256(path.read_bytes()).hexdigest() for name, path in {
        "nlb_sdl": w / "build/nlb_sdl", "runner": w / "repo/tests/differential/run_route.py"}.items()},
    "counts": json.loads((w / "logs/totals.json").read_text()),
}
(w / "metadata.json").write_text(json.dumps(metadata, ensure_ascii=False, indent=2) + "\n")
PY
