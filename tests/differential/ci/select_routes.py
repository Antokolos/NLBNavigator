#!/usr/bin/env python3
"""Какие задания CI выполнять: матрица маршрутов и проверка достижений для GitHub Actions.

  select_routes.py --all                      все маршруты (ручной запуск, расписание)
  select_routes.py --only id1,id2             только эти (ручной запуск с выбором маршрутов)
  select_routes.py --base <sha> --head <sha>  по изменённым файлам (push, pull request)

По изменённым файлам: изменился сам маршрут (JSON) — только он; изменился фрагмент общего
префикса (routes/fragments/*.json) — все маршруты с ним в цепочке prefix; изменились раннер, workflow, каталог,
ядро и SDL-фронтенд плеера (Source/NLBCore, NLBReader, NLBPresentation,
NLBSdl, NLBAudioSDL, thirdparty; CMakeLists.txt) или файлы CI — все маршруты; изменилась только документация — ни
одного (статическая проверка всё равно идёт). Если base неизвестен (новая ветка, сдвиг истории) — все.

Для pull request решает суммарный diff `base..head`, а не последний push: коммит из одних .md в PR
с функциональными изменениями НЕ отменяет проверки — иначе упавший или несостоявшийся runtime
можно было бы «прикрыть» документационным коммитом, получить COUNT=0 и зелёный Differential gate.
Полностью документационный PR (все изменения — .md) не запускает ничего: это и есть безопасный
случай оптимизации.

Проверка сохранения достижений (`check_wolfman_achievement.py`) нужна: при полном ручном и
плановом прогоне; при изменениях самого теста, его маршрута, плеера и раннера (то же, что «все
маршруты»), платформенного слоя (`Source/NLBPlatforms/`, выдача достижений), валидатора state-log
и фрагментов, от которых зависит маршрут теста. Документы её не включают.

Пишет в $GITHUB_OUTPUT (или в stdout): matrix={"include": [...]}, count=N,
achievements=true|false (нужна ли проверка достижений), build=true|false (нужна ли сборка
nlb_sdl: маршруты или достижения) и reason — человекочитаемое объяснение для сводки CI.
У каждого маршрута — id, path и таймауты по числу шагов (снимки INSTEAD занимают ~6 с на шаг
на hosted runners).
"""
import argparse
import json
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tests/differential"))
from route_validation import expand_prefix  # noqa: E402
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

# Проверка сохранения достижений: сам тест и его маршрут меняются — проверка нужна; маршрут
# теста не входит в ALL_PREFIXES, поэтому изменение только теста не выбирает обычных маршрутов.
ACHIEVEMENT_TEST = "tests/differential/check_wolfman_achievement.py"
ACHIEVEMENT_ROUTE = "tests/differential/routes/wolfman-guard-low-wolfiness-escape.json"
# Плеер и раннер — через ALL_PREFIXES; дополнительно: платформенный слой (выдача достижений в
# PlatformBridge/PlatformServices) и валидатор state-log, которым проверяются прогоны теста.
ACHIEVEMENT_EXTRA_PREFIXES = (
    "Source/NLBPlatforms/",
    "tools/check_state_log.py",
)


def load_routes():
    catalog = json.loads(CATALOG.read_text(encoding="utf-8"))
    return catalog["routes"]


def expanded(path):
    """Маршрут с шагами общего префикса (routes/fragments)"""
    route = json.loads((ROOT / path).read_text(encoding="utf-8"))
    return expand_prefix(route, (ROOT / path).parent, path)[0]


def timeouts(path):
    route = expanded(path)
    steps = len(route.get("steps", []))
    # advance_until — до max_clicks щелчков за шаг: INSTEAD ждёт после каждого запись trace (и
    # затишье), nlb_sdl щёлкает без снимков экрана
    clicks = sum(int(step.get("action", {}).get("max_clicks", 0)) for step in route.get("steps", [])
                 if step.get("action", {}).get("type") == "advance_until")
    # INSTEAD: снимок экрана и trace на каждом шаге; nlb_sdl быстрее
    return {"instead_timeout": 120 + 8 * steps + 3 * clicks, "nlb_timeout": 90 + 5 * steps + clicks}


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


