#!/usr/bin/env python3
"""Временный probe: воспроизведение #68 (порция VN) и #69 (trace INSTEAD).

Не входит в main: запускается workflow probe-save-load.yml на временной ветке.
Собирает первичные данные (state-log, route-report, INSTEAD trace) и пишет
probe-summary.json с вердиктами. Exit code — 0, если сбор данных удался;
вердикты о дефектах — в summary (pre-fix ожидаются провалы).
"""

import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
SOURCE_ROUTE = HERE / "routes/wolfman-police-hospital-doctor.json"
A = "hospital-page3"
B = "eat-doctor"
A_PAGE = "8c1fa8c6-8027-43c8-8d8b-0bf454d5e717"
B_PAGE = "d937c5c1-766c-4581-a03e-736560b0cc8f"
SLOT = Path("nlbnavigator-home/.nlbnavigator/wolfman.saves/slot1.sav")
TEXT_0 = "Вы очнулись от яркого света"
TEXT_1 = "Приоткрыв глаза"


def load_source():
    return json.loads(SOURCE_ROUTE.read_text(encoding="utf-8"))


def route_with(source, name, steps):
    # visual_change_from валиден только в исходном порядке шагов; в переставленных
    # сценариях он не нужен (снимки — диагностика), поэтому убираем его совсем
    clean = [{key: value for key, value in step.items() if key != "visual_change_from"}
             for step in steps]
    return dict(source, id=f"probe-{name}", steps=clean)


