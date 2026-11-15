#!/usr/bin/env python3
"""Report declared page coverage from route expect.page_id fields (not runtime results)."""
import argparse, json, subprocess, sys
from collections import defaultdict
from pathlib import Path

KINDS = ("page", "return", "ending", "ending?")
ROOT = Path(__file__).resolve().parents[2]


class CoverageError(RuntimeError):
    pass


def read_json(path):
    try:
        return json.loads(Path(path).read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise CoverageError(f"Не удалось прочитать JSON {path}: {exc}") from exc


def book_entry(catalog, book_id):
    found = [b for b in catalog.get("books", []) if b.get("id") == book_id]
    if len(found) != 1 or not (found[0].get("nlb") or {}).get("reference_commit"):
        raise CoverageError(f"Для book-id={book_id!r} нужна ровно одна запись books с nlb.reference_commit")
    return found[0]


def verify_commit(book, expected):
    try:
        actual = subprocess.run(
            ["git", "-C", str(book), "rev-parse", "HEAD"], check=True,
            capture_output=True, text=True,
        ).stdout.strip()
    except (OSError, subprocess.CalledProcessError) as exc:
        detail = getattr(exc, "stderr", None) or str(exc)
        raise CoverageError(f"Не удалось определить Git commit книги {book}: {detail.strip()}") from exc
    if actual != expected:
        raise CoverageError(
            f"Версия книги не совпадает с catalog.json: reference_commit={expected}, actual={actual}. "
            "Используйте локальную копию на закреплённом commit."
        )
    return actual


def inventory(book, tool):
    if not tool.is_file():
        raise CoverageError(f"Не найден инструмент инвентаризации страниц: {tool}")
    try:
        result = subprocess.run(
            [sys.executable, str(tool), str(book), "--json"], check=True,
            capture_output=True, text=True,
        )
        payload = json.loads(result.stdout)
    except (OSError, subprocess.CalledProcessError, json.JSONDecodeError) as exc:
        detail = getattr(exc, "stderr", None) or str(exc)
        raise CoverageError(f"Ошибка tools/nlb_pages.py: {detail.strip() or exc}") from exc
    if payload.get("missing_modules"):
        raise CoverageError("Не найдены внешние модули: " + ", ".join(payload["missing_modules"]))
    pages = payload.get("pages")
    if not isinstance(pages, list):
        raise CoverageError("В JSON tools/nlb_pages.py отсутствует массив pages")
    seen = set()
    for page in pages:
        pid, kind = page.get("id"), page.get("kind")
        if not isinstance(pid, str) or not pid or pid in seen:
            raise CoverageError(f"Отсутствует или повторяется UUID страницы: {pid!r}")
        if kind not in (*KINDS, "module"):
            raise CoverageError(f"Неизвестный вид страницы {kind!r}: {pid}")
        seen.add(pid)
    return pages


def routes_for(catalog, book_id, repo_root):
    routes, root = [], Path(repo_root).resolve()
    for item in catalog.get("routes", []):
        if item.get("book_id") != book_id:
            continue
        path = (root / item["path"]).resolve()
        try:
            path.relative_to(root)
        except ValueError as exc:
            raise CoverageError(f"Путь маршрута выходит за пределы репозитория: {item['path']}") from exc
        route = read_json(path)
        if route.get("id") != item.get("id"):
            raise CoverageError(f"ID маршрута в каталоге и файле не совпадают: {item.get('id')}")
        routes.append(route)
    if not routes:
        raise CoverageError(f"Нет зарегистрированных маршрутов для книги {book_id!r}")
    return routes


def make_report(book_id, entry, commit, pages, routes):
    ids, covered_by = {p["id"] for p in pages}, defaultdict(set)
    unknown = defaultdict(lambda: defaultdict(set))
    for route in routes:
        for step in route.get("steps", []):
            expect = step.get("expect")
            pid = expect.get("page_id") if isinstance(expect, dict) else None
            if not isinstance(pid, str) or not pid.strip():
                continue
            pid = pid.strip()
            if pid in ids:
                covered_by[pid].add(route["id"])
            else:
                unknown[pid][route["id"]].add(str(step.get("id", "<без id>")))

    summaries, page_rows = {}, []
    for kind in KINDS:
        selected = [p for p in pages if p["kind"] == kind]
        uncovered = [p for p in selected if p["id"] not in covered_by]
        summaries[kind] = {
            "total": len(selected), "covered": len(selected) - len(uncovered),
            "uncovered": [{"id": p["id"], "caption": p.get("caption", ""),
                           "modules": p.get("modules", []), "path": p.get("path", "")}
                          for p in uncovered],
        }
        for p in selected:
            page_rows.append({
                "id": p["id"], "kind": kind, "caption": p.get("caption", ""),
                "modules": p.get("modules", []), "path": p.get("path", ""),
                "covered": p["id"] in covered_by, "routes": sorted(covered_by[p["id"]]),
            })
    unknown_rows = [{"id": pid, "routes": [
        {"route_id": rid, "steps": sorted(steps)} for rid, steps in sorted(by_route.items())
    ]} for pid, by_route in sorted(unknown.items())]
    return {
        "schema_version": 1,
        "coverage_semantics": "declared coverage from route expect.page_id; runtime execution is not evaluated",
        "book": {"id": book_id, "title": entry.get("title", book_id),
                 "reference_commit": entry["nlb"]["reference_commit"], "actual_commit": commit},
        "inventory": {
            "total_pages_including_modules": len(pages),
            "module_containers_excluded_from_coverage": sum(p["kind"] == "module" for p in pages),
            "pages_in_coverage_denominator": sum(p["kind"] != "module" for p in pages),
            "by_kind": summaries,
        },
        "pages": sorted(page_rows, key=lambda p: (p["kind"], p["id"])),
        "unknown_route_page_ids": unknown_rows,
    }


def markdown(report):
    book, inv = report["book"], report["inventory"]
    labels = {"page": "Обычные страницы", "return": "Возвраты из модулей",
              "ending": "Концовки", "ending?": "Страницы-кандидаты (ending?)"}
    out = [f"# Заявленное покрытие страниц: {book['title']}", "",
           "> Это заявленное покрытие: анализируются expect.page_id зарегистрированных маршрутов; "
           "выполнение маршрутов в runtime не проверяется.", "",
           f"- Book ID: {book['id']}",
           f"- Commit книги: {book['actual_commit']} (совпадает с reference_commit)",
           f"- Всего страниц: {inv['total_pages_including_modules']}; контейнеров module исключено: "
           f"{inv['module_containers_excluded_from_coverage']}",
           f"- Знаменатель покрытия: {inv['pages_in_coverage_denominator']}", ""]
    for kind in KINDS:
        s = inv["by_kind"][kind]
        pct = 100 * s["covered"] / s["total"] if s["total"] else 0
        out += [f"## {labels[kind]}", "", f"Всего: **{s['total']}**, заявлено покрыто: **{s['covered']}** ({pct:.1f}%).", "",
                "| UUID | Страница | Путь модулей | Статус | Маршруты |", "|---|---|---|---|---|"]
        for p in (x for x in report["pages"] if x["kind"] == kind):
            caption = (p["caption"] or "").replace("|", "\\|").replace("\n", " ")
            modules = " → ".join(p["modules"]) or "—"
            route_ids = ", ".join(p["routes"]) or "—"
            status = "покрыта" if p["covered"] else "не покрыта"
            out.append(f"| {p['id']} | {caption} | {modules} | {status} | {route_ids} |")
        out.append("")
    out += ["## page_id из маршрутов, отсутствующие в книге", ""]
    if not report["unknown_route_page_ids"]:
        out.append("Нет.")
    else:
        out += ["| UUID | Маршруты и шаги |", "|---|---|"]
        for item in report["unknown_route_page_ids"]:
            refs = "; ".join(f"{r['route_id']} ({', '.join(r['steps'])})" for r in item["routes"])
            out.append(f"| {item['id']} | {refs} |")
    return "\n".join(out) + "\n"


def main(argv=None):
    parser = argparse.ArgumentParser(description="Отчёт о заявленном покрытии страниц маршрутами")
    parser.add_argument("book", type=Path, help="Путь к локальной pinned-копии книги")
    parser.add_argument("--book-id", required=True, help="ID книги из catalog.json")
    parser.add_argument("--catalog", type=Path, default=ROOT / "tests/differential/catalog.json")
    parser.add_argument("--json", action="store_true", help="Вывести JSON вместо Markdown")
    args = parser.parse_args(argv)
    try:
        catalog = read_json(args.catalog)
        entry = book_entry(catalog, args.book_id)
        commit = verify_commit(args.book.resolve(), entry["nlb"]["reference_commit"])
        pages = inventory(args.book.resolve(), ROOT / "tools/nlb_pages.py")
        routes = routes_for(catalog, args.book_id, ROOT)
        report = make_report(args.book_id, entry, commit, pages, routes)
    except CoverageError as exc:
        print(f"Ошибка coverage_report: {exc}", file=sys.stderr)
        return 2
    if args.json:
        json.dump(report, sys.stdout, ensure_ascii=False, indent=2)
        print()
    else:
        sys.stdout.write(markdown(report))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
