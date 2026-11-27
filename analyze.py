"""Строгая сверка общих полей по первичным снимкам SDL и INSTEAD."""
import hashlib
import json
import re
import sys
from pathlib import Path, PurePosixPath

from execute import ROOT, REPO, NLBSLOT, ISLOT

sys.path.insert(0, str(REPO / "tests/differential"))
from run_route import (CHOICE_LINK, TRACE_VARIABLES, normalize_story_text, paragraphs_text,
                       read_instead_trace, trace_sequence_valid, variable_text)

checks = []
NLB, INSTEAD = "nlbnavigator", "instead"
SIDES = (NLB, INSTEAD)
A, B = "hospital-page3", "eat-doctor"
TAIL = ["hospital-page3-last", "hospital-choices", B]
TITLE = "f7cb8bed-47a9-4ce2-b713-c9b53571c93f"


def note(label, ok, detail=None):
    checks.append({"check": label, "ok": ok, "detail": detail})


def report(location, name="route-report.json"):
    path = ROOT / location / name
    return json.loads(path.read_text()) if path.is_file() else {}


def records(location):
    return {row["step_id"]: row for row in report(location).get("state_records", [])}


def normalize(value):
    return normalize_story_text(value).rstrip("\n")


def scene(row, side):
    if side == NLB:
        model, portion = row["model"], row["portion"]
        screen = "end" if row["screen"] == "finished" else row["screen"]
        page = row["pageId"]
        background = model["pageImage"]["file"]
        variables = {name: variable_text(row["variables"][name]) for name in TRACE_VARIABLES}
        index, count, more = (portion[key] for key in ("index", "count", "more"))
        choices = [normalize(item["text"]) for item in model["choices"] if item["enabled"]]
        visible = paragraphs_text(portion["visible"])
    else:
        screen = "end" if row["end_screen"] else row["screen_kind"]
        page = row["page_id"]
        background = row["background"].removeprefix("images/")
        variables = {name: row[name] for name in TRACE_VARIABLES}
        index, count, more = (row[key] for key in ("portion_index", "portion_count", "more"))
        choices = [normalize(label) for _index, label in CHOICE_LINK.findall(row["rendered"])]
        visible = row["rendered"]
    path = PurePosixPath(background)
    if path.suffix.lower() in (".png", ".jpg", ".jpeg"):
        background = str(path.with_suffix(""))
    return dict(page=page, screen=screen, index=index, count=count, more=more, variables=variables,
                background=background, visible=choices if screen == "choices" else normalize(visible))


def compare(label, left, right, left_side=NLB, right_side=INSTEAD):
    if left is None or right is None:
        note(label, False, {"missing": [s for s, row in (("left", left), ("right", right)) if row is None]})
        return
    l, r = scene(left, left_side), scene(right, right_side)
    diff = {key: [l[key], r[key]] for key in l if l[key] != r[key]}
    note(label, not diff, diff or l)


def paired(label, dirs, ids, references=None):
    by = {side: records(dirs[side]) for side in SIDES}
    for left_name, right_name, expected in ids:
        left, right = by[NLB].get(left_name), by[INSTEAD].get(right_name)
        compare(f"{label}/{left_name}/cross", left, right)
        if references:
            compare(f"{label}/{left_name}/SDL-baseline", left, references[NLB].get(expected), NLB, NLB)
            compare(f"{label}/{right_name}/INSTEAD-baseline", right, references[INSTEAD].get(expected), INSTEAD, INSTEAD)


def locations(case):
    return {side: f"matrix/{case}/{side}" for side in SIDES}


