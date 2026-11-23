#!/usr/bin/env python3
"""Проверяет запись и восстановление VILLAIN_PUNISHED в Linux-версии nlb_sdl."""

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


def run_case(args, launcher, name, route, seed=None):
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
        "--timeout", "240",
    ]
    with log.open("w", encoding="utf-8") as stream:
        result = subprocess.run(command, cwd=REPO, env=environment, stdout=stream, stderr=subprocess.STDOUT)
    print(f"{name}: runner exit={result.returncode}, журнал {log}", flush=True)
    require(result.returncode == 0, f"{name}: ошибка выполнения маршрута, см. {log}")
    report = json.loads((output / "route-report.json").read_text(encoding="utf-8"))
    require(report["ok"] and not report["errors"], f"{name}: ошибки в route-report.json")
    storage = output / "nlbnavigator-home/.nlbnavigator" / filename
    rows = storage.read_text(encoding="utf-8").splitlines()
    return output, rows, report


def check_final_step(report, step_id, page_id):
    last = report["state_records"][-1]
    require(last["step_id"] == step_id and last["pageId"] == page_id,
            f"Ожидалась контрольная точка {step_id} ({page_id})")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for option in ("player", "book", "theme", "base-theme", "output"):
        parser.add_argument("--" + option, type=lambda value: Path(value).resolve(), required=True)
    args = parser.parse_args()

    route = load_route(ROUTE)
    probe_index = next(i for i, step in enumerate(route["steps"]) if step["id"] == PROBE_STEP)
    route["steps"] = route["steps"][:probe_index + 1]
    args.output.mkdir(parents=True, exist_ok=False)
    probe = args.output / "readback-route.json"
    probe.write_text(json.dumps(route, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    # Раннер создаёт новый HOME/XDG для каждого процесса. Подкладываем только целевые строки
    # из реального файла прогресса после создания HOME, до запуска настоящего nlb_sdl.
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

    award, rows, report = run_case(args, launcher, "award", ROUTE)
    check_final_step(report, AWARD_STEP, AWARD_PAGE)
    require(count_grants(award, AWARD) == 1, f"award: ожидался один вызов площадки для {AWARD}")
    target_rows = rows_for(rows, AWARD)
    require(target_rows == AWARD_ROWS, f"award: неожиданные записи {AWARD}: {target_rows}")
    print("award: подтверждены вызов площадки и строки id/granted", flush=True)

    seed = args.output / "readback-seed.tsv"
    seed.write_text("\n".join(row for row in rows if row in target_rows) + "\n", encoding="utf-8")
    for name, initial, expected in (("readback", seed, target_rows), ("clean", None, set())):
        output, rewritten, report = run_case(args, launcher, name, probe, initial)
        check_final_step(report, PROBE_STEP, PROBE_PAGE)
        with (output / "state-log.jsonl").open(encoding="utf-8") as trace:
            require(all(json.loads(line).get("pageId") != AWARD_PAGE for line in trace),
                    f"{name}: маршрут чтения не должен посещать страницу выдачи {AWARD}")
        require(count_grants(output, "RIGHT_CHOICE") == 1 and "granted\tRIGHT_CHOICE" in rewritten,
                f"{name}: ожидались вызов и новая запись RIGHT_CHOICE")
        require(count_grants(output, AWARD) == 0, f"{name}: преждевременная выдача {AWARD}")
        require(rows_for(rewritten, AWARD) == expected, f"{name}: неверное состояние {AWARD} после записи")
        print(f"{name}: RIGHT_CHOICE записано, строк {AWARD}: {len(expected)}", flush=True)


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, ValueError) as error:
        sys.exit(str(error))
