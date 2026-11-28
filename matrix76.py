"""Строгие сценарии D1–D5 через штатный раннер. Каталоги каждого запуска новые."""
from pathlib import Path
import hashlib
import json
import sys

from execute import ROOT, REPO, NLBSLOT, ISLOT, engine, pair
sys.path.insert(0, str(REPO / "tests/differential"))
from check_wolfman_save_portion import load, save, renamed, write_route
from run_route import CHOICE_LINK, TRACE_VARIABLES, load_route, normalize_story_text, paragraphs_text, read_instead_trace, trace_sequence_valid, variable_text

OUT = ROOT / "matrix"
NLB, INSTEAD = "nlbnavigator", "instead"
SLOTS = {NLB: Path("nlbnavigator-home") / NLBSLOT, INSTEAD: ISLOT}
ROUTES = REPO / "tests/differential/routes"
source = load_route(ROUTES / "wolfman-police-hospital-doctor.json")
corpse = load_route(ROUTES / "wolfman-police-hospital-corpse.json")
steps = source["steps"]
indices = {step["id"]: index for index, step in enumerate(steps)}
a, a1, panel, b, c = (indices[k] for k in ("hospital-page3", "hospital-page3-last",
                     "hospital-choices", "eat-doctor", "page5-after-doctor"))
title = steps[0]
doctor_step = steps[b]
corpse_step = next(step for step in corpse["steps"] if step["id"] == "eat-corpse")
ordered_choices = ["Съесть врача", "Съесть труп"]
flags = {key: False for key in TRACE_VARIABLES[2:]}
choice_expect = dict(steps[panel]["expect"], portion_count=2, instead_portion_count=2,
                     variables=dict(flags, wolfiness=0, hp=2))
checks = []


def report(path):
    return json.loads(path.read_text(encoding="utf-8"))


def confirm(label, condition, details=None):
    checks.append(dict(check=label, ok=bool(condition), detail=details))
    if not condition:
        print("ОШИБКА", label, details, flush=True)


def action_expect(step):
    return {k: value for k, value in step["expect"].items() if k not in ("min_changed_pixels", "portion_index", "instead_portion_index")}


def check_case(name, route, seeds=None, no_autosave=False):
    result_dir = OUT / name
    code = pair(route, result_dir, 371, seeds, **(
        {"instead_player": ROOT / "instead-no-autosave.py"} if no_autosave else {}
    ))
    confirm(name + "/exits", all(value == 0 for value in code.values()), code)
    reports = {runtime: report(result_dir / runtime / "route-report.json")
               for runtime in SLOTS if (result_dir / runtime / "route-report.json").is_file()}
    confirm(name + "/reports", len(reports) == 2 and all(row["ok"] and not row["errors"] for row in reports.values()))
    if INSTEAD in reports:
        confirm(name + "/process", reports[INSTEAD]["process"]["exit_code"] == 0, reports[INSTEAD]["process"])
    return result_dir


def result_states(case):
    return {runtime: {r["step_id"]: r for r in report(case / runtime / "route-report.json")["state_records"]}
            for runtime in SLOTS if (case / runtime / "route-report.json").is_file()}


def normalize(text):
    return normalize_story_text(text).rstrip("\n")


def scene(record, runtime):
    if runtime == NLB:
        model, portion = record["model"], record["portion"]
        screen, page = record["screen"], record["pageId"]
        background = model["pageImage"]["file"]
        variables = {key: variable_text(record["variables"][key]) for key in TRACE_VARIABLES}
        index, count, more = (portion[key] for key in ("index", "count", "more"))
        choices = [normalize(item["text"]) for item in model["choices"] if item["enabled"]]
        text = paragraphs_text(portion["visible"])
        if screen == "finished":
            screen = "end"
    else:
        screen = "end" if record["end_screen"] else record["screen_kind"]
        page = record["page_id"]
        background = record["background"].removeprefix("images/")
        variables = {key: record[key] for key in TRACE_VARIABLES}
        index, count, more = (record[key] for key in ("portion_index", "portion_count", "more"))
        text = record["rendered"]
        choices = [normalize(value) for _, value in CHOICE_LINK.findall(text)]
    return {"page": page, "index": index, "count": count, "more": more, "screen": screen,
            "background": str(Path(background).with_suffix("")) if background else background,
            "variables": variables, "visible": choices if screen == "choices" else normalize(text)}


