#!/usr/bin/env python3
"""Проверяет сохранение VILLAIN_PUNISHED и подавление повторной выдачи в nlb_sdl.

Сценарии (последовательно, каждый — новый процесс и свой HOME/XDG, который создаёт
run_route.py):
  award     — полный маршрут до страницы 42: выдача достижения, строки id/granted в хранилище,
              ровно один вызов площадки;
  readback  — новый профиль только с этими строками, маршрут до page35b: новое достижение
              RIGHT_CHOICE записывается, строки VILLAIN_PUNISHED сохраняются, страница выдачи
              не посещается;
  clean     — тот же префикс с пустым профилем: строк VILLAIN_PUNISHED нет;
  reaward   — полный маршрут с готовыми строками: повторной выдачи и вызова площадки нет.

Рядом с отчётами остаётся achievement-summary.json — машинный итог по сценариям для разбора
артефактов CI (пишется после каждого сценария, поэтому падение тоже видно в файле), и
achievements.tsv — копия хранилища достижений (сам файл `wolfman.achievements` без расширения
в компактные артефакты CI не попадает).
"""

import argparse
import json
import os
import subprocess
import sys
from pathlib import Path

from run_route import load_route


HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
ROUTE = HERE / "routes/wolfman-guard-low-wolfiness-escape.json"
AWARD_STEP = "page40-to-page42-villain-punished"
AWARD_PAGE = "c81126f6-4646-42b3-af03-aae3d6a9a0c2"
PROBE_STEP = "page33-to-page35b"
PROBE_PAGE = "6e8c1c0f-abd8-4e5c-83a5-7e3e003c7f46"
AWARD = "VILLAIN_PUNISHED"
AWARD_ROWS = {
    f"id\t{AWARD}\tec437b19-13ab-4f4a-b851-185849a51249",
    f"granted\t{AWARD}",
}


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def rows_for(rows, name):
    return {row for row in rows if row.split("\t")[1:2] == [name]}


def count_grants(output, name):
    return (output / "platform.log").read_text(encoding="utf-8").splitlines().count(
        f"[log] achievement {name}"
    )


def default_case_timeout(steps):
    """Таймаут одного прогона, как nlb_timeout в select_routes.py (90 + 5 шагов), но не меньше 240 с."""
    return max(240, 90 + 5 * steps)


def run_case(args, launcher, name, route, seed=None, timeout=0):
    output = args.output / name
    log = args.output / f"{name}.log"
    filename = args.book.name + ".achievements"
    environment = os.environ.copy()
    environment.update(
        NLB_ACHIEVEMENT_PLAYER=str(args.player),
        NLB_ACHIEVEMENT_PLATFORM_LOG=str(output / "platform.log"),
        NLB_ACHIEVEMENT_FILENAME=filename,
        NLB_ACHIEVEMENT_SEED=str(seed) if seed else "",
    )
    command = [
        sys.executable, str(HERE / "run_route.py"), "--runtime", "nlbnavigator",
        "--player", str(launcher), "--route", str(route), "--output", str(output),
        "--book", str(args.book), "--theme", str(args.theme), "--base-theme", str(args.base_theme),
        "--timeout", str(timeout or default_case_timeout(len(route["steps"]))),
    ]
    if sys.platform == "win32":
        # POSIX-лаунчер на Windows не исполняется: платформенный лог и засев хранилища передаются
        # раннеру напрямую (--player-arg/--seed-home), он сам изолирует USERPROFILE.
        command += ["--player-arg=--platform",
                    "--player-arg=log:" + str(output / "platform.log")]
        if seed:
            command += ["--seed-home", ".nlbnavigator/{0}={1}".format(filename, seed)]
    with log.open("w", encoding="utf-8") as stream:
        result = subprocess.run(command, cwd=REPO, env=environment, stdout=stream, stderr=subprocess.STDOUT)
    print(f"{name}: runner exit={result.returncode}, журнал {log}", flush=True)
    require(result.returncode == 0, f"{name}: ошибка выполнения маршрута, см. {log}")
    report = json.loads((output / "route-report.json").read_text(encoding="utf-8"))
    require(report["ok"] and not report["errors"], f"{name}: ошибки в route-report.json")
    storage = output / "nlbnavigator-home/.nlbnavigator" / filename
    rows = storage.read_text(encoding="utf-8").splitlines()
    # Копия для артефактов CI: у .achievements расширение не текстовое, и компактный шаг его
    # пропускает; .tsv попадает всегда.
    (output / "achievements.tsv").write_text("\n".join(rows) + "\n", encoding="utf-8")
    return output, rows, report


