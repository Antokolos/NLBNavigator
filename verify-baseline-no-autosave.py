"""Подтверждает, что в D1 сохранений нет, а config.ini и prefs относятся к настройкам."""
import json

from execute import ROOT

test = ROOT / "matrix/baseline-noautosave-retry"
previous = json.loads((test / "no-autosave-proof.json").read_text())
settings = {"config.ini", "prefs"}
appdata = test / "instead/instead-appdata"
files = sorted(path for path in (appdata / "saves").rglob("*") if path.is_file())
preferences = [str(path.relative_to(appdata)) for path in files if path.name in settings]
slot_files = [str(path.relative_to(appdata)) for path in files if path.name not in settings]
assert "autosave = 0" in (appdata / "insteadrc").read_text().splitlines()
sdl = ROOT / "matrix/baseline/nlbnavigator"
records = [json.loads(line) for line in (sdl / "state-log.jsonl").read_text().splitlines()]
commands = [row["command"] for row in records if row["command"] in ("key:f5", "key:f9")]
sdl_slots = list((sdl / "nlbnavigator-home/.nlbnavigator").glob("*.saves/*"))
assert not any((previous["instead_exit"], previous["stock_compare_exit"],
                previous["save_or_load_trace_events"], slot_files, commands, sdl_slots))
assert previous["identical_to_first_instead_baseline"] and previous["checkpoints"] == 13
proof = {**previous, "slot_files": slot_files, "preferences": preferences,
         "sdl_save_or_load_commands": commands, "sdl_slot_files": sdl_slots}
with (test / "no-autosave-verified.json").open("x") as report:
    json.dump(proof, report, ensure_ascii=False, indent=2)
    report.write("\n")
print(json.dumps(proof, ensure_ascii=False, indent=2))