def snapshot(case, checkpoint, baseline=None, terminal=False):
    states = result_states(case)
    rows = {side: scene(states[side][checkpoint], side) for side in SLOTS
            if checkpoint in states.get(side, {})}
    if terminal:
        for row in rows.values():
            del row["visible"]
    confirm(f"{case.name}/{checkpoint}/cross", len(rows) == 2 and rows[NLB] == rows[INSTEAD], rows)
    if baseline is not None:
        confirm(f"{case.name}/{checkpoint}/baseline", len(rows) == 2 and all(row == baseline for row in rows.values()), rows)
    return rows.get(NLB)


def commands(case, expected):
    raw = read_instead_trace(case / INSTEAD / "instead-state.tsv")
    events = [(r["command"].split(" ", 1)[0], r["seq"]) for r in raw
              if r["kind"] != "fmt" and r["command"].endswith("/save9")]
    confirm(case.name + "/INSTEAD-slot", bool(raw) and trace_sequence_valid(raw)
            and [kind for kind, _ in events] == expected, events)
    log = case / NLB / "state-log.jsonl"
    rows = [json.loads(line) for line in log.read_text().splitlines()] if log.is_file() else []
    actions = [("save" if prev["command"] == "key:f5" else "load", row["index"])
               for prev, row in zip(rows, rows[1:]) if prev["command"] in ("key:f5", "key:f9")
               and row["command"] == "menu:1" and prev["ok"] and row["ok"]]
    playthrough = case / NLB / "playthrough.log"
    text = playthrough.read_text() if playthrough.is_file() else ""
    written = text.count("Сохранить в слот 1")
    confirm(case.name + "/SDL-slot", bool(rows) and written == expected.count("save")
            and [kind for kind, _ in actions] == expected, actions)


def seeds_from(folder):
    sources = {side: folder / side / SLOTS[side] for side in SLOTS}
    for path in sources.values():
        assert path.is_file() and path.resolve() == path and path.is_relative_to(OUT), path
    return sources


def transfer(source_slots, target):
    entries = {}
    for side in SLOTS:
        source = source_slots[side]
        copy = target / side / SLOTS[side]
        original = source.read_bytes()
        copied = copy.read_bytes() if copy.is_file() else b""
        hashes = [hashlib.sha256(data).hexdigest() for data in (original, copied)]
        confirm(f"{target.name}/{side}/transfer", original == copied and bool(copied), hashes)
        entries[side] = dict(source=str(source.relative_to(ROOT)), copy=str(copy.relative_to(ROOT)),
                             bytes=len(copied), sha256=hashes[1])
    (target / "slots.json").write_text(json.dumps(entries, indent=2) + "\n", encoding="utf-8")


def targets():
    name = "choices"
    before = steps[:panel + 1]
    case = check_case(name, write_route(OUT, name, source, before + [
        save(expect=choice_expect), renamed(corpse_step, "go-to-corpse"),
        load(expect=choice_expect), save("resave-choices", choice_expect),
        renamed(doctor_step, "choose-doctor-after-load")
    ]))
    commands(case, ["save", "load", "save"])
    choice = snapshot(case, "hospital-choices")
    for label in ("save-a", "load-a", "resave-choices"):
        result = snapshot(case, label, choice)
        confirm(f"{name}/{label}/visible-order",
                bool(result and result["visible"] == ordered_choices and result["screen"] == "choices"
                     and result["index"] == 1 and result["count"] == 2 and result["more"] is False), result)
    snapshot(case, "choose-doctor-after-load")

    slots = seeds_from(case)
    name = "choices-restart"
    restart = check_case(name, write_route(OUT, name, source, [title, load(expect=choice_expect),
                         renamed(corpse_step, "choose-corpse-after-load")]), slots)
    commands(restart, ["load"])
    transfer(slots, restart)
    snapshot(restart, "load-a", choice)
    snapshot(restart, "choose-corpse-after-load")
    proof = result_states(restart).get(INSTEAD, {}).get("load-a", {})
    confirm("choices-restart/new-INSTEAD-process", bool(proof) and
            report(case / INSTEAD / "route-report.json")["process"]["pid"] !=
            report(restart / INSTEAD / "route-report.json")["process"]["pid"])


