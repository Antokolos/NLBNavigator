#!/usr/bin/env python3
"""Регрессия #68: сохранение/загрузка позиции VN-порции в nlb_sdl (Wolfman).

Сценарии: первая порция, вторая (с переходом на другую страницу и возвратом), перезапуск
процесса, промежуточная и последняя порции длинной страницы, продолжение после загрузки,
совместимость со старым сохранением без записей `ui`.

Сравниваются pageId, screen, finished, variables, portion (индекс, число, видимый текст) и
ScreenModel — не только страница и переменные. Запуск — вручную (как
check_wolfman_save_load.py), под Xvfb; сценарий A1 (воспроизведение до исправления) —
в evidence-ветке PR.
"""

import argparse
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

from run_route import load_route

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
ROUTE = HERE / "routes/wolfman-police-hospital-doctor.json"
A_PAGE = "8c1fa8c6-8027-43c8-8d8b-0bf454d5e717"
B_PAGE = "d937c5c1-766c-4581-a03e-736560b0cc8f"
SLOT = Path("nlbnavigator-home/.nlbnavigator/wolfman.saves/slot1.sav")


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def menu(name, nlb, expect, settle=1500):
    return {"id": name, "screenshot": f"{name}.png", "settle_ms": settle,
            "action": {"type": "menu", "nlbnavigator": nlb, "instead": []}, "expect": expect}


def save(name="save-a", expect=None):
    return menu(name, ["f5", 1], expect or {}, settle=1200)


def load(name="load-a", expect=None):
    return menu(name, ["f9", 1], expect or {}, settle=2500)


def renamed(step, name, expect=None):
    copy = dict(step, id=name, screenshot=f"{name}.png")
    if expect is not None:
        copy["expect"] = expect
    else:
        copy.pop("expect", None)
    copy.pop("visual_change_from", None)
    return copy