def check_final_step(report, step_id, page_id):
    last = report["state_records"][-1]
    require(last["step_id"] == step_id and last["pageId"] == page_id,
            f"Ожидалась контрольная точка {step_id} ({page_id})")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for option in ("player", "book", "theme", "base-theme", "output"):
        parser.add_argument("--" + option, type=lambda value: Path(value).resolve(), required=True)
    parser.add_argument("--case-timeout", type=int, default=0,
                        help="таймаут одного прогона, с (по умолчанию 90 + 5 шагов, не меньше 240)")
    args = parser.parse_args()

    route = load_route(ROUTE)
    full_steps = len(route["steps"])
    probe_index = next(i for i, step in enumerate(route["steps"]) if step["id"] == PROBE_STEP)
    route["steps"] = route["steps"][:probe_index + 1]
    probe_steps = len(route["steps"])
    args.output.mkdir(parents=True, exist_ok=False)
    probe = args.output / "readback-route.json"
    probe.write_text(json.dumps(route, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    # Итог по сценариям для артефактов CI: пишется до и после каждого сценария, поэтому при
    # падении в файле остаётся начатый сценарий с ok=false, а не пустота.
    summary_path = args.output / "achievement-summary.json"
    summary = {"schema": 1, "award": AWARD, "cases": []}

    def write_summary():
        summary_path.write_text(json.dumps(summary, ensure_ascii=False, indent=2) + "\n",
                                encoding="utf-8")

    def start(name):
        summary["cases"].append({"name": name, "ok": False})
        write_summary()

    def finish():
        summary["cases"][-1]["ok"] = True
        write_summary()

    write_summary()

    # Раннер создаёт новый HOME/XDG для каждого процесса. Подкладываем только целевые строки
    # из реального файла прогресса после создания HOME, до запуска настоящего nlb_sdl.
    if sys.platform == "win32":
        launcher = args.player
    else:
        launcher = args.output / "nlb-sdl-with-platform-log"
        launcher.write_text(
            """#!/bin/sh
set -eu
store="$HOME/.nlbnavigator/$NLB_ACHIEVEMENT_FILENAME"
test ! -e "$store"
if [ -n "$NLB_ACHIEVEMENT_SEED" ]; then
    mkdir -p "$(dirname "$store")"
    cp "$NLB_ACHIEVEMENT_SEED" "$store"
fi
exec "$NLB_ACHIEVEMENT_PLAYER" "$@" --platform "log:$NLB_ACHIEVEMENT_PLATFORM_LOG"
""", encoding="utf-8",
        )
        launcher.chmod(0o700)
    award_timeout = args.case_timeout or default_case_timeout(full_steps)
    probe_timeout = args.case_timeout or default_case_timeout(probe_steps)

    start("award")
    award, rows, report = run_case(args, launcher, "award", ROUTE, timeout=award_timeout)
    check_final_step(report, AWARD_STEP, AWARD_PAGE)
    require(count_grants(award, AWARD) == 1, f"award: ожидался один вызов площадки для {AWARD}")
    target_rows = rows_for(rows, AWARD)
    require(target_rows == AWARD_ROWS, f"award: неожиданные записи {AWARD}: {target_rows}")
    finish()
    print("award: подтверждены вызов площадки и строки id/granted", flush=True)

    seed = args.output / "readback-seed.tsv"
    seed.write_text("\n".join(row for row in rows if row in target_rows) + "\n", encoding="utf-8")
    for name, initial, expected in (("readback", seed, target_rows), ("clean", None, set())):
        start(name)
        output, rewritten, report = run_case(args, launcher, name, probe, initial, timeout=probe_timeout)
        check_final_step(report, PROBE_STEP, PROBE_PAGE)
        with (output / "state-log.jsonl").open(encoding="utf-8") as trace:
            require(all(json.loads(line).get("pageId") != AWARD_PAGE for line in trace),
                    f"{name}: маршрут чтения не должен посещать страницу выдачи {AWARD}")
        require(count_grants(output, "RIGHT_CHOICE") == 1 and "granted\tRIGHT_CHOICE" in rewritten,
                f"{name}: ожидались вызов и новая запись RIGHT_CHOICE")
        require(count_grants(output, AWARD) == 0, f"{name}: преждевременная выдача {AWARD}")
        require(rows_for(rewritten, AWARD) == expected, f"{name}: неверное состояние {AWARD} после записи")
        finish()
        print(f"{name}: RIGHT_CHOICE записано, строк {AWARD}: {len(expected)}", flush=True)

    start("reaward")
    output, rewritten, report = run_case(args, launcher, "reaward", ROUTE, seed, timeout=award_timeout)
    check_final_step(report, AWARD_STEP, AWARD_PAGE)
    require(count_grants(output, "RIGHT_CHOICE") == 1 and "granted\tRIGHT_CHOICE" in rewritten,
            "reaward: ожидались вызов и новая запись RIGHT_CHOICE")
    require(count_grants(output, AWARD) == 0, "reaward: повторный вызов площадки для VILLAIN_PUNISHED")
    require(rows_for(rewritten, AWARD) == target_rows, "reaward: неверное состояние VILLAIN_PUNISHED после страницы 42")
    finish()
    print("reaward: обе строки VILLAIN_PUNISHED сохранены, повторных вызовов площадки нет", flush=True)


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, ValueError) as error:
        sys.exit(str(error))
