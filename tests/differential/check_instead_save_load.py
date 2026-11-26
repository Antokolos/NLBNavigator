#!/usr/bin/env python3
"""Проверяет быстрый слот Wolfman, повторную загрузку и независимый перезапуск INSTEAD."""
import argparse
import hashlib
import json
import shlex
import subprocess
import sys
from pathlib import Path

from run_route import (TRACE_VARIABLES, load_route, normalize_story_text,
                       read_instead_trace, semantic_state, trace_sequence_valid)

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
ROUTE = HERE / "routes/wolfman-police-hospital-doctor.json"
A_PAGE = "8c1fa8c6-8027-43c8-8d8b-0bf454d5e717"
B_PAGE = "d937c5c1-766c-4581-a03e-736560b0cc8f"
SLOT = Path("instead-appdata/saves/instead-game-overlay/save9")


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def continuation(steps, tag):
    renamed = []
    for step in steps:
        name = f"{tag}-{step['id']}"
        copy = dict(step, id=name, screenshot=f"{name}.png")
        copy.pop("visual_change_from", None)
        renamed.append(copy)
    return renamed


def menu(name, key, page):
    return {"id": name, "screenshot": f"{name}.png", "settle_ms": 2000,
            "action": {"type": "menu", "nlbnavigator": ["f5" if key == "F8" else "f9", 1],
                       "instead": [{"key": key}]},
            "expect": {"instead_page_id": page} if page else {}}


def write_json(path, data):
    path.write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def run(args, source, name, steps, expected_commands, seed=None, expected_exit=0):
    output = args.output / name
    output.mkdir()
    route = args.output / f"{name}.json"
    write_json(route, dict(source, id=f"wolfman-instead-save-load-{name}", steps=steps))
    load_route(route)
    command = [sys.executable, str(HERE / "run_route.py"), "--runtime", "instead",
               "--route", str(route), "--output", str(output), "--game", str(args.game),
               "--base-theme", str(args.base_theme), "--instead-binary", str(args.instead_binary)]
    if seed is not None:
        command += ["--instead-seed-save", str(seed)]
    (output / "runner.command").write_text(shlex.join(command) + "\n", encoding="utf-8")
    with (output / "runner.log").open("w", encoding="utf-8") as stream:
        code = subprocess.run(command, cwd=REPO, stdout=stream, stderr=subprocess.STDOUT).returncode
    (output / "runner.exit").write_text(f"{code}\n", encoding="utf-8")
    print(f"{name}: раннер завершился с кодом {code}", flush=True)
    report_path = output / "route-report.json"
    require(report_path.is_file(), f"{name}: нет отчёта раннера")
    report = json.loads(report_path.read_text(encoding="utf-8"))
    require(code == expected_exit and report["ok"] == (code == 0), f"{name}: {report['errors']}")
    require(report["process"]["exit_code"] == 0, f"{name}: завершение INSTEAD: {report['process']}")

    trace_path = output / "instead-state.tsv"
    trace = read_instead_trace(trace_path)
    require(trace and trace_sequence_valid(trace), f"{name}: seq должны строго возрастать")
    with trace_path.open(encoding="utf-8") as file:
        headers = sum(line.startswith("#") for line in file)
    require(headers == 1, f"{name}: число заголовков {headers}")
    commands = []
    for record in trace:
        verb, _, path = record["command"].partition(" ")
        if record["kind"] == "fmt" or verb not in ("save", "load"):
            continue
        path = path.replace("\\", "/")
        # Экспорт может создавать собственное автосохранение. Проверяем отдельно
        # команды, адресованные быстрому слоту; исходный TSV содержит и autosave.
        if path.endswith("/autosave"):
            continue
        require(path.endswith("/saves/instead-game-overlay/save9"),
                f"{name}: неожиданный слот в {record['command']}")
        commands.append(record)
    require([record["command"].split(" ", 1)[0] for record in commands] == expected_commands,
            f"{name}: неожиданный порядок или число save/load")
    proof = {"runner_exit": code, "process": report["process"], "headers": headers,
             "records": len(trace), "first_seq": trace[0]["seq"], "last_seq": trace[-1]["seq"],
             "commands": []}
    for record in commands:
        index = trace.index(record)
        proof["commands"].append({"command": record, "before": trace[max(0, index - 1)],
                                  "after": trace[min(len(trace) - 1, index + 1)]})
    write_json(output / "trace-proof.json", proof)
    return report, {record["step_id"]: record for record in report["state_records"]}