def exact_commands(path, side, intended):
    target = ROOT / path
    if side == INSTEAD:
        trace = read_instead_trace(target / "instead-state.tsv")
        events = [(row["command"].split(" ", 1)[0], row["seq"]) for row in trace
                  if row["kind"] != "fmt" and row["command"].endswith("/save9")
                  and row["command"].split(" ", 1)[0] in ("load", "save")]
        checks_pass = bool(trace) and trace_sequence_valid(trace)
        checks_pass &= report(path).get("process", {}).get("exit_code") == 0
    else:
        log = target / "state-log.jsonl"
        trace = [json.loads(line) for line in log.read_text().splitlines()] if log.is_file() else []
        pairs = [(older, newer) for older, newer in zip(trace, trace[1:])
                 if older["command"] in ("key:f5", "key:f9") and newer["command"] == "menu:1"]
        events = [("save" if first["command"] == "key:f5" else "load", second["index"])
                  for first, second in pairs if first["ok"] and second["ok"]]
        log = target / "playthrough.log"
        text = log.read_text() if log.is_file() else ""
        checks_pass = len(pairs) == len(events) and bool(trace)
        checks_pass &= all(text.count(marker) == intended.count(kind) for kind, marker in
                           (("save", "Сохранить в слот 1"), ("load", "Загрузить слот 1")))
    note(f"{path}/commands", checks_pass and [event[0] for event in events] == intended,
         {"observed": events, "expected": intended, "underlying_checks": bool(checks_pass)})


def transfer(label, original, restored):
    evidence = []
    for path in (original, restored):
        path = ROOT / path
        if path.is_file():
            data = path.read_bytes()
            evidence.append({"file": str(path.relative_to(ROOT)), "bytes": len(data),
                             "sha256": hashlib.sha256(data).hexdigest()})
    note(label, len(evidence) == 2 and evidence[0]["sha256"] == evidence[1]["sha256"]
         and evidence[0]["bytes"] == evidence[1]["bytes"] > 0, evidence)


base = locations("baseline")
ref = {side: records(base[side]) for side in SIDES}
required = [A, "hospital-page3-last", "hospital-choices", B, "doctor-page4-portion-1",
            "doctor-page4-portion-2", "doctor-page4-portion-3", "page5-after-doctor"]
paired("D1", base, [(name, name, name) for name in required])
for side in SIDES:
    for name, uuid, index, count, variables in [
        (A, "8c1fa8c6-8027-43c8-8d8b-0bf454d5e717", 0, 2, ("0", "2")),
        (B, "d937c5c1-766c-4581-a03e-736560b0cc8f", 0, 4, ("1", "3")),
    ]:
        row = ref[side].get(name)
        actual = scene(row, side) if row else {}
        var = dict(zip(TRACE_VARIABLES, (*variables, "false", "false", "false", "false")))
        note(f"D1/{side}/{name}/expected", (actual.get("page"), actual.get("index"), actual.get("count"),
                                           actual.get("variables")) == (uuid, index, count, var), actual)

cycles = {NLB: "standard/cycles/nlbnavigator", INSTEAD: "instead-standard/cycles"}
mappings = [(A, A, A), ("save-a", "save-a", A)] + [
    ("baseline-" + name, "baseline-" + name, name) for name in TAIL]
for cycle in range(1, 4):
    mappings += [(f"load-a-{cycle}", f"load-a-{cycle}", A)]
    mappings += [(f"cycle-{cycle}-{name}", f"cycle-{cycle}-{name}", name) for name in TAIL]
paired("D2", cycles, mappings, ref)
for side in SIDES:
    exact_commands(cycles[side], side, ["save", "load", "load", "load"])

write_dirs = {NLB: "standard/save_load/save-only", INSTEAD: "instead-standard/restart-save"}
restart_dirs = {NLB: "standard/save_load/restart", INSTEAD: "instead-retry/restart-load"}
paired("D3/save", write_dirs, [("save-a", "save-a", A)], ref)
paired("D3/restart", restart_dirs,
       [("load-a", "load-a", A), *[("replay-" + name, "restart-" + name, name) for name in TAIL]], ref)
for side, slot in ((NLB, Path("nlbnavigator-home") / NLBSLOT), (INSTEAD, ISLOT)):
    transfer(f"D3/{side}/seed", Path(write_dirs[side]) / slot, Path(restart_dirs[side]) / slot)
    exact_commands(write_dirs[side], side, ["save"])
    exact_commands(restart_dirs[side], side, ["load"])
processes = [report(p).get("process", {}).get("pid") for p in (write_dirs[INSTEAD], restart_dirs[INSTEAD])]
note("D3/INSTEAD/new-process", None not in processes and len(set(processes)) == 2, processes)

