#!/usr/bin/env python3
"""Проверяет сохранение и загрузку больничной ветки Wolfman в nlb_sdl."""

import argparse
import json
import os
import subprocess
import sys
from pathlib import Path

from run_route import load_route


HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
ROUTE = HERE / "routes/wolfman-police-hospital-doctor.json"
A = "hospital-page3"
B = "eat-doctor"
SLOT = Path("nlbnavigator-home/.nlbnavigator/wolfman.saves/slot1.sav")


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def write_route(output, name, route, steps):
    path = output / f"{name}.json"
    path.write_text(json.dumps(dict(route, id=f"wolfman-save-load-{name}", steps=steps),
                               ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    load_route(path)
    return path


def menu_step(step, name, nlb_keys, instead_key):
    return {
        "id": name, "screenshot": f"{name}.png", "settle_ms": 1000,
        "action": {"type": "menu", "nlbnavigator": nlb_keys, "instead": [{"key": instead_key}]},
        "expect": dict(step["expect"]),
    }


def replay_steps(steps, load_step_id):
    result = []
    previous = load_step_id
    for step in steps:
        replay = dict(step, id="replay-" + step["id"], screenshot="replay-" + step["screenshot"])
        if "visual_change_from" in step:
            replay["visual_change_from"] = previous
        result.append(replay)
        previous = replay["id"]
    return result


def run_case(args, launcher, route, seed=None, empty=False):
    output = args.output / route.stem
    env = dict(os.environ, NLB_SAVE_LOAD_PLAYER=str(args.player), NLB_SAVE_LOAD_SEED=str(seed or ""))
    command = [
        sys.executable, str(HERE / "run_route.py"), "--runtime", "nlbnavigator",
        "--route", str(route), "--output", str(output), "--player", str(launcher),
        "--book", str(args.book), "--theme", str(args.theme), "--base-theme", str(args.base_theme),
    ]
    if sys.platform == "win32" and seed:
        command += ["--seed-home", ".nlbnavigator/wolfman.saves/slot1.sav=" + str(seed)]
    log = args.output / f"{route.stem}.log"
    with log.open("w", encoding="utf-8") as stream:
        result = subprocess.run(command, cwd=REPO, env=env, stdout=stream, stderr=subprocess.STDOUT)
    print(f"{route.stem}: runner exit={result.returncode}, журнал {log}", flush=True)
    if empty:
        require(result.returncode == 1, "Пустой слот должен отклонить menu:1")
        states = [json.loads(line) for line in (output / "state-log.jsonl").read_text(encoding="utf-8").splitlines()]
        commands = [(item["command"], item["ok"]) for item in states]
        require(commands == [("", True), ("key:f9", True), ("menu:1", False),
                             ("settle:1000", True), (f"shot:{output / 'empty-slot.png'}", True)],
                "В пустом профиле отказ должен произойти именно на menu:1")
        require("[НЕ ВЫПОЛНЕНО] menu:1" in (output / "runner.log").read_text(encoding="utf-8"),
                "Плеер не подтвердил отказ menu:1")
        return output, states
    require(result.returncode == 0, f"{route.stem}: ошибка маршрута, см. {log}")
    report = json.loads((output / "route-report.json").read_text(encoding="utf-8"))
    require(report["ok"] and not report["errors"], f"{route.stem}: ошибки в route-report.json")
    return output, {record["step_id"]: record for record in report["state_records"]}


def snapshot(record):
    return {key: record[key] for key in ("pageId", "screen", "finished", "portion", "variables", "model")}


def same_state(actual, expected, label):
    actual, expected = snapshot(actual), snapshot(expected)
    changed = [key for key in expected if actual[key] != expected[key]]
    if "model" in changed:
        changed.extend("model." + key for key in expected["model"]
                       if actual["model"].get(key) != expected["model"][key])
    require(not changed, f"{label}: состояние отличается от baseline: {', '.join(changed)}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for option in ("player", "book", "theme", "base-theme", "output"):
        parser.add_argument("--" + option, type=lambda value: Path(value).resolve(), required=True)
    args = parser.parse_args()
    require(args.book.name == "wolfman", "Каталог закреплённой книги должен называться wolfman")
    args.output.mkdir(parents=True, exist_ok=False)

    route = load_route(ROUTE)
    steps = route["steps"]
    start = steps[0]
    a = next(index for index, step in enumerate(steps) if step["id"] == A)
    b = next(index for index, step in enumerate(steps) if step["id"] == B)
    prefix, continuation = steps[:a + 1], steps[a + 1:b + 1]
    save = menu_step(steps[a], "save-a", ["f5", 1], "F8")
    for key in ("portion_index", "portion_count", "instead_portion_index", "instead_portion_count"):
        save["expect"].pop(key, None)
    load = menu_step(steps[a], "load-a", ["f9", 1], "F9")
    replay = replay_steps(continuation, load["id"])
    baseline = write_route(args.output, "baseline", route, prefix + continuation)
    after_save = dict(continuation[0], visual_change_from=save["id"])
    roundtrip = write_route(args.output, "roundtrip", route,
                            prefix + [save, after_save] + continuation[1:] + [load] + replay)
    save_only = write_route(args.output, "save-only", route, prefix + [save])
    restart = write_route(args.output, "restart", route, [start, load] + replay)
    empty = write_route(args.output, "empty", route, [menu_step(start, "empty-slot", ["f9", 1], "F9")])

    if sys.platform == "win32":
        launcher = args.player
    else:
        launcher = args.output / "nlb-sdl-with-save"
        launcher.write_text(
            """#!/bin/sh
set -eu
slot="$HOME/.nlbnavigator/wolfman.saves/slot1.sav"
test ! -e "$slot"
if [ -n "$NLB_SAVE_LOAD_SEED" ]; then
    mkdir -p "$(dirname "$slot")"
    cp "$NLB_SAVE_LOAD_SEED" "$slot"
fi
exec "$NLB_SAVE_LOAD_PLAYER" "$@"
""", encoding="utf-8",
        )
        launcher.chmod(0o700)

    output, reference = run_case(args, launcher, baseline)
    require(not (output / SLOT).exists(), "Baseline записал сохранение")
    require(snapshot(reference[A]) != snapshot(reference[B]), "A и B совпали")
    require(reference[A]["pageId"] != reference["title"]["pageId"], "A совпала с титулом")

    output, same = run_case(args, launcher, roundtrip)
    require((output / SLOT).is_file(), "Нет файла слота в roundtrip")
    for current, initial in ((A, A), ("save-a", A), (B, B), ("load-a", A)):
        same_state(same[current], reference[initial], "roundtrip/" + current)
    for step in continuation:
        same_state(same["replay-" + step["id"]], reference[step["id"]], "roundtrip/replay-" + step["id"])

    output, first = run_case(args, launcher, save_only)
    same_state(first["save-a"], reference[A], "save-only/save-a")
    seed = output / SLOT
    require(seed.is_file(), "После завершения процесса нет файла слота")
    restarted_output, second = run_case(args, launcher, restart, seed)
    require((restarted_output / SLOT).read_bytes() == seed.read_bytes(), "Загрузка изменила файл слота")
    same_state(second["title"], reference["title"], "restart/title")
    same_state(second["load-a"], reference[A], "restart/load-a")
    for step in continuation:
        same_state(second["replay-" + step["id"]], reference[step["id"]], "restart/replay-" + step["id"])

    output, rejected = run_case(args, launcher, empty, empty=True)
    require(not (output / SLOT).exists(), "В пустом профиле появился файл слота")
    same_state(rejected[0], reference["title"], "empty/start")
    same_state(rejected[-1], reference["title"], "empty/load")
    print("OK: A и продолжение восстановлены в том же и в новом процессе; пустой слот недоступен")


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, ValueError) as error:
        sys.exit(str(error))