def write_route(output, name, route, steps):
    path = output / f"{name}.json"
    path.write_text(json.dumps(dict(route, id=f"wolfman-save-portion-{name}", steps=steps),
                               ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    load_route(path)
    return path


def run_case(args, launcher, route, seed=None, env_extra=None):
    output = args.output / route.stem
    env = dict(os.environ, NLB_SAVE_LOAD_PLAYER=str(args.player),
               NLB_SAVE_LOAD_SEED=str(seed or ""))
    if env_extra:
        env.update(env_extra)
    command = [
        sys.executable, str(HERE / "run_route.py"), "--runtime", "nlbnavigator",
        "--route", str(route), "--output", str(output), "--player", str(launcher),
        "--book", str(args.book), "--theme", str(args.theme), "--base-theme", str(args.base_theme),
    ]
    log = args.output / f"{route.stem}.log"
    with log.open("w", encoding="utf-8") as stream:
        result = subprocess.run(command, cwd=REPO, env=env, stdout=stream, stderr=subprocess.STDOUT)
    print(f"{route.stem}: runner exit={result.returncode}, журнал {log}", flush=True)
    report = json.loads((output / "route-report.json").read_text(encoding="utf-8")) \
        if (output / "route-report.json").is_file() else {}
    records = {record["step_id"]: record for record in report.get("state_records", [])}
    return result.returncode, records, report.get("errors", [])


def snapshot(record):
    return {key: record.get(key) for key in
            ("pageId", "screen", "finished", "portion", "variables", "model")}


def same(actual, expected, label):
    actual, expected = snapshot(actual), snapshot(expected)
    changed = [key for key in expected if actual.get(key) != expected.get(key)]
    require(not changed, f"{label}: отличие от baseline: {', '.join(changed)}")


def portion(record):
    return (record or {}).get("portion", {}).get("index")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for option in ("player", "book", "theme", "base-theme", "output"):
        parser.add_argument("--" + option, type=lambda value: Path(value).resolve(), required=True)
    args = parser.parse_args()
    require(args.book.name == "wolfman", "Каталог закреплённой книги должен называться wolfman")
    args.output.mkdir(parents=True, exist_ok=False)

    route = load_route(ROUTE)
    steps = route["steps"]
    ids = [step["id"] for step in steps]
    a = ids.index("hospital-page3")
    b = ids.index("eat-doctor")
    prefix = steps[:a + 1]
    p1 = renamed(steps[a + 1], "portion-1")                     # page3: 0 -> 1 (последняя из 2)
    choices = renamed(steps[a + 2], "choices")
    eat = renamed(steps[b], "eat-doctor")
    page4 = [renamed(step, f"page4-{index}") for index, step in enumerate(steps[b + 1:b + 4], 1)]
    page5 = renamed(steps[b + 4], "page5-first")

    launcher = args.output / "nlb-sdl-with-save"
    launcher.write_text(
        "#!/bin/sh\nset -eu\n"
        'slot="$HOME/.nlbnavigator/wolfman.saves/slot1.sav"\n'
        'test ! -e "$slot"\n'
        'if [ -n "$NLB_SAVE_LOAD_SEED" ]; then\n'
        '    mkdir -p "$(dirname "$slot")"\n'
        '    cp "$NLB_SAVE_LOAD_SEED" "$slot"\n'
        "fi\n"
        'exec "$NLB_SAVE_LOAD_PLAYER" "$@"\n',
        encoding="utf-8")
    launcher.chmod(0o700)

    # baseline: первая и вторая порции, экран вариантов, страница B
    baseline = write_route(args.output, "baseline",
                           route, prefix + [p1, choices, eat])
    code, ref, errors = run_case(args, launcher, baseline)
    require(code == 0, f"baseline: код {code}, {errors}")
    require(portion(ref["portion-1"]) == 1, "baseline: не вторая порция")
    require(ref["portion-1"]["pageId"] == A_PAGE, "baseline: не страница A")
    require(ref["eat-doctor"]["pageId"] == B_PAGE, "baseline: не страница B")

    # A2: первая порция — сохранение и загрузка не ухудшают поведение
    first = write_route(args.output, "first-portion", route,
                        prefix + [save(expect={"page_id": A_PAGE, "portion_index": 0}),
                                  load(expect={"page_id": A_PAGE, "portion_index": 0})])
    code, got, errors = run_case(args, launcher, first)
    require(code == 0, f"first-portion: код {code}, {errors}")
    same(got["load-a"], ref["hospital-page3"], "first-portion/load")

    # A3/A5: сохранение на 1/2, уход на B, загрузка и продолжение
    roundtrip = write_route(
        args.output, "second-portion", route,
        prefix + [p1, save(expect={"page_id": A_PAGE, "portion_index": 1}), choices, eat,
                  load(expect={"page_id": A_PAGE, "portion_index": 1}), choices, eat])
    code, got, errors = run_case(args, launcher, roundtrip)
    require(code == 0, f"second-portion: код {code}, {errors}")
    same(got["save-a"], ref["portion-1"], "second-portion/save")
    same(got["load-a"], ref["portion-1"], "second-portion/load")
    same(got["choices"], ref["choices"], "second-portion/choices")
    same(got["eat-doctor"], ref["eat-doctor"], "second-portion/eat-doctor")

    # A4: перезапуск процесса — слот переносится, загрузка с титула
    save_only = write_route(args.output, "second-portion-save",
                            route, prefix + [p1, save()])
    code, got, errors = run_case(args, launcher, save_only)
    require(code == 0, f"second-portion-save: код {code}, {errors}")
    slot = args.output / "second-portion-save" / SLOT
    require(slot.is_file(), "second-portion-save: нет файла слота")
    seed = args.output / "seed-second.sav"
    shutil.copy2(slot, seed)
    restart = write_route(args.output, "second-portion-restart", route,
                          [steps[0], load(expect={"page_id": A_PAGE, "portion_index": 1}),
                           choices, eat])
    code, got, errors = run_case(args, launcher, restart, seed)
    require(code == 0, f"second-portion-restart: код {code}, {errors}")
    same(got["load-a"], ref["portion-1"], "restart/load")
    same(got["eat-doctor"], ref["eat-doctor"], "restart/eat-doctor")

    # A6: промежуточная порция длинной страницы (page4, 3 из 4)
    middle = write_route(args.output, "middle-portion", route,
                         prefix + [p1, choices, eat] + page4[:2]
                         + [save(expect={"page_id": B_PAGE, "portion_index": 2})])
    code, got, errors = run_case(args, launcher, middle)
    require(code == 0, f"middle-portion: код {code}, {errors}")
    seed = args.output / "seed-middle.sav"
    shutil.copy2(args.output / "middle-portion" / SLOT, seed)
    middle_restart = write_route(
        args.output, "middle-portion-restart", route,
        [steps[0], load(expect={"page_id": B_PAGE, "portion_index": 2}), page4[2], page5])
    code, got, errors = run_case(args, launcher, middle_restart, seed)
    require(code == 0, f"middle-portion-restart: код {code}, {errors}")
    require(got["load-a"]["pageId"] == B_PAGE, "middle: не страница B после загрузки")
    require(portion(got["load-a"]) == 2, "middle: не третья порция после загрузки")
    same(got["page4-3"], ref_portion(route, args, launcher, prefix, p1, choices, eat, page4[:3]),
         "middle/next-portion")

    # A7: последняя порция page4 (4 из 4) и переход на page5
    last = write_route(args.output, "last-portion", route,
                       prefix + [p1, choices, eat] + page4
                       + [save(expect={"page_id": B_PAGE, "portion_index": 3})])
    code, got, errors = run_case(args, launcher, last)
    require(code == 0, f"last-portion: код {code}, {errors}")
    seed = args.output / "seed-last.sav"
    shutil.copy2(args.output / "last-portion" / SLOT, seed)
    last_restart = write_route(
        args.output, "last-portion-restart", route,
        [steps[0], load(expect={"page_id": B_PAGE, "portion_index": 3}), page5])
    code, got, errors = run_case(args, launcher, last_restart, seed)
    require(code == 0, f"last-portion-restart: код {code}, {errors}")
    require(portion(got["load-a"]) == 3, "last: не последняя порция после загрузки")
    require(got["page5-first"]["pageId"] != B_PAGE, "last: не перешли на следующую страницу")

    # A8: старое сохранение без записей ui — грузится, порция с начала страницы
    old = args.output / "seed-old.sav"
    old.write_text("".join(line for line in seed.read_text(encoding="utf-8").splitlines(True)
                           if not line.startswith("ui\t")), encoding="utf-8")
    require("ui\t" not in old.read_text(encoding="utf-8"), "A8: записи ui не удалены")
    old_restart = write_route(
        args.output, "old-save-restart", route,
        [steps[0], load(expect={"page_id": B_PAGE, "portion_index": 0})])
    code, got, errors = run_case(args, launcher, old_restart, old)
    require(code == 0, f"old-save: код {code}, {errors}")
    require(portion(got["load-a"]) == 0, "old-save: старая порция не сброшена на первую")
    print("OK: порция VN восстанавливается на всех проверенных позициях; старые сохранения совместимы")


def ref_portion(route, args, launcher, prefix, p1, choices, eat, page_steps):
    """Эталонное состояние после page_steps без сохранения (для сравнения следующего действия)"""
    baseline = write_route(args.output, "baseline-" + page_steps[-1]["id"],
                           route, prefix + [p1, choices, eat] + page_steps)
    code, records, errors = run_case(args, launcher, baseline)
    require(code == 0, f"baseline {page_steps[-1]['id']}: код {code}, {errors}")
    return records[page_steps[-1]["id"]]


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, ValueError, KeyError) as error:
        sys.exit(str(error))