def full_matrix():
    base = check_case("baseline", write_route(OUT, "baseline", source, steps[:c + 1]), no_autosave=True)
    reference = {step["id"]: snapshot(base, step["id"]) for step in steps[a:c + 1]}
    commands(base, [])
    no_slots = not any((base / runtime / relative).exists() for runtime, relative in SLOTS.items())
    trace = read_instead_trace(base / INSTEAD / "instead-state.tsv")
    confirm("baseline/no-auto", no_slots and not any(
        r["kind"] != "fmt" and r["command"].split(" ", 1)[0] in ("save", "load") for r in trace))

    suffix = steps[a+1:b+1]
    prefix = steps[:a+1]
    flow = prefix + [save(expect=action_expect(steps[a]))] + [
        renamed(step, "baseline-" + step["id"]) for step in suffix
    ]
    for cycle in range(1, 4):
        flow += [load(f"load-a-{cycle}", action_expect(steps[a]))] + [
            renamed(step, f"cycle-{cycle}-" + step["id"]) for step in suffix]
    cycles = check_case("cycles", write_route(OUT, "cycles", source, flow))
    commands(cycles, ["save", "load", "load", "load"])
    snapshot(cycles, "save-a", reference[steps[a]["id"]])
    for cycle in range(1, 4):
        snapshot(cycles, f"load-a-{cycle}", reference[steps[a]["id"]])
        for step in suffix:
            snapshot(cycles, f"cycle-{cycle}-" + step["id"], reference[step["id"]])

    recorded = seeds_from(cycles)
    replay = [renamed(step, "replay-" + step["id"]) for step in suffix]
    restart = check_case("restart", write_route(OUT, "restart", source,
                         [title, load(expect=action_expect(steps[a]))] + replay), recorded)
    commands(restart, ["load"])
    transfer(recorded, restart)
    snapshot(restart, "load-a", reference[steps[a]["id"]])
    for step in suffix:
        snapshot(restart, "replay-" + step["id"], reference[step["id"]])

    for key, index, limit in (("a1", a1, b), ("b2", indices["doctor-page4-portion-2"], c),
                              ("b3", indices["doctor-page4-portion-3"], c)):
        prior = steps[index]
        expectation = action_expect(prior)
        expectation.update(screen="page", instead_screen_kind="page")
        tail = steps[index+1:limit+1]
        replay = [renamed(step, "replay-" + step["id"]) for step in tail]
        name = key + "-roundtrip"
        roundtrip = check_case(name, write_route(OUT, name, source, steps[:index+1]
                               + [save(expect=expectation)] + tail + [load(expect=expectation)] + replay))
        commands(roundtrip, ["save", "load"])
        snapshot(roundtrip, "save-a", reference[prior["id"]])
        saved = seeds_from(roundtrip)
        name = key + "-restart"
        restarted = check_case(name, write_route(OUT, name, source, [title, load(expect=expectation)] + replay), saved)
        commands(restarted, ["load"])
        transfer(saved, restarted)
        for case in (roundtrip, restarted):
            snapshot(case, "load-a", reference[prior["id"]])
            for step in tail:
                snapshot(case, "replay-" + step["id"], reference[step["id"]])

    resave = check_case("resave", write_route(OUT, "resave", source, [title,
                        load(expect=action_expect(steps[a])), save("resave-a", action_expect(steps[a]))]), recorded)
    commands(resave, ["load", "save"])
    for step in ("load-a", "resave-a"):
        snapshot(resave, step, reference[steps[a]["id"]])

    ending = load_route(ROUTES / "wolfman-forest-wolfiness-one-rowan-ending34.json")
    final = ending["steps"]
    expectation = action_expect(final[-2])
    case = check_case("ending", write_route(OUT, "ending", ending, final[:-1] +
                      [save(expect=expectation), load(expect=expectation), renamed(final[-1], "continue-ending")]))
    commands(case, ["save", "load"])
    ref = snapshot(case, final[-2]["id"])
    snapshot(case, "save-a", ref)
    snapshot(case, "load-a", ref)
    snapshot(case, "continue-ending", terminal=True)


if __name__ == "__main__":
    phase, = sys.argv[1:]
    OUT.mkdir(exist_ok=True)
    if phase == "red":
        route = write_route(OUT, "choices-red", source, steps[:panel+1] +
                            [save(expect=choice_expect), load(expect=choice_expect)])
        code = engine(route, OUT / "choices-red", NLB, 372)
        errors = report(OUT / "choices-red/nlbnavigator/route-report.json")["errors"]
        print(json.dumps({"code": code, "errors": errors}, ensure_ascii=False))
        assert code == 1 and len([e for e in errors if "ожидался экран choices, получен page" in e]) == 2
    else:
        {"choices": targets, "full": full_matrix}[phase]()
        (OUT / f"{phase}-semantic.json").write_text(json.dumps(checks, ensure_ascii=False, indent=2) + "\n")
        print(f"Проверок: {len(checks)}, успешных: {sum(x['ok'] for x in checks)}")
        raise SystemExit(int(not checks or not all(x["ok"] for x in checks)))
