"""Выбор маршрутов для CI (tests/differential/ci/select_routes.py)."""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent / "ci"))
import select_routes  # noqa: E402

ROUTES = [
    {"id": "a", "path": "tests/differential/routes/a.json"},
    {"id": "b", "path": "tests/differential/routes/b.json"},
]


class SelectRoutesTest(unittest.TestCase):
    def ids(self, files):
        return [r["id"] for r in select_routes.select(ROUTES, files)[0]]

    def test_changed_route_only(self):
        self.assertEqual(self.ids(["tests/differential/routes/b.json"]), ["b"])

    def test_docs_only_changes_select_nothing(self):
        self.assertEqual(self.ids(["tests/differential/README.md", "docs/state-log.md"]), [])

    def test_runner_or_player_changes_run_all(self):
        for changed in ("tests/differential/run_route.py", ".github/workflows/nlb-sdl-test.yml",
                        "Source/NLBSdl/SdlPlayer.cpp", "Source/SessionLog.cpp", "tests/differential/catalog.json"):
            self.assertEqual(self.ids([changed]), ["a", "b"], changed)

    def test_docs_never_select_routes(self):
        # .md даже в каталогах «прогнать всё» (ci/) маршрутов не добавляет
        self.assertEqual(self.ids(["tests/differential/ci/README.md", "Source/NLBSdl/NOTES.md"]), [])
        self.assertEqual(self.ids(["tests/differential/README.md", "tests/differential/routes/a.json"]), ["a"])
        self.assertEqual(self.ids(["docs/state-log.md", "tests/differential/run_route.py"]), ["a", "b"])

    def test_docs_only(self):
        self.assertTrue(select_routes.docs_only(["tests/differential/README.md", "docs/A.MD"]))
        self.assertFalse(select_routes.docs_only(["tests/differential/README.md", "tests/differential/routes/a.json"]))
        self.assertFalse(select_routes.docs_only([]))
        self.assertFalse(select_routes.docs_only(None))

    def test_other_source_does_not_run(self):
        self.assertEqual(self.ids(["Source/NLBPlayer/Private/Foo.cpp"]), [])

    def test_unknown_base_runs_all(self):
        self.assertEqual(self.ids(None), ["a", "b"])
        self.assertIsNone(select_routes.changed_files("0000000000000000000000000000000000000000", "HEAD"))

    def test_catalog_routes_have_timeouts(self):
        for route in select_routes.load_routes():
            t = select_routes.timeouts(route["path"])
            self.assertGreater(t["instead_timeout"], 120)
            self.assertGreater(t["nlb_timeout"], 90)


if __name__ == "__main__":
    unittest.main()
