#!/usr/bin/env python3
"""INSTEAD save/load: B5 (Ð¿ÐµÑ€ÐµÐ·Ð°Ð¿ÑƒÑÐº Ð¿Ñ€Ð¾Ñ†ÐµÑÑÐ°) Ð¸ B6 (Ð¿Ð¾Ð²Ñ‚Ð¾Ñ€Ð½Ñ‹Ðµ Ñ†Ð¸ÐºÐ»Ñ‹) â€” issue #69.

Ð¡Ñ†ÐµÐ½Ð°Ñ€Ð¸Ð¸ ÑÑ‚Ñ€Ð¾ÑÑ‚ÑÑ Ð¸Ð· Ð¼Ð°Ñ€ÑˆÑ€ÑƒÑ‚Ð° wolfman-police-hospital-doctor:
- ÑÐ¾Ñ…Ñ€Ð°Ð½Ð¸Ñ‚ÑŒ A (F8), Ð²Ñ‹Ð¹Ñ‚Ð¸, Ð·Ð°Ð¿ÑƒÑÑ‚Ð¸Ñ‚ÑŒ Ð½Ð¾Ð²Ñ‹Ð¹ INSTEAD Ð¸ Ð·Ð°Ð³Ñ€ÑƒÐ·Ð¸Ñ‚ÑŒ ÑÐ»Ð¾Ñ‚ (F9);
- Ð½ÐµÑÐºÐ¾Ð»ÑŒÐºÐ¾ Ñ†Ð¸ÐºÐ»Ð¾Ð² save/load Ð² Ð¾Ð´Ð½Ð¾Ð¼ Ð¿Ñ€Ð¾Ñ†ÐµÑÑÐµ (A â†’ save â†’ B â†’ load â†’ A â†’ save â†’ B â†’ load).
ÐŸÑ€Ð¾Ð²ÐµÑ€ÑÐµÑ‚ÑÑ ÐºÐ¾Ð´ Ñ€Ð°Ð½Ð½ÐµÑ€Ð° Ð¸ Ð¾Ñ‚Ñ‡Ñ‘Ñ‚Ñ‹ Ð¼Ð°Ñ€ÑˆÑ€ÑƒÑ‚Ð¾Ð²; trace-Ð¾ÑˆÐ¸Ð±ÐºÐ¸ (Ð²ÐºÐ»ÑŽÑ‡Ð°Ñ Ð¿ÐµÑ€ÐµÐ¸Ð½Ð¸Ñ†Ð¸Ð°Ð»Ð¸Ð·Ð°Ñ†Ð¸ÑŽ) Ð½Ðµ
Ð´Ð¾Ð¿ÑƒÑÐºÐ°ÑŽÑ‚ÑÑ.
"""
import argparse
import json
import shutil
import subprocess
import sys
from pathlib import Path

from run_route import load_route

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
ROUTE = HERE / "routes/wolfman-police-hospital-doctor.json"
A_PAGE = "8c1fa8c6-8027-43c8-8d8b-0bf454d5e717"


def renamed(step, name, expect=None):
    copy = dict(step, id=name, screenshot=f"{name}.png")
    if expect is not None:
        copy["expect"] = expect
    copy.pop("visual_change_from", None)
    return copy


def menu(name, key, expect):
    return {"id": name, "screenshot": f"{name}.png", "settle_ms": 2000,
            "action": {"type": "menu", "nlbnavigator": (["f5"] if key == "F8" else ["f9", 1]), "instead": [{"key": key}]},
            "expect": expect}


