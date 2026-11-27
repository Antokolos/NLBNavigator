"""Проверяет отсутствие записей слота и тождественность состояния baseline INSTEAD."""
import json
import sys

from execute import ROOT, REPO, GAME, RUNNER, THEME, run, stock_compare

matrix = ROOT / "matrix"
label = sys.argv[1] if len(sys.argv) > 1 else "baseline-noautosave"
assert label in ("baseline-noautosave", "baseline-noautosave-retry")
target = matrix / label
target.mkdir()
(target / "nlbnavigator").symlink_to(matrix / "baseline/nlbnavigator", target_is_directory=True)
path = target / "instead"
route = matrix / "baseline.json"
code = run([sys.executable, RUNNER, "--runtime", "instead", "--route", route, "--output", path,
            "--instead-binary", ROOT / "instead-no-autosave.py", "--game", GAME, "--base-theme", THEME],
           path, 440, limit=300)
if code:
    raise SystemExit(code)
sys.path.insert(0, str(REPO / "tests/differential"))
from run_route import TRACE_VARIABLES, normalize_story_text, read_instead_trace
trace = read_instead_trace(path / "instead-state.tsv")
commands = [item["command"] for item in trace if item["command"].split(" ")[0] in ("save", "load")]
files = sorted(str(item.relative_to(path)) for item in (path / "instead-appdata/saves").rglob("*") if item.is_file())
fields = ("step_id", "page_id", "screen_kind", "rendered", "background", "portion_index",
          "portion_count", "more", "end_screen", *TRACE_VARIABLES)

def project(directory):
    report = json.loads((directory / "route-report.json").read_text())
    result = [{field: step[field] for field in fields} for step in report["state_records"]]
    for step in result:
        step["rendered"] = normalize_story_text(step["rendered"]).rstrip("\n")
    return result

earlier, fresh = project(matrix / "baseline/instead"), project(path)
same = earlier == fresh
comparison = stock_compare(route, target)
report = {"instead_exit": code, "stock_compare_exit": comparison, "save_or_load_trace_events": commands,
          "slot_files": files, "identical_to_first_instead_baseline": same, "checkpoints": len(fresh)}
(target / "no-autosave-proof.json").write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n")
assert same and not commands and not files and not comparison, report