for name, point, tail in (
    ("a1", "hospital-page3-last", ["hospital-choices", B]),
    ("b2", "doctor-page4-portion-2", ["doctor-page4-portion-3", "page5-after-doctor"]),
    ("b3", "doctor-page4-portion-3", ["page5-after-doctor"]),
):
    for phase in ("roundtrip", "restart"):
        dirs = locations(name + "-" + phase)
        ids = [("load-a", "load-a", point)] + [
            ("replay-" + step, "replay-" + step, step) for step in tail]
        if phase == "roundtrip":
            ids = [(point, point, point), ("save-a", "save-a", point)] + ids
        paired(f"D4/{name}/{phase}", dirs, ids, ref)
        for side, slot in ((NLB, Path("nlbnavigator-home") / NLBSLOT), (INSTEAD, ISLOT)):
            exact_commands(dirs[side], side, ["save", "load"] if phase == "roundtrip" else ["load"])
            if phase == "restart":
                transfer(f"D4/{name}/{side}/seed", Path(locations(name + "-roundtrip")[side]) / slot,
                         Path(dirs[side]) / slot)

empty = ROOT / "standard/save_load/empty"
states_path = empty / "state-log.jsonl"
states = [json.loads(line) for line in states_path.read_text().splitlines()] if states_path.is_file() else []
note("D5/SDL/empty", bool(states) and all(item["pageId"] == TITLE and item["variables"]["hp"] == 0 for item in states)
     and [item["command"] for item in states if not item["ok"]] == ["menu:1"]
     and not (empty / "nlbnavigator-home" / NLBSLOT).exists(), len(states))
empty_instead = "instead-retry/missing-slot"
rows = read_instead_trace(ROOT / empty_instead / "instead-state.tsv")
note("D5/INSTEAD/empty", bool(rows) and all(row["page_id"] == TITLE and row["hp"] == "0"
                                          for row in rows if row["kind"] == "scene")
     and not (ROOT / empty_instead / ISLOT).exists()
     and any("нет свежей записи команды load" in e for e in report(empty_instead).get("errors", [])), len(rows))
exact_commands(empty_instead, INSTEAD, [])

resave = {side: f"standard/resave/{side}" for side in SIDES}
paired("D5/resave", resave, [("load-a", "load-a", A), ("resave-a", "resave-a", A)], ref)
for side in SIDES:
    exact_commands(resave[side], side, ["load", "save"])

for case in ("choices", "ending"):
    dirs = locations(case)
    for side in SIDES:
        by_id = records(dirs[side])
        start = next((row for step, row in reversed(list(by_id.items()))
                      if step not in ("save-a", "load-a", "continue-ending")), None)
        for checkpoint in ("save-a", "load-a"):
            compare(f"D5/{case}/{side}/{checkpoint}/same-scene", by_id.get(checkpoint), start, side, side)
        exact_commands(dirs[side], side, ["save", "load"])
    paired("D5/" + case, dirs, [("save-a", "save-a", ""), ("load-a", "load-a", "")])

end_left = records(locations("ending")[NLB]).get("continue-ending")
end_right = records(locations("ending")[INSTEAD]).get("continue-ending")
if end_left and end_right:
    left, right = scene(end_left, NLB), scene(end_right, INSTEAD)
    # На терминальном экране движки показывают разный интерфейс повтора игры.
    keys = ("page", "screen", "index", "count", "more", "variables", "background")
    note("D5/ending/next", all(left[k] == right[k] for k in keys)
         and left["screen"] == "end" and end_left["finished"] and end_right["end_screen"],
         {"SDL": left, "INSTEAD": right})
else:
    note("D5/ending/next", False, {"SDL": bool(end_left), "INSTEAD": bool(end_right)})

(ROOT / "semantic-comparison.json").write_text(json.dumps(checks, ensure_ascii=False, indent=2) + "\n")
for item in checks:
    if not item["ok"]:
        print("FAIL", item["check"], json.dumps(item["detail"], ensure_ascii=False))
print(f"checks={len(checks)}, passed={sum(item['ok'] for item in checks)}, conditions_not_met={sum(not item['ok'] for item in checks)}")
sys.exit(int(not all(item["ok"] for item in checks)))
