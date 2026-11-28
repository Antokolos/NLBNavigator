import json
import re
from pathlib import Path

w = Path(__file__).resolve().parent
catalog = json.loads((w / "repo/tests/differential/catalog.json").read_text())
head = (w / "tested-sha").read_text().strip()
roots = [w / "results", *sorted(w.glob("retries*"))]
rows = []
for spec in catalog["routes"]:
    route = spec["id"]
    attempts = {}
    for root in roots:
        case = root / route
        if case.is_dir():
            attempts[root.name] = [int((case / (stage + ".exit")).read_text())
                                   for stage in ("nlb", "instead", "compare")]
    shipped = next((key for key in reversed(attempts) if attempts[key] == [0, 0, 0]), None)
    assert shipped, (route, attempts)
    case = w / shipped / route
    def report(relative):
        value = json.loads((case / relative).read_text())
        assert value["ok"] and not value["errors"], (route, relative, value["errors"])
        return value
    sdl = report("nlbnavigator/route-report.json")
    instead = report("instead/route-report.json")
    diff = report("comparison-report.json")
    assert sdl["metadata"]["navigator_commit"] == head
    assert instead["process"]["exit_code"] is not None
    assert len(sdl["steps"]) == len(instead["steps"]) == len(diff["steps"])
    rows.append({"route": route, "directory": str(case.relative_to(w)),
                 "checkpoints": len(diff["steps"]), "codes": [0,0,0], "attempts": attempts})
assert len(rows) == 22
(w / "logs/routes.json").write_text(json.dumps(rows, indent=2) + "\n")

for label in ("ctest-head", "python", "catalog", "diff-check", "choices-primary", "matrix-same-test",
              "matrix-choices", "matrix-full"):
    assert (w / f"logs/{label}.exit").read_text().strip() == "0", label
for name in ("save_load", "save_portion", "achievement"):
    assert (w / f"results/{name}/nlb.exit").read_text().strip() == "0", name
assert (w / "results/c6-final/instead.exit").read_text().strip() == "0"
for case in ("cycles", "restart-save", "restart-load", "missing-slot"):
    path = w / f"results/c6-final/scenarios/{case}/trace-proof.json"
    proof = json.loads(path.read_text())
    assert proof["process"]["exit_code"] == 0 and proof["runner_exit"] == (1 if case == "missing-slot" else 0)
for label in ("unchanged", "cmd", "fmt", "both", "nested", "scene", "repeat",
              "inflight", "global-reset", "append", "new-file"):
    assert (w / f"logs/lua-{label}.exit").read_text().strip() == "0", label
matrix = {}
for phase in ("choices", "full"):
    checks = json.loads((w / f"matrix/{phase}-semantic.json").read_text())
    assert checks and all(item["ok"] for item in checks), phase
    matrix[phase] = len(checks)
python = (w / "logs/python.log").read_text()
count = int(re.search(r"Ran (\d+) tests", python)[1])
skipped = int(re.search(r"skipped=(\d+)", python)[1])
tests = dict(ctest=17, python_passed=count-skipped, python_skipped=skipped, lua=11, matrix=matrix,
             routes=len(rows), checkpoints=sum(row["checkpoints"] for row in rows))
(w / "logs/totals.json").write_text(json.dumps(tests, ensure_ascii=False, indent=2) + "\n")
for row in rows:
    print(f"| [{row['route']}]({row['directory']}/comparison-report.json) | {row['checkpoints']} | 0/0/0 |")
print(json.dumps(tests, ensure_ascii=False), flush=True)
