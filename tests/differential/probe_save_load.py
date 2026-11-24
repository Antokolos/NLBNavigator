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


def load_source():
    return json.loads(SOURCE_ROUTE.read_text(encoding="utf-8"))


def step_ids(route):
    return [step["id"] for step in route["steps"]]


def route_with(source, name, steps):
    return dict(source, id=f"probe-{name}", steps=steps)


def save_route(output, name, route):
    path = output / f"{name}.json"
    path.write_text(json.dumps(route, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    return path


def advance(name="advance-portion"):
    return {
        "id": name, "settle_ms": 1500, "screenshot": f"{name}.png",
        "action": {"type": "click", "x_ratio": 0.5, "y_ratio": 0.5},
    }


def menu(name, nlb, instead):
    return {
        "id": name, "settle_ms": 1500, "screenshot": f"{name}.png",
        "action": {"type": "menu", "nlbnavigator": nlb, "instead": instead},
    }


def replay(steps, prefix):
    out = []
    for step in steps:
        copy = dict(step, id=f"{prefix}-{step['id']}", screenshot=f"{prefix}-{step['screenshot']}")
        if "visual_change_from" in step:
            copy["visual_change_from"] = f"{prefix}-{step['visual_change_from']}"
        else:
            copy.pop("visual_change_from", None)
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
        command += [
            "--player", str(launcher or args.player), "--book", str(args.book),
            "--theme", str(args.theme), "--base-theme", str(args.base_theme),
        ]
    else:
        command += [
            "--game", str(args.game), "--base-theme", str(args.base_theme),
            "--instead-binary", str(args.instead_binary),
        ]
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


def portion(record, runtime):
    if not record:
        return None
    if runtime == "nlbnavigator":
        return record.get("portion", {}).get("index")
    return record.get("portion_index")


def page(record):
    return (record or {}).get("pageId") or (record or {}).get("page_id")


def screen(record):
    return (record or {}).get("screen") or (record or {}).get("screen_kind")


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
        "last_commands": [fields[2] for fields in records[-8:] if len(fields) > 2],
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
    ids = step_ids(source)
    a_index = ids.index(A)
    b_index = ids.index(B)
    prefix = source["steps"][:a_index + 1]
    continuation = source["steps"][a_index + 1:b_index + 1]
    title = source["steps"][0]

    scenarios = {
        "same-page": prefix + [advance(), menu("save-a", ["f5", 1], [{"key": "F8"}]),
                               menu("load-a", ["f9", 1], [{"key": "F9"}])],
        "cross-page": prefix + [advance(), menu("save-a", ["f5", 1], [{"key": "F8"}])]
                      + continuation + [menu("load-a", ["f9", 1], [{"key": "F9"}])]
                      + replay(continuation, "replay"),
        "first-portion": prefix + [menu("save-a", ["f5", 1], [{"key": "F8"}]),
                                   menu("load-a", ["f9", 1], [{"key": "F9"}])],
        "restart-save": prefix + [advance(), menu("save-a", ["f5", 1], [{"key": "F8"}])],
    }
    routes = {name: save_route(args.output, name, route_with(source, name, steps))
              for name, steps in scenarios.items()}
    restart_load = save_route(
        args.output, "restart-load",
        route_with(source, "restart-load", [title, menu("load-a", ["f9", 1], [{"key": "F9"}])]
                   + replay(continuation, "replay")),
    )

    launcher = launcher_with_slot(args.output)
    for name in ("same-page", "cross-page", "first-portion"):
        output = args.output / "nlbnavigator" / name
        code, _ = run_runner(name, "nlbnavigator", routes[name], output, args, launcher)
        records, errors = state_records(output)
        summary["scenarios"][f"nlbnavigator/{name}"] = {
            "exit": code, "errors": errors,
            "load_page": page(records.get("load-a")),
            "load_portion": portion(records.get("load-a"), "nlbnavigator"),
            "load_screen": screen(records.get("load-a")),
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
        restart.update({
            "load_exit": code2, "errors": errors,
            "load_page": page(records.get("load-a")),
            "load_portion": portion(records.get("load-a"), "nlbnavigator"),
            "load_screen": screen(records.get("load-a")),
        })
    summary["scenarios"]["nlbnavigator/restart"] = restart

    # INSTEAD: тот же cross-page и first-portion; смотрим код раннера и trace
    for name in ("cross-page", "first-portion"):
        output = args.output / "instead" / name
        code, _ = run_runner(name, "instead", routes[name], output, args)
        trace = trace_stats(output / "instead-state.partial.tsv")
        records, errors = state_records(output)
        summary["scenarios"][f"instead/{name}"] = {
            "exit": code, "errors": errors, "trace": trace,
            "load_page": page(records.get("load-a")),
            "load_portion": portion(records.get("load-a"), "instead"),
            "save_rendered_empty": bool(
                records.get("save-a") and not (records["save-a"].get("rendered") or "").strip()
            ),
        }

    (args.output / "probe-summary.json").write_text(
        json.dumps(summary, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(summary, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