def relevant(files):
    """Изменённые файлы без документов (.md)"""
    return [f for f in files if not is_doc(f)]


def touches_player_or_runner(path):
    """Путь требует прогона всех маршрутов (плеер, раннер, workflow, каталог, CI)"""
    return path.startswith(ALL_PREFIXES) or (path.startswith("Source/") and path.count("/") == 1)


def changed_fragments(files):
    """Имена изменённых файлов общих префиксов маршрутов"""
    return set(Path(f).stem for f in files
               if f.startswith(ROUTES_DIR + "fragments/") and f.endswith(".json"))


def needs_achievements(files):
    """Нужна ли проверка сохранения достижений (check_wolfman_achievement.py)"""
    for path in files:
        if touches_player_or_runner(path) or path in (ACHIEVEMENT_TEST, ACHIEVEMENT_ROUTE) \
                or path.startswith(ACHIEVEMENT_EXTRA_PREFIXES):
            return True
    chain = set(expanded(ACHIEVEMENT_ROUTE).get("prefix_chain") or [])
    return bool(changed_fragments(files) & chain)


def select(routes, files):
    """(выбранные маршруты, объяснение, нужна ли проверка достижений)"""
    if files is None:
        return routes, "все маршруты: база сравнения неизвестна", True
    originals = files
    files = relevant(files)
    if not files:
        reason = "только документы (.md)" if docs_only(originals) else "изменений вне документов нет"
        return [], reason + ": проверки не нужны", False
    achievements = needs_achievements(files)
    if any(touches_player_or_runner(f) for f in files):
        return routes, "все маршруты: изменились раннер, workflow, каталог или плеер", achievements
    changed = set(f for f in files if f.startswith(ROUTES_DIR) and f.endswith(".json"))
    fragments = changed_fragments(files)
    chosen = [r for r in routes if r["path"] in changed
              or (fragments and fragments & set(expanded(r["path"]).get("prefix_chain") or []))]
    return chosen, "изменённые маршруты: {}".format(", ".join(r["id"] for r in chosen) or "нет"), achievements


def outputs(routes, chosen, why, achievements):
    """Значения для $GITHUB_OUTPUT: матрица, счётчики и флаги заданий"""
    include = [dict(id=r["id"], path=r["path"], **timeouts(r["path"])) for r in chosen]
    count = len(include)
    return {
        "matrix": json.dumps({"include": include}, ensure_ascii=False),
        "count": str(count),
        "achievements": "true" if achievements else "false",
        "build": "true" if (count or achievements) else "false",
        "reason": "{} ({}){}".format(why, count, "; достижения: да" if achievements else ""),
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--all", action="store_true")
    parser.add_argument("--only", default="")
    parser.add_argument("--base", default="")
    parser.add_argument("--head", default="HEAD")
    args = parser.parse_args()

    routes = load_routes()
    if args.only.strip():
        wanted = [x.strip() for x in args.only.split(",") if x.strip()]
        unknown = [x for x in wanted if x not in {r["id"] for r in routes}]
        if unknown:
            sys.exit("Нет в catalog.json: " + ", ".join(unknown))
        chosen, why, achievements = [r for r in routes if r["id"] in wanted], "выбраны вручную", True
    elif args.all:
        chosen, why, achievements = routes, "все маршруты", True
    else:
        chosen, why, achievements = select(routes, changed_files(args.base, args.head))

    values = outputs(routes, chosen, why, achievements)
    print("{}; достижения: {}".format(why, "да" if achievements else "нет"), file=sys.stderr)
    lines = ["{}={}".format(key, value) for key, value in values.items()]
    output = os.environ.get("GITHUB_OUTPUT")
    if output:
        with open(output, "a", encoding="utf-8") as stream:
            stream.write("\n".join(lines) + "\n")
    else:
        print("\n".join(lines))


if __name__ == "__main__":
    main()
