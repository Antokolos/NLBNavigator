#!/usr/bin/env python3
"""Проверяет согласованность каталога тестовых книг и закреплённых CI refs."""

import json
import re
import sys
from pathlib import Path

from route_validation import validate_route


ROOT = Path(__file__).resolve().parents[2]
CATALOG_PATH = ROOT / "tests/differential/catalog.json"
WORKFLOW_PATH = ROOT / ".github/workflows/nlb-sdl-test.yml"


def fail(message):
    print("[ERROR] " + message, file=sys.stderr)
    return 1


def main():
    with CATALOG_PATH.open("r", encoding="utf-8") as source:
        catalog = json.load(source)
    workflow = WORKFLOW_PATH.read_text(encoding="utf-8")

    if catalog.get("schema_version") != 1:
        return fail("Неизвестная версия схемы каталога")

    books = catalog.get("books", [])
    ids = [book.get("id") for book in books]
    if len(ids) != len(set(ids)) or not ids:
        return fail("ID книг должны быть уникальными и непустыми")

    for book in books:
        nlb = book.get("nlb") or {}
        nlb_sha = nlb.get("reference_commit", "")
        if not re.fullmatch(r"[0-9a-f]{40}", nlb_sha):
            return fail("{}: NLB reference_commit должен быть полным SHA-1".format(book.get("id")))

        instead = book.get("instead")
        if instead is None:
            if book.get("status") != "nlb-only-unpaired":
                return fail("{}: книга без INSTEAD экспорта должна быть помечена unpaired".format(book.get("id")))
            continue

        instead_sha = instead.get("reference_commit", "")
        if not re.fullmatch(r"[0-9a-f]{40}", instead_sha):
            return fail("{}: INSTEAD reference_commit должен быть полным SHA-1".format(book.get("id")))

    base_theme = catalog.get("base_theme") or {}
    base_theme_sha = base_theme.get("reference_commit", "")
    if not re.fullmatch(r"[0-9a-f]{40}", base_theme_sha):
        return fail("base_theme.reference_commit должен быть полным SHA-1")

    wolfman = next((book for book in books if book.get("id") == "wolfman"), None)
    if not wolfman or not wolfman.get("instead"):
        return fail("В каталоге отсутствует парная книга wolfman")

    expected = {
        "GAME_REF": wolfman["nlb"]["reference_commit"],
        "REFERENCE_REF": wolfman["instead"]["reference_commit"],
        "BASE_THEME_REF": base_theme_sha,
    }
    for variable, sha in expected.items():
        pattern = r"{}\s*:\s*\$\{{\{{\s*github\.event\.inputs\.[a-z_]+\s*\|\|\s*'([0-9a-f]{{40}})'\s*\}}\}}".format(variable)
        match = re.search(pattern, workflow)
        if not match:
            return fail("В workflow не найден закреплённый default для " + variable)
        if match.group(1) != sha:
            return fail(
                "{} в workflow ({}) не совпадает с catalog.json ({})".format(
                    variable, match.group(1), sha
                )
            )

        input_name = {
            "GAME_REF": "game_ref",
            "REFERENCE_REF": "reference_ref",
            "BASE_THEME_REF": "base_theme_ref",
        }[variable]
        default_pattern = r"{}\s*:\s*\n\s*description:.*\n\s*required:\s*true\n\s*default:\s*[\"']?".format(input_name) + re.escape(sha)
        if not re.search(default_pattern, workflow):
            return fail("workflow_dispatch input {} не использует тот же pinned SHA".format(input_name))

    routes = catalog.get("routes", [])
    if not isinstance(routes, list) or not routes:
        return fail("В каталоге отсутствует непустой массив routes")
    route_ids = [item.get("id") if isinstance(item, dict) else None for item in routes]
    if any(not isinstance(value, str) or not value for value in route_ids) or len(route_ids) != len(set(route_ids)):
        return fail("ID маршрутов в catalog.json должны быть уникальными и непустыми")

    book_ids = set(ids)
    for item in routes:
        route_id = item["id"]
        book_id = item.get("book_id")
        relative_path = item.get("path", "")
        if book_id not in book_ids:
            return fail("{}: неизвестная книга {!r}".format(route_id, book_id))
        if not isinstance(relative_path, str) or not relative_path:
            return fail("{}: не задан path маршрута".format(route_id))
        route_path = (ROOT / relative_path).resolve()
        try:
            route_path.relative_to(ROOT)
        except ValueError:
            return fail("{}: путь маршрута выходит за пределы репозитория".format(route_id))
        if not route_path.is_file():
            return fail("{}: файл маршрута не найден: {}".format(route_id, relative_path))
        try:
            with route_path.open("r", encoding="utf-8") as source:
                route = json.load(source)
        except (OSError, json.JSONDecodeError) as error:
            return fail("{}: не удалось прочитать маршрут: {}".format(route_id, error))
        if not isinstance(route, dict):
            return fail("{}: корень JSON маршрута должен быть объектом".format(route_id))
        if route.get("id") != route_id:
            return fail("{}: id в JSON ({}) не совпадает с catalog.json".format(route_id, route.get("id")))
        route_errors = validate_route(route, relative_path)
        if route_errors:
            return fail("; ".join(route_errors))

    print(
        "Catalog OK: {} books, {} paired INSTEAD exports, {} unpaired book, {} routes.".format(
            len(books),
            sum(1 for book in books if book.get("instead")),
            sum(1 for book in books if not book.get("instead")),
            len(routes),
        )
    )
    print("wolfman, INSTEAD export, base theme pins and registered routes are valid.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
