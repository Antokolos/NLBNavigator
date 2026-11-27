"""Прогоны штатных маршрутов с отдельным профилем и журналом на команду."""
import concurrent.futures
import json
import os
import shlex
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
REPO = Path("/workspace/NLBNavigator")
ENV = Path("/workspace/nlb-environment")
DATA = ENV / "test-data"
PLAYER = ROOT / "build/nlb_sdl"
INSTEAD = ENV / "instead/bin/sdl-instead"
GAME = DATA / "instead-am_i_a_monster_vn"
THEME = DATA / "instead/themes/default"
BOOK = DATA / "wolfman"
RUNNER = REPO / "tests/differential/run_route.py"
NLBSLOT = Path(".nlbnavigator/wolfman.saves/slot1.sav")
ISLOT = Path("instead-appdata/saves/instead-game-overlay/save9")


def canonical(path):
    path = Path(path)
    if path.resolve() != path or not path.exists():
        raise RuntimeError(f"Неканонический путь: {path}")
    return path


def run(command, out, display=None, limit=None, logfile="command"):
    out.mkdir(parents=True, exist_ok=True)
    canonical(out)
    profile = out / (logfile + "-outer-profile")
    profile.mkdir(mode=0o700)
    for name in ("config", "data", "cache", "runtime", "tmp"):
        (profile / name).mkdir(mode=0o700)
    env = dict(os.environ, HOME=str(profile), USERPROFILE=str(profile),
               XDG_CONFIG_HOME=str(profile / "config"), XDG_DATA_HOME=str(profile / "data"),
               XDG_CACHE_HOME=str(profile / "cache"), XDG_RUNTIME_DIR=str(profile / "runtime"),
               APPDATA=str(profile / "data"), LOCALAPPDATA=str(profile / "cache"), TMPDIR=str(profile / "tmp"),
               PYTHONDONTWRITEBYTECODE="1", LP_NUM_THREADS="1", OMP_NUM_THREADS="1")
    if display is not None:
        command = ["xvfb-run", "-a", "-n", str(display), "-s", "-screen 0 1920x1080x24", *command]
    if limit is not None:
        command = ["timeout", "--signal=TERM", "--kill-after=3s", str(limit), *command]
    (out / (logfile + ".command")).write_text(shlex.join(map(str, command)) + "\n", encoding="utf-8")
    with (out / (logfile + ".log")).open("x", encoding="utf-8") as stream:
        code = subprocess.run(command, env=env, cwd=REPO, stdout=stream, stderr=subprocess.STDOUT).returncode
    (out / (logfile + ".exit")).write_text(str(code) + "\n", encoding="utf-8")
    print(f"{out.relative_to(ROOT)} {logfile}: exit={code}", flush=True)
    return code


def engine(route, root, runtime, display, seed=None):
    out = root / runtime
    out.mkdir(parents=True)
    command = [sys.executable, RUNNER, "--runtime", runtime, "--route", route, "--output", out, "--timeout", "240"]
    if runtime == "nlbnavigator":
        command += ["--player", PLAYER, "--book", BOOK, "--theme", GAME / "themes/default", "--base-theme", THEME]
        if seed:
            command += ["--seed-home", f"{NLBSLOT}={canonical(seed)}"]
    else:
        command += ["--instead-binary", INSTEAD, "--game", GAME, "--base-theme", THEME]
        if seed:
            command += ["--instead-seed-save", canonical(seed)]
    return run(command, out, display, limit=340)


def stock_compare(route, root):
    return run([sys.executable, RUNNER, "--runtime", "compare", "--route", route, "--output", root],
               root, logfile="compare", limit=180)


def pair(route, root, display, seeds=None):
    seeds = seeds or {}
    states = {}
    for runtime in ("nlbnavigator", "instead"):
        states[runtime] = engine(route, root, runtime, display, seeds.get(runtime))
    states["compare"] = stock_compare(route, root)
    (root / "exits.json").write_text(json.dumps(states, indent=2) + "\n")
    return states


def regression():
    entries = json.loads((REPO / "tests/differential/catalog.json").read_text())["routes"]
    assert len(entries) == 22
    def lane(index):
        return {e["id"]: pair(REPO / e["path"], ROOT / "regression" / e["id"], 200 + 20 * index)
                for e in entries[index::2]}
    with concurrent.futures.ThreadPoolExecutor(max_workers=2) as executor:
        results = dict(pair for res in executor.map(lane, range(2)) for pair in res.items())
    (ROOT / "regression/results.json").write_text(json.dumps(results, indent=2) + "\n")
    return int(any(any(row.values()) for row in results.values()))


if __name__ == "__main__":
    for path in (ROOT, REPO, PLAYER, INSTEAD, BOOK, GAME, THEME):
        canonical(path)
    if (GAME / "saves").exists():
        raise RuntimeError("Исходный экспорт содержит каталог saves")
    sys.exit(regression())
