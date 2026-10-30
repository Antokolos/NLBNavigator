#!/usr/bin/env python3
"""Проверяет согласованность каталога тестовых книг и закреплённых CI refs."""

import json
import re
import sys
from pathlib import Path


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

    print(
        "Catalog OK: {} books, {} paired INSTEAD exports, {} unpaired book.".format(
            len(books), sum(1 for book in books if book.get("instead")), sum(1 for book in books if not book.get("instead"))
        )
    )
    print("wolfman, INSTEAD export and base theme pins match workflow defaults.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
