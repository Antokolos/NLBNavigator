"""Прогоны штатных маршрутов с отдельным профилем и журналом на команду."""
import json
import os
import shlex
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
REPO = ROOT / "repo"
ENV = Path("/workspace/nlb-developer-env-v1")
DATA = Path("/workspace/nlb-acceptance.01Jw9LE0/nav-data/test-data")
PLAYER = Path(os.environ.get("NLB_TEST_PLAYER", ROOT / "build/nlb_sdl"))
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
    assert out.is_relative_to(ROOT) and out.resolve() == out
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
               PYTHONDONTWRITEBYTECODE="1", LP_NUM_THREADS="2", OMP_NUM_THREADS="2",
               MAGICK_THREAD_LIMIT="2", SDL_AUDIODRIVER="dummy", SDL_VIDEODRIVER="x11",
               SDL_RENDER_DRIVER="opengl", NLB_CLICK_PAUSE="0.27",
               PATH=f"{ROOT}/capture-tools:{os.environ['PATH']}")
    if display is not None:
        if "instead" in command:
            env["PATH"] = f"{ROOT}/wm-tools:{ROOT}/slow-input:{env['PATH']}"
        else:
            command = ["bash", ROOT / "run-final.sh", "x11", out, *command]
        command = ["xvfb-run", "-a", "-n", str(display), "-e", out / (logfile + ".xvfb.log"),
                   "-s", "-screen 0 1920x1080x24 -noreset", *command]
    if limit is not None:
        command = ["timeout", "--signal=TERM", "--kill-after=3s", str(limit), *command]
    (out / (logfile + ".command")).write_text(shlex.join(map(str, command)) + "\n", encoding="utf-8")
    with (out / (logfile + ".log")).open("x", encoding="utf-8") as stream:
        code = subprocess.run(command, env=env, cwd=REPO, stdout=stream, stderr=subprocess.STDOUT).returncode
    (out / (logfile + ".exit")).write_text(str(code) + "\n", encoding="utf-8")
    print(f"{out.relative_to(ROOT)} {logfile}: exit={code}", flush=True)
    return code


def engine(route, root, runtime, display, seed=None, instead_player=INSTEAD):
    out = root / runtime
    out.mkdir(parents=True)
    command = [sys.executable, RUNNER, "--runtime", runtime, "--route", route, "--output", out, "--timeout", "240"]
    if runtime == "nlbnavigator":
        command += ["--player", PLAYER, "--book", BOOK, "--theme", GAME / "themes/default", "--base-theme", THEME]
        if seed:
            command += ["--seed-home", f"{NLBSLOT}={canonical(seed)}"]
    else:
        command += ["--instead-binary", instead_player, "--game", GAME, "--base-theme", THEME]
        if seed:
            command += ["--instead-seed-save", canonical(seed)]
    return run(command, out, display, limit=720)


def stock_compare(route, root):
    return run([sys.executable, RUNNER, "--runtime", "compare", "--route", route, "--output", root],
               root, logfile="compare", limit=380)


def pair(route, root, display, seeds=None, instead_player=INSTEAD):
    seeds = seeds or {}
    states = {}
    for runtime in ("nlbnavigator", "instead"):
        states[runtime] = engine(route, root, runtime, display, seeds.get(runtime), instead_player)
    states["compare"] = stock_compare(route, root)
    (root / "exits.json").write_text(json.dumps(states, indent=2) + "\n")
    return states