def save_route(output, name, route):
    path = output / f"{name}.json"
    path.write_text(json.dumps(route, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    return path


def advance(name="advance-portion"):
    return {
        "id": name, "settle_ms": 1500, "screenshot": f"{name}.png",
        "action": {"type": "click", "x_ratio": 0.5, "y_ratio": 0.5},
    }


def expect_state(portion):
    text = TEXT_1 if portion == 1 else TEXT_0
    return {
        "page_id": A_PAGE, "portion_index": portion, "portion_count": 2,
        "text_contains": text,
        "instead_page_id": A_PAGE, "instead_portion_index": portion, "instead_portion_count": 2,
        "instead_text_contains": text,
    }


def menu(name, nlb, instead, expect):
    return {
        "id": name, "settle_ms": 1500, "screenshot": f"{name}.png",
        "action": {"type": "menu", "nlbnavigator": nlb, "instead": instead},
        "expect": expect,
    }


def replay(steps, prefix):
    out = []
    for step in steps:
        copy = {key: value for key, value in step.items() if key != "visual_change_from"}
        copy.update(id=f"{prefix}-{step['id']}", screenshot=f"{prefix}-{step['screenshot']}")
        out.append(copy)
    return out


def run_runner(name, runtime, route_path, output, args, launcher=None, seed=None):
    output.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ)
    if launcher is not None:
        env["PROBE_PLAYER"] = str(args.player)
        env["PROBE_SEED"] = str(seed or "")
    command = [
        sys.executable, str(HERE / "run_route.py"), "--runtime", runtime,
        "--route", str(route_path), "--output", str(output), "--timeout", "420",
    ]
    if runtime == "nlbnavigator":
        command += ["--player", str(launcher or args.player), "--book", str(args.book),
                    "--theme", str(args.theme), "--base-theme", str(args.base_theme)]
    else:
        command += ["--game", str(args.game), "--base-theme", str(args.base_theme),
                    "--instead-binary", str(args.instead_binary)]
    log = output / "runner.log"
    with log.open("w", encoding="utf-8") as stream:
        result = subprocess.run(command, cwd=REPO, env=env, stdout=stream, stderr=subprocess.STDOUT)
    return result.returncode, output


def state_records(output):
    path = output / "route-report.json"
    if not path.is_file():
        return {}, []
    report = json.loads(path.read_text(encoding="utf-8"))
    records = {record["step_id"]: record for record in report.get("state_records", [])}
    return records, report.get("errors", [])


def tail_load(output):
    """Последняя загрузка из state-log, если route-report не записан (раннер упал)"""
    path = output / "state-log.jsonl"
    if not path.is_file():
        return {}
    entries = [json.loads(line) for line in path.read_text(encoding="utf-8").splitlines() if line.strip()]
    for index in range(len(entries) - 1, -1, -1):
        if entries[index].get("command") == "key:f9":
            for record in entries[index + 1:]:
                if record.get("command") == "menu:1":
                    return record
    return {}


def portion(record, runtime):
    if not record:
        return None
    if runtime == "nlbnavigator":
        return record.get("portion", {}).get("index")
    return record.get("portion_index")


def page(record):
    return (record or {}).get("pageId") or (record or {}).get("page_id")


def trace_stats(path):
    if not path.is_file():
        return {"exists": False}
    lines = [line for line in path.read_text(encoding="utf-8", errors="replace").splitlines()
             if line and not line.startswith("#")]
    records = [line.split("\t") for line in lines]
    seqs = []
    for fields in records:
        try:
            seqs.append(int(fields[0]))
        except (IndexError, ValueError):
            pass
    empty = sum(1 for fields in records if len(fields) > 4 and fields[4] == "")
    return {
        "exists": True, "records": len(records), "max_seq": max(seqs, default=0),
        "min_seq": min(seqs, default=0), "empty_rendered": empty,
        "first_commands": [fields[2] for fields in records[:3] if len(fields) > 2],
        "last_commands": [fields[2] for fields in records[-6:] if len(fields) > 2],
    }


def launcher_with_slot(output):
    launcher = output / "player-with-slot"
    launcher.write_text(
        "#!/bin/sh\nset -eu\n"
        'slot="$HOME/.nlbnavigator/wolfman.saves/slot1.sav"\n'
        'test ! -e "$slot"\n'
        'if [ -n "${PROBE_SEED:-}" ]; then\n'
        '    mkdir -p "$(dirname "$slot")"\n'
        '    cp "$PROBE_SEED" "$slot"\n'
        "fi\n"
        'exec "$PROBE_PLAYER" "$@"\n',
        encoding="utf-8",
    )
    launcher.chmod(0o700)
    return launcher


def main():
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    for option in ("player", "book", "theme", "base-theme", "game", "instead-binary", "output"):
        parser.add_argument("--" + option, type=lambda value: Path(value).resolve(), required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    summary = {"schema": 1, "scenarios": {}}

    source = load_source()
    ids = [step["id"] for step in source["steps"]]
    a_index = ids.index(A)
    b_index = ids.index(B)
    prefix = source["steps"][:a_index + 1]
    after_portion = source["steps"][a_index + 2:b_index + 1]   # экран вариантов и «съесть врача»
    title = source["steps"][0]

    save_a = menu("save-a", ["f5", 1], [{"key": "F8"}], expect_state(1))
    load_a = menu("load-a", ["f9", 1], [{"key": "F9"}], expect_state(1))
    save_0 = menu("save-a", ["f5", 1], [{"key": "F8"}], expect_state(0))
    load_0 = menu("load-a", ["f9", 1], [{"key": "F9"}], expect_state(0))
    scenarios = {
        "same-page": prefix + [advance(), save_a, load_a],
        "cross-page": prefix + [advance(), save_a] + after_portion + [load_a],
        "cross-page-continue": prefix + [advance(), save_a] + after_portion + [load_a] + after_portion,
        "first-portion": prefix + [save_0, load_0],
        "restart-save": prefix + [advance(), save_a],
    }
    routes = {name: save_route(args.output, name, route_with(source, name, steps))
              for name, steps in scenarios.items()}
    restart_load = save_route(
        args.output, "restart-load",
        route_with(source, "restart-load", [title, load_a] + after_portion))

    launcher = launcher_with_slot(args.output)
    for name in ("same-page", "cross-page", "cross-page-continue", "first-portion"):
        output = args.output / "nlbnavigator" / name
        code, _ = run_runner(name, "nlbnavigator", routes[name], output, args, launcher)
        records, errors = state_records(output)
        record = records.get("load-a") or tail_load(output)
        summary["scenarios"][f"nlbnavigator/{name}"] = {
            "exit": code, "errors": errors,
            "load_page": page(record), "load_portion": portion(record, "nlbnavigator"),
            "load_screen": (record or {}).get("screen"),
        }

    output = args.output / "nlbnavigator" / "restart-save"
    code, _ = run_runner("restart-save", "nlbnavigator", routes["restart-save"], output, args, launcher)
    slot = output / SLOT
    restart = {"save_exit": code, "slot": str(slot), "slot_exists": slot.is_file()}
    if slot.is_file():
        seed = args.output / "restart-seed.sav"
        shutil.copy2(slot, seed)
        load_output = args.output / "nlbnavigator" / "restart-load"
        code2, _ = run_runner("restart-load", "nlbnavigator", restart_load, load_output, args, launcher, seed)
        records, errors = state_records(load_output)
        record = records.get("load-a") or tail_load(load_output)
        restart.update({
            "load_exit": code2, "errors": errors,
            "load_page": page(record), "load_portion": portion(record, "nlbnavigator"),
            "load_screen": (record or {}).get("screen"),
        })
    summary["scenarios"]["nlbnavigator/restart"] = restart

    for name in ("cross-page", "first-portion"):
        output = args.output / "instead" / name
        code, _ = run_runner(name, "instead", routes[name], output, args)
        records, errors = state_records(output)
        trace = trace_stats(output / "instead-state.partial.tsv")
        summary["scenarios"][f"instead/{name}"] = {
            "exit": code, "errors": errors, "trace": trace,
            "load_page": page(records.get("load-a")),
            "load_portion": portion(records.get("load-a"), "instead"),
            "save_rendered_empty": bool(
                records.get("save-a") and not (records["save-a"].get("rendered") or "").strip()),
        }

    (args.output / "probe-summary.json").write_text(
        json.dumps(summary, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(summary, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