def write_route(output, name, route, steps):
    path = output / f"{name}.json"
    path.write_text(json.dumps(dict(route, id=f"wolfman-instead-save-load-{name}", steps=steps),
                               ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    load_route(path)
    return path


def run(route, output, args, seed=None):
    command = [sys.executable, str(HERE / "run_route.py"), "--runtime", "instead",
               "--route", str(route), "--output", str(output),
               "--game", str(args.game), "--base-theme", str(args.base_theme),
               "--instead-binary", str(args.instead_binary)]
    if seed:
        command += ["--instead-seed-appdata", str(seed)]
    log = output / "runner.log"
    output.mkdir(parents=True, exist_ok=True)
    with log.open("w", encoding="utf-8") as stream:
        code = subprocess.run(command, cwd=REPO, stdout=stream, stderr=subprocess.STDOUT).returncode
    report = json.loads((output / "route-report.json").read_text(encoding="utf-8")) \
        if (output / "route-report.json").is_file() else {}
    return code, report


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--game", type=Path, required=True)
    parser.add_argument("--base-theme", type=Path, required=True)
    parser.add_argument("--instead-binary", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)

    source = load_route(ROUTE)
    steps = source["steps"]
    ids = [step["id"] for step in steps]
    a = ids.index("hospital-page3")
    b = ids.index("eat-doctor")
    prefix = steps[:a + 1]
    def portion(tag):
        return [renamed(step, f"{tag}-{step['id']}") for step in steps[a + 1:b + 1]]

    after_portion = portion("cycle")  # ÑÐºÑ€Ð°Ð½ Ð²Ð°Ñ€Ð¸Ð°Ð½Ñ‚Ð¾Ð² Ð¸ Â«ÑÑŠÐµÑÑ‚ÑŒ Ð²Ñ€Ð°Ñ‡Ð°Â»

    b_expect = {"page_id": "d937c5c1-766c-4581-a03e-736560b0cc8f",
                "instead_page_id": "d937c5c1-766c-4581-a03e-736560b0cc8f"}
    save_expect = {"page_id": A_PAGE, "instead_page_id": A_PAGE}
    load_expect = {"page_id": A_PAGE, "instead_page_id": A_PAGE}
    save = menu("save-a", "F8", save_expect)
    load = menu("load-a", "F9", load_expect)

    # B5: ÑÐ¾Ñ…Ñ€Ð°Ð½ÐµÐ½Ð¸Ðµ, Ð·Ð°Ð²ÐµÑ€ÑˆÐµÐ½Ð¸Ðµ Ð¿Ñ€Ð¾Ñ†ÐµÑÑÐ°, Ð½Ð¾Ð²Ñ‹Ð¹ Ð¿Ñ€Ð¾Ñ†ÐµÑÑ Ñ Ñ‚ÐµÐ¼ Ð¶Ðµ appdata
    save_output = args.output / "restart-save"
    save_route = write_route(args.output, "restart-save", source, prefix + [save])
    code, report = run(save_route, save_output, args)
    require(code == 0, f"restart-save: ÐºÐ¾Ð´ {code}, Ð¾ÑˆÐ¸Ð±ÐºÐ¸ {report.get('errors')}")
    appdata = save_output / "instead-appdata"
    require((appdata / "saves" / "instead-game-overlay" / "save9").is_file(),
            "restart-save: INSTEAD Ð½Ðµ ÑÐ¾Ð·Ð´Ð°Ð» save9")
    seed = args.output / "seed-appdata"
    if seed.exists():
        shutil.rmtree(seed)
    shutil.copytree(appdata, seed)

    load_output = args.output / "restart-load"
    load_path = write_route(args.output, "restart-load", source,
                             [steps[0], load] + after_portion)
    code, report = run(load_path, load_output, args, seed)
    require(code == 0, f"restart-load: ÐºÐ¾Ð´ {code}, Ð¾ÑˆÐ¸Ð±ÐºÐ¸ {report.get('errors')}")
    require(report.get("ok") and not report.get("errors"),
            "restart-load: Ð¾Ñ‚Ñ‡Ñ‘Ñ‚ Ð½Ðµ ok")

    # B6: Ð¿Ð¾Ð²Ñ‚Ð¾Ñ€Ð½Ñ‹Ðµ Ñ†Ð¸ÐºÐ»Ñ‹ save/load Ð² Ð¾Ð´Ð½Ð¾Ð¼ Ð¿Ñ€Ð¾Ñ†ÐµÑÑÐµ
    cycles_output = args.output / "cycles"
    cycles_route = write_route(
        args.output, "cycles", source,
        prefix + [save] + portion("cycle1") + [load] + portion("cycle2")
        + [menu("save-b", "F8", b_expect), menu("load-b", "F9", b_expect)])
    code, report = run(cycles_route, cycles_output, args)
    require(code == 0, f"cycles: ÐºÐ¾Ð´ {code}, Ð¾ÑˆÐ¸Ð±ÐºÐ¸ {report.get('errors')}")
    require(report.get("ok") and not report.get("errors"), "cycles: Ð¾Ñ‚Ñ‡Ñ‘Ñ‚ Ð½Ðµ ok")
    print("OK: INSTEAD save/load Ð²Ñ‹Ð´ÐµÑ€Ð¶Ð¸Ð²Ð°ÐµÑ‚ Ð¿ÐµÑ€ÐµÐ·Ð°Ð¿ÑƒÑÐº Ð¸ Ð¿Ð¾Ð²Ñ‚Ð¾Ñ€Ð½Ñ‹Ðµ Ñ†Ð¸ÐºÐ»Ñ‹")


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, ValueError, KeyError) as error:
        sys.exit(str(error))


