#!/usr/bin/env python3
"""Какие маршруты каталога прогонять в CI: матрица для GitHub Actions.

  select_routes.py --all                      все маршруты (ручной запуск, расписание)
  select_routes.py --only id1,id2             только эти (ручной запуск с выбором маршрутов)
  select_routes.py --base <sha> --head <sha>  по изменённым файлам (push, pull request)

По изменённым файлам: изменился сам маршрут (JSON) — только он; изменились раннер, workflow, каталог,
ядро и SDL-фронтенд плеера (Source/NLBCore, NLBReader, NLBPresentation,
NLBSdl, NLBAudioSDL, thirdparty; CMakeLists.txt) или файлы CI — все маршруты; изменилась только документация — ни
одного (статическая проверка всё равно идёт). Если base неизвестен (новая ветка, сдвиг истории) — все.

Пишет в $GITHUB_OUTPUT (или в stdout): matrix={"include": [...]} и count=N. У каждого маршрута —
id, path и таймауты по числу шагов (снимки INSTEAD занимают ~6 с на шаг на hosted runners).
"""
import argparse
import json
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
CATALOG = ROOT / "tests/differential/catalog.json"
ROUTES_DIR = "tests/differential/routes/"
# изменения, после которых нужно прогнать всё
ALL_PREFIXES = (
    ".github/workflows/nlb-sdl-test.yml",
    "CMakeLists.txt",
    "Source/NLBCore/",
    "Source/NLBReader/",
    "Source/NLBPresentation/",
    "Source/NLBSdl/",
    "Source/NLBAudioSDL/",
    "Source/thirdparty/",
    "tests/differential/ci/",
    "tests/differential/catalog.json",
    "tests/differential/run_route.py",
    "tests/differential/route_validation.py",
    "tests/differential/instead_trace.lua",
)


def load_routes():
    catalog = json.loads(CATALOG.read_text(encoding="utf-8"))
    return catalog["routes"]


def timeouts(path):
    route = json.loads((ROOT / path).read_text(encoding="utf-8"))
    steps = len(route.get("steps", []))
    # INSTEAD: снимок экрана и trace на каждом шаге; nlb_sdl быстрее
    return {"instead_timeout": 120 + 8 * steps, "nlb_timeout": 90 + 5 * steps}


def changed_files(base, head):
    if not base or set(base) == {"0"}:
        return None
    result = subprocess.run(["git", "diff", "--name-only", base, head], cwd=ROOT,
                            capture_output=True, text=True)
    if result.returncode != 0:
        print("git diff не удался ({}): прогоняю все маршруты".format(result.stderr.strip()), file=sys.stderr)
        return None
    return [line for line in result.stdout.splitlines() if line]


def is_doc(path):
    """Документ (.md): на маршруты не влияет"""
    return path.lower().endswith(".md")


def docs_only(files):
    """Изменились только документы (известный непустой список .md)"""
    return bool(files) and all(is_doc(f) for f in files)


def select(routes, files):
    if files is None:
        return routes, "все маршруты: база сравнения неизвестна"
    files = [f for f in files if not is_doc(f)]
    if any(f.startswith(ALL_PREFIXES) or (f.startswith("Source/") and f.count("/") == 1) for f in files):
        return routes, "все маршруты: изменились раннер, workflow, каталог или плеер"
    changed = set(f for f in files if f.startswith(ROUTES_DIR) and f.endswith(".json"))
    chosen = [r for r in routes if r["path"] in changed]
    return chosen, "изменённые маршруты: {}".format(", ".join(r["id"] for r in chosen) or "нет")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--all", action="store_true")
    parser.add_argument("--only", default="")
    parser.add_argument("--base", default="")
    parser.add_argument("--head", default="HEAD")
    # PR: если в последнем push (от этого коммита до --docs-head) одни документы, маршруты не
    # прогоняются — иначе каждый коммит с документами в PR заново гоняет все его маршруты
    parser.add_argument("--docs-only-since", default="")
    parser.add_argument("--docs-head", default="")
    args = parser.parse_args()

    routes = load_routes()
    if args.only.strip():
        wanted = [x.strip() for x in args.only.split(",") if x.strip()]
        unknown = [x for x in wanted if x not in {r["id"] for r in routes}]
        if unknown:
            sys.exit("Нет в catalog.json: " + ", ".join(unknown))
        chosen, why = [r for r in routes if r["id"] in wanted], "выбраны вручную"
    elif args.all:
        chosen, why = routes, "все маршруты"
    elif args.docs_only_since and docs_only(changed_files(args.docs_only_since, args.docs_head or args.head)):
        chosen, why = [], "последний push — только документы (.md): маршруты не прогонялись"
    else:
        chosen, why = select(routes, changed_files(args.base, args.head))

    include = [dict(id=r["id"], path=r["path"], **timeouts(r["path"])) for r in chosen]
    print("{} ({})".format(why, len(include)), file=sys.stderr)
    lines = ["matrix=" + json.dumps({"include": include}, ensure_ascii=False), "count={}".format(len(include)),
             "reason={} ({})".format(why, len(include))]
    output = os.environ.get("GITHUB_OUTPUT")
    if output:
        with open(output, "a", encoding="utf-8") as stream:
            stream.write("\n".join(lines) + "\n")
    else:
        print("\n".join(lines))


if __name__ == "__main__":
    main()
