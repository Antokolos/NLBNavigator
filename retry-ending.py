"""Проба концовки по уже развёрнутым шагам штатного маршрута."""
import sys
from execute import ROOT, REPO, pair
sys.path.insert(0, str(REPO / "tests/differential"))
from check_wolfman_save_portion import load, renamed, save, write_route
from run_route import load_route

ending = load_route(REPO / "tests/differential/routes/wolfman-forest-wolfiness-one-rowan-ending34.json")
ending.pop("prefix", None)
steps = ending["steps"]
out = ROOT / "matrix"
route = write_route(out, "ending-verified", ending,
                    steps[:-1] + [save(), load(), renamed(steps[-1], "continue-ending")])
pair(route, out / "ending", 140)
