#!/usr/bin/env python3
import json, subprocess, tempfile, unittest
from pathlib import Path
from unittest.mock import patch
from coverage_report import CoverageError, book_entry, inventory, make_report, markdown, routes_for, verify_commit


class CoverageReportTests(unittest.TestCase):
    def setUp(self):
        self.entry = {"id": "sample", "title": "Sample book", "nlb": {"reference_commit": "a" * 40}}
        self.pages = [
            {"id": "module-id", "kind": "module", "caption": "Module", "modules": [], "path": "pages/module"},
            {"id": "page-a", "kind": "page", "caption": "A", "modules": [], "path": "pages/page-a"},
            {"id": "return-a", "kind": "return", "caption": "Return", "modules": ["module-id"], "path": "pages/module/pages/return-a"},
            {"id": "ending-a", "kind": "ending", "caption": "Ending", "modules": [], "path": "pages/ending-a"},
            {"id": "maybe-a", "kind": "ending?", "caption": "Maybe", "modules": [], "path": "pages/maybe-a"},
        ]
        self.routes = [
            {"id": "route-one", "steps": [
                {"id": "covered", "expect": {"page_id": "page-a"}},
                {"id": "empty-expect", "expect": {}}, {"id": "no-expect"},
                {"id": "unknown", "expect": {"page_id": "not-in-book"}}]},
            {"id": "route-two", "steps": [
                {"id": "same-page", "expect": {"page_id": "page-a"}},
                {"id": "return", "expect": {"page_id": "return-a"}}]},
        ]

    def report(self):
        return make_report("sample", self.entry, "a" * 40, self.pages, self.routes)

    def test_only_nonempty_expected_page_ids_count(self):
        by_kind = self.report()["inventory"]["by_kind"]
        self.assertEqual((by_kind["page"]["total"], by_kind["page"]["covered"]), (1, 1))
        self.assertEqual(by_kind["return"]["covered"], 1)
        self.assertEqual(by_kind["ending"]["covered"], 0)
        self.assertEqual(by_kind["ending?"]["covered"], 0)
        self.assertEqual(self.report()["inventory"]["pages_in_coverage_denominator"], 4)
        self.assertEqual(self.report()["inventory"]["module_containers_excluded_from_coverage"], 1)

    def test_expect_pages_count_for_coverage(self):
        routes = self.routes + [{
            "id": "advance-route",
            "steps": [{
                "id": "advance-to-ending",
                "action": {
                    "type": "advance_until",
                    "expect_pages": [{"page_id": "ending-a", "portion_count": 3}],
                },
            }],
        }]
        report = make_report("sample", self.entry, "a" * 40, self.pages, routes)
        row = next(p for p in report["pages"] if p["id"] == "ending-a")
        self.assertTrue(row["covered"])
        self.assertEqual(row["routes"], ["advance-route"])

    def test_advance_until_without_nonempty_expect_pages_is_reported(self):
        routes = [{
            "id": "route-advance",
            "steps": [
                {"id": "missing", "action": {"type": "advance_until", "until": "choices"}},
                {"id": "empty", "action": {"type": "advance_until", "expect_pages": []}},
                {"id": "present", "action": {
                    "type": "advance_until", "expect_pages": [{"page_id": "page-a"}],
                }},
                {"id": "other-action", "action": {"type": "advance"}},
            ],
        }]
        report = make_report("sample", self.entry, "a" * 40, self.pages, routes)
        expected = [
            {"route_id": "route-advance", "step_id": "empty"},
            {"route_id": "route-advance", "step_id": "missing"},
        ]
        self.assertEqual(report["advance_until_missing_expect_pages"], expected)
        rendered = markdown(report)
        self.assertIn("## advance_until без expect_pages", rendered)
        self.assertIn("| route-advance | missing |", rendered)
        self.assertIn("| route-advance | empty |", rendered)
        self.assertNotIn("| route-advance | present |", rendered)

    def test_every_covering_route_is_listed(self):
        row = next(p for p in self.report()["pages"] if p["id"] == "page-a")
        self.assertEqual(row["routes"], ["route-one", "route-two"])

    def test_prefix_fragment_pages_are_credited_to_each_route(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            routes_dir = root / "tests" / "differential" / "routes"
            fragments_dir = routes_dir / "fragments"
            fragments_dir.mkdir(parents=True)
            fragment = {
                "id": "shared-start",
                "steps": [
                    {"id": "opening", "expect": {"page_id": "ending-a"}},
                    {
                        "id": "advance",
                        "action": {
                            "type": "advance_until",
                            "expect_pages": [{"page_id": "return-a"}],
                        },
                    },
                ],
            }
            (fragments_dir / "shared-start.json").write_text(
                json.dumps(fragment), encoding="utf-8"
            )
            route_items = []
            for route_id in ("branch-one", "branch-two"):
                route = {"id": route_id, "prefix": "shared-start", "steps": []}
                relative = f"tests/differential/routes/{route_id}.json"
                (routes_dir / f"{route_id}.json").write_text(
                    json.dumps(route), encoding="utf-8"
                )
                route_items.append({"id": route_id, "book_id": "sample", "path": relative})

            expanded_routes = routes_for({"routes": route_items}, "sample", root)
            report = make_report("sample", self.entry, "a" * 40, self.pages, expanded_routes)
            for page_id in ("ending-a", "return-a"):
                row = next(p for p in report["pages"] if p["id"] == page_id)
                self.assertTrue(row["covered"])
                self.assertEqual(row["routes"], ["branch-one", "branch-two"])

    def test_unknown_ids_include_routes_and_steps(self):
        self.assertEqual(self.report()["unknown_route_page_ids"], [
            {"id": "not-in-book", "routes": [{"route_id": "route-one", "steps": ["unknown"]}]}])

    def test_ending_question_mark_is_not_called_an_ending(self):
        rendered = markdown(self.report())
        self.assertIn("Страницы-кандидаты (ending?)", rendered)
        self.assertNotIn("Концовки (ending?)", rendered)

    def test_book_commit_mismatch_fails_clearly(self):
        result = subprocess.CompletedProcess([], 0, "b" * 40 + "\n", "")
        with patch("coverage_report.subprocess.run", return_value=result):
            with self.assertRaisesRegex(CoverageError, "reference_commit"):
                verify_commit(Path("/book"), "a" * 40)

    def test_matching_book_commit_is_returned(self):
        result = subprocess.CompletedProcess([], 0, "a" * 40 + "\n", "")
        with patch("coverage_report.subprocess.run", return_value=result):
            self.assertEqual(verify_commit(Path("/book"), "a" * 40), "a" * 40)

    def test_pages_tool_json_interface_is_used(self):
        with tempfile.TemporaryDirectory() as directory:
            tool = Path(directory) / "nlb_pages.py"
            tool.write_text("# fixture", encoding="utf-8")
            payload = {"pages": [{"id": "p", "kind": "page"}], "missing_modules": []}
            result = subprocess.CompletedProcess([], 0, json.dumps(payload), "")
            with patch("coverage_report.subprocess.run", return_value=result) as run:
                self.assertEqual(inventory(Path("/book"), tool), payload["pages"])
            self.assertEqual(run.call_args.args[0][-2:], [str(Path("/book")), "--json"])

    def test_missing_external_modules_fail_closed(self):
        with tempfile.TemporaryDirectory() as directory:
            tool = Path(directory) / "nlb_pages.py"
            tool.write_text("# fixture", encoding="utf-8")
            payload = {"pages": [], "missing_modules": ["missing-addon"]}
            result = subprocess.CompletedProcess([], 0, json.dumps(payload), "")
            with patch("coverage_report.subprocess.run", return_value=result):
                with self.assertRaisesRegex(CoverageError, "missing-addon"):
                    inventory(Path("/book"), tool)

    def test_route_path_cannot_escape_repository(self):
        catalog = {"routes": [{"id": "bad", "book_id": "sample", "path": "../../outside.json"}]}
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(CoverageError, "выходит за пределы"):
                routes_for(catalog, "sample", Path(directory))

    def test_book_id_must_match_one_catalog_entry(self):
        with self.assertRaisesRegex(CoverageError, "ровно одна"):
            book_entry({"books": []}, "missing")


if __name__ == "__main__":
    unittest.main()
