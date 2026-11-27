"""Повторное использование штатных save/load-сценариев и проверка записи сразу после загрузки."""
import sys
import time

from execute import ROOT, REPO, PLAYER, BOOK, GAME, THEME, ISLOT, NLBSLOT, engine, pair, run, stock_compare

sys.path.insert(0, str(REPO / "tests/differential"))
from check_wolfman_save_portion import load, save, write_route
from run_route import load_route

out = ROOT / "standard"
out.mkdir()
instead = ROOT / "instead-standard"
cycles = instead / "cycles.json"
for _ in range(120):
    if (instead / "cycles/route-report.json").exists():
        break
    time.sleep(2)
else:
    sys.exit("Нет отчёта INSTEAD cycles")
paired = out / "cycles"
paired.mkdir()
(paired / "instead").symlink_to(instead / "cycles", target_is_directory=True)
engine(cycles, paired, "nlbnavigator", 170)
stock_compare(cycles, paired)

common = ["--player", PLAYER, "--book", BOOK, "--theme", GAME / "themes/default", "--base-theme", THEME]
for name in ("save_load", "save_portion"):
    command = [sys.executable, REPO / f"tests/differential/check_wolfman_{name}.py",
               *common, "--output", out / name]
    run(command, out, 170, limit=650, logfile=name)

source = load_route(REPO / "tests/differential/routes/wolfman-police-hospital-doctor.json")
a = source["steps"][5]
load_a = load(expect={"page_id": a["expect"]["page_id"]})
route = write_route(out, "resave", source, [source["steps"][0], load_a, save("resave-a")])
seeds = {"nlbnavigator": out / "save_load/save-only/nlbnavigator-home" / NLBSLOT,
         "instead": instead / "restart-save" / ISLOT}
if all(slot.is_file() for slot in seeds.values()):
    pair(route, out / "resave", 170, seeds)
else:
    print("Повторная запись заблокирована: отсутствует исходный слот", flush=True)
