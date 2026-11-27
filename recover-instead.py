"""Изолированная повторная попытка после тайм-аута xwininfo при старте."""
import argparse
import json
import sys

from execute import ROOT, REPO, GAME, THEME, INSTEAD, ISLOT, run

sys.path.insert(0, str(REPO / "tests/differential"))
from check_instead_save_load import menu, run as run_case, slot_info
from run_route import load_route

root = ROOT / "instead-retry"
root.mkdir()
args = argparse.Namespace(output=root, game=GAME, base_theme=THEME, instead_binary=INSTEAD)
original = ROOT / "instead-standard"
title = load_route(original / "cycles.json")["steps"][0]
restart = load_route(original / "restart-load.json")
seed = original / "restart-save" / ISLOT
report, states = run_case(args, restart, "restart-load", restart["steps"], ["load"], seed=seed)
a, b = slot_info(seed), slot_info(root / "restart-load" / ISLOT)
assert (a["bytes"], a["sha256"]) == (b["bytes"], b["sha256"])
(root / "slots.json").write_text(json.dumps({"original": a, "copied": b}, indent=2) + "\n")
empty, _ = run_case(args, restart, "missing-slot", [title, menu("load-empty", "F9", None)], [], expected_exit=1)
assert any("нет свежей записи команды load" in message for message in empty["errors"])
assert not (root / "missing-slot" / ISLOT).exists()