def assert_checkpoint(record, page, count, wolfiness, hp):
    variables = dict(zip(TRACE_VARIABLES, (str(wolfiness), str(hp), "false", "false", "false", "false")))
    require((record["page_id"], record["screen_kind"], record["portion_index"], record["portion_count"])
            == (page, "page", 0, count), f"Не совпало состояние сцены: {record['step_id']}")
    require(all(record.get(name) == value for name, value in variables.items()),
            f"Не совпали переменные сцены: {record['step_id']}")
    require(record["background"] and normalize_story_text(record["rendered"]),
            f"Нет ресурса или текста сцены: {record['step_id']}")


def assert_same(actual, baseline):
    # INSTEAD добавляет пустую строку к той же порции при перерисовке после F9.
    actual_text = normalize_story_text(actual["rendered"]).rstrip("\r\n")
    baseline_text = normalize_story_text(baseline["rendered"]).rstrip("\r\n")
    require(semantic_state(actual) == semantic_state(baseline) and actual_text == baseline_text,
            f"{actual['step_id']}: состояние отличается от {baseline['step_id']}")


def slot_info(path):
    data = path.read_bytes()
    require(bool(data), f"Пустой слот: {path}")
    require(b"__nlb_trace_state" not in data, f"Состояние трассировки попало в слот: {path}")
    return {"path": str(path), "bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for option in ("game", "base-theme", "instead-binary", "output"):
        parser.add_argument("--" + option, type=Path, required=True)
    args = parser.parse_args()
    args.output = args.output.absolute()
    args.output.mkdir(parents=True)

    source = load_route(ROUTE)
    steps = source["steps"]
    ids = [step["id"] for step in steps]
    a, b = ids.index("hospital-page3"), ids.index("eat-doctor")
    prefix, to_b = steps[:a + 1], steps[a + 1:b + 1]
    save = menu("save-a", "F8", A_PAGE)

    cycles = prefix + [save] + continuation(to_b, "baseline")
    for index in range(1, 4):
        cycles += [menu(f"load-a-{index}", "F9", A_PAGE)] + continuation(to_b, f"cycle-{index}")
    _, states = run(args, source, "cycles", cycles, ["save", "load", "load", "load"])
    baseline_a, baseline_b = states["hospital-page3"], states["baseline-eat-doctor"]
    assert_checkpoint(baseline_a, A_PAGE, 2, 0, 2)
    assert_checkpoint(baseline_b, B_PAGE, 4, 1, 3)
    assert_same(states["save-a"], baseline_a)
    for index in range(1, 4):
        assert_same(states[f"load-a-{index}"], baseline_a)
        assert_same(states[f"cycle-{index}-eat-doctor"], baseline_b)
    print("Три цикла A → B → A прошли с теми же экраном, текстом, ресурсом и шестью переменными.", flush=True)

    save_report, states = run(args, source, "restart-save", prefix + [save], ["save"])
    assert_same(states["save-a"], baseline_a)
    seed = args.output / "restart-save" / SLOT
    original = slot_info(seed)
    load = menu("load-a", "F9", A_PAGE)
    load_report, states = run(args, source, "restart-load", [steps[0], load] + continuation(to_b, "restart"),
                              ["load"], seed=seed)
    assert_same(states["load-a"], baseline_a)
    assert_same(states["restart-eat-doctor"], baseline_b)
    require(save_report["process"]["pid"] != load_report["process"]["pid"], "Ожидался новый процесс INSTEAD")
    restored = slot_info(args.output / "restart-load" / SLOT)
    require(original["bytes"] == restored["bytes"] and original["sha256"] == restored["sha256"],
            "Содержимое перенесённого слота отличается")
    write_json(args.output / "slots.json", {"cycles": slot_info(args.output / "cycles" / SLOT),
                                           "original": original, "copied": restored})

    report, _ = run(args, source, "missing-slot", [steps[0], menu("load-empty", "F9", None)],
                    [], expected_exit=1)
    require(any("нет свежей записи команды load" in error for error in report["errors"]),
            "Отсутствующий слот не распознан")
    require(not (args.output / "missing-slot" / SLOT).exists(), "В пустом профиле появился слот")
    print("Перезапуск с одним файлом save9 и отрицательный контроль пустого слота прошли.")


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, ValueError, KeyError) as error:
        sys.exit(str(error))
