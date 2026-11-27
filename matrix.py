"""Матрица D1/D4 и дополнительные сценарии поверх штатного раннера."""
import json
import sys

from execute import ROOT, REPO, pair, NLBSLOT, ISLOT

sys.path.insert(0, str(REPO / "tests/differential"))
from check_wolfman_save_portion import load, renamed, save, write_route
from run_route import load_route

OUT = ROOT / "matrix"
OUT.mkdir()
source = load_route(REPO / "tests/differential/routes/wolfman-police-hospital-doctor.json")
steps = source["steps"]
indices = {step["id"]: index for index, step in enumerate(steps)}
keys = ("hospital-page3", "hospital-page3-last", "doctor-page4-portion-2", "doctor-page4-portion-3")
b, c = indices["eat-doctor"], indices["page5-after-doctor"]
book_title = steps[0]
baseline = write_route(OUT, "baseline", source, steps[:c + 1])
pair(baseline, OUT / "baseline", 140)
cases = {}
for name, target in zip(("a0", "a1", "b2", "b3"), keys):
    if name == "a0":
        continue  # Эту позицию покрывают D2/D3 штатных сценариев и три цикла.
    index = indices[target]
    page = steps[index]["expect"]["page_id"]
    tail = steps[index + 1:(b if name.startswith("a") else c) + 1]
    walked = [renamed(step, step["id"]) for step in tail]
    replay = [renamed(step, "replay-" + step["id"]) for step in tail]
    roundtrip = write_route(OUT, name + "-roundtrip", source,
                            steps[:index + 1] + [save()] + walked + [load(expect={"page_id": page})] + replay)
    out = OUT / roundtrip.stem
    states = pair(roundtrip, out, 140)
    seeds = {"nlbnavigator": out / "nlbnavigator/nlbnavigator-home" / NLBSLOT,
             "instead": out / "instead" / ISLOT}
    restart = write_route(OUT, name + "-restart", source, [book_title, load(expect={"page_id": page})] + replay)
    ready = {runtime: slot.is_file() for runtime, slot in seeds.items()}
    if all(ready.values()):
        pair(restart, OUT / restart.stem, 140, seeds)
    else:
        (OUT / (restart.stem + ".blocked.json")).write_text(json.dumps(ready) + "\n")
    cases[name] = {"baseline": target, "follow": [step["id"] for step in tail]}
(OUT / "cases.json").write_text(json.dumps(cases, indent=2) + "\n")

idx = indices["hospital-choices"]
route = write_route(OUT, "choices", source, steps[:idx + 1] + [save(), load()])
pair(route, OUT / "choices", 140)

ending = load_route(REPO / "tests/differential/routes/wolfman-forest-wolfiness-one-rowan-ending34.json")
ending_steps = ending["steps"]
route = write_route(OUT, "ending", ending,
                    ending_steps[:-1] + [save(), load(), renamed(ending_steps[-1], "continue-ending")])
pair(route, OUT / "ending", 140)
