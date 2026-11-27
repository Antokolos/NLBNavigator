"""Повторяет INSTEAD после сбоя снимка X11, используя завершённый SDL-прогон."""
import json

from execute import ROOT, REPO, engine, stock_compare

name = "wolfman-guard-high-wolves-talk-ending38"
first = ROOT / "regression-tail" / name
codes = json.loads((first / "exits.json").read_text())
assert codes["nlbnavigator"] == 0 and codes["instead"] == 1
target = ROOT / "regression-retry" / name
target.mkdir()
(target / "nlbnavigator").symlink_to(first / "nlbnavigator", target_is_directory=True)
route = REPO / "tests/differential/routes" / (name + ".json")
retry_codes = {"instead": engine(route, target, "instead", 400)}
retry_codes["compare"] = stock_compare(route, target)
(target / "exits.json").write_text(json.dumps(retry_codes, indent=2) + "\n")
raise SystemExit(int(any(retry_codes.values())))
