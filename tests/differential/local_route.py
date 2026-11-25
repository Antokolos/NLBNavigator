#!/usr/bin/env python3
"""Один маршрут локально: INSTEAD → NLBNavigator → сравнение (как шаги CI).

Платформенно-нейтральный запуск: на Linux используется Xvfb-путь раннера,
на Windows — оконный драйвер (tests/differential/win_gui.py).
"""
import argparse
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent


def prepare_output(path):
    """Новый каталог результатов; существующий непустой отклоняется."""
    path = Path(path)
    if path.exists() and (not path.is_dir() or any(path.iterdir())):
        raise SystemExit("каталог результатов уже существует: {}; укажите новый".format(path))
    path.mkdir(parents=True, exist_ok=True)
    return path


def run(command):
    print("+", " ".join(str(part) for part in command), flush=True)
    return subprocess.run(command, cwd=HERE.parents[1]).returncode


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--route", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path,
                        help="каталог маршрута; внутри появятся instead/ и nlbnavigator/")
    parser.add_argument("--book", required=True, type=Path)
    parser.add_argument("--game", required=True, type=Path)
    parser.add_argument("--theme", required=True, type=Path)
    parser.add_argument("--base-theme", required=True, type=Path)
    parser.add_argument("--player", required=True, type=Path)
    parser.add_argument("--instead-binary", type=Path)
    parser.add_argument("--timeout", type=int, default=420)
    args = parser.parse_args()

    route = str(args.route.resolve())
    output = prepare_output(args.output).resolve()
    instead_dir = output / "instead"
    nlb_dir = output / "nlbnavigator"
    instead_dir.mkdir(parents=True, exist_ok=True)
    nlb_dir.mkdir(parents=True, exist_ok=True)

    instead = [sys.executable, str(HERE / "run_route.py"), "--runtime", "instead",
               "--route", route, "--output", str(instead_dir),
               "--game", str(args.game.resolve()), "--base-theme", str(args.base_theme.resolve())]
    if args.instead_binary:
        instead += ["--instead-binary", str(args.instead_binary.resolve())]
    if run(instead) != 0:
        print("INSTEAD: FAILED", flush=True)
        return 1

    nlb = [sys.executable, str(HERE / "run_route.py"), "--runtime", "nlbnavigator",
           "--route", route, "--output", str(nlb_dir), "--timeout", str(args.timeout),
           "--player", str(args.player.resolve()), "--book", str(args.book.resolve()),
           "--theme", str(args.theme.resolve()), "--base-theme", str(args.base_theme.resolve())]
    if run(nlb) != 0:
        print("NLBNavigator: FAILED", flush=True)
        return 1

    compare = [sys.executable, str(HERE / "run_route.py"), "--runtime", "compare",
               "--route", route, "--output", str(output)]
    if run(compare) != 0:
        print("compare: FAILED", flush=True)
        return 1
    print("OK:", args.route.stem, flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
