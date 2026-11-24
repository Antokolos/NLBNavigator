"""Выбор маршрутов для CI (tests/differential/ci/select_routes.py)."""
import contextlib
import io
import os
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent / "ci"))
import select_routes  # noqa: E402

ROUTES = [
    {"id": "a", "path": "tests/differential/routes/a.json"},
    {"id": "b", "path": "tests/differential/routes/b.json"},
]


class SelectRoutesTest(unittest.TestCase):
    def ids(self, files):
        return [r["id"] for r in select_routes.select(ROUTES, files)[0]]

    def achievements(self, files):
        return select_routes.select(ROUTES, files)[2]

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

    def test_files_outside_player_select_nothing(self):
        # workflow запускается на каждом PR в main (обязательная проверка «Differential gate»):
        # PR без плеера, раннера и маршрутов — ни одного маршрута, сборка пропускается
        for changed in ("android/app/build.gradle", "integrations/godot/src/register_types.cpp",
                        "tools/make_nlbz.py", ".gitignore", "Source/NLBPlatforms/Steam.cpp"):
            self.assertEqual(self.ids([changed]), [], changed)

    def test_unknown_base_runs_all(self):
        self.assertEqual(self.ids(None), ["a", "b"])
        self.assertIsNone(select_routes.changed_files("0000000000000000000000000000000000000000", "HEAD"))

    def test_catalog_routes_have_timeouts(self):
        for route in select_routes.load_routes():
            t = select_routes.timeouts(route["path"])
            self.assertGreater(t["instead_timeout"], 120)
            self.assertGreater(t["nlb_timeout"], 90)


class SelectAchievementsTest(unittest.TestCase):
    """Проверка достижений — отдельное задание CI: её нужно включать и при COUNT=0."""

    def test_docs_only_needs_neither(self):
        self.assertFalse(select_routes.select(ROUTES, ["tests/differential/README.md", "docs/A.md"])[2])
        self.assertEqual(select_routes.select(ROUTES, ["tests/differential/README.md", "docs/A.md"])[0], [])

    def test_player_runner_ci_change_needs_achievements(self):
        for changed in ("Source/NLBSdl/SdlPlayer.cpp",
                        "Source/NLBPresentation/Private/nlb/platform/PlatformBridge.cpp",
                        "Source/NLBCore/Private/nlb/domain/PageImpl.cpp",
                        "tests/differential/run_route.py",
                        ".github/workflows/nlb-sdl-test.yml",
                        "tests/differential/catalog.json",
                        "tests/differential/ci/gate.py",
                        "tests/differential/ci/select_routes.py"):
            self.assertTrue(select_routes.select(ROUTES, [changed])[2], changed)

    def test_achievement_test_change_needs_only_achievements(self):
        files = ["tests/differential/check_wolfman_achievement.py"]
        self.assertEqual(select_routes.select(ROUTES, files)[0], [])
        self.assertTrue(select_routes.select(ROUTES, files)[2])

    def test_achievement_route_change_selects_it_and_achievements(self):
        catalog = select_routes.load_routes()
        changed = [select_routes.ACHIEVEMENT_ROUTE]
        chosen, _, achievements = select_routes.select(catalog, changed)
        self.assertTrue(achievements)
        self.assertEqual([r["id"] for r in chosen], ["wolfman-guard-low-wolfiness-escape"])

    def test_platform_layer_and_validator_need_only_achievements(self):
        for changed in ("Source/NLBPlatforms/Steam.cpp", "tools/check_state_log.py"):
            self.assertEqual(select_routes.select(ROUTES, [changed])[0], [], changed)
            self.assertTrue(select_routes.select(ROUTES, [changed])[2], changed)

    def test_unrelated_changes_need_nothing(self):
        for changed in ("android/app/build.gradle", "integrations/godot/src/register_types.cpp",
                        "tools/make_nlbz.py", ".gitignore", "Source/NLBPlayer/Private/Foo.cpp"):
            self.assertEqual(select_routes.select(ROUTES, [changed])[0], [], changed)
            self.assertFalse(select_routes.select(ROUTES, [changed])[2], changed)

    def test_unknown_base_needs_routes_and_achievements(self):
        chosen, _, achievements = select_routes.select(ROUTES, None)
        self.assertTrue(achievements)
        self.assertEqual([r["id"] for r in chosen], ["a", "b"])

    def test_outputs_build_flag_for_achievements_only(self):
        # COUNT=0, но проверка достижений нужна — сборка обязательна, иначе job нечего запускать
        catalog = select_routes.load_routes()
        values = select_routes.outputs(catalog, [], "тест", True)
        self.assertEqual(values["count"], "0")
        self.assertEqual(values["achievements"], "true")
        self.assertEqual(values["build"], "true")
        self.assertIn("достижения: да", values["reason"])
        values = select_routes.outputs(catalog, [], "документы", False)
        self.assertEqual(values["build"], "false")
        self.assertNotIn("достижения", values["reason"])
        values = select_routes.outputs(catalog, catalog, "маршруты", False)
        self.assertEqual(values["count"], str(len(catalog)))
        self.assertEqual(values["achievements"], "false")
        self.assertEqual(values["build"], "true")


class PullRequestDocsOnlyRegressionTest(unittest.TestCase):
    """Оптимизация docs-only не может скрыть функциональное изменение.

    Для pull request решение принимается по суммарному diff base..head. Коммит из одних .md не
    меняет выбор, поэтому упавший, отменённый или несостоявшийся runtime нельзя «прикрыть»
    документацией и получить зелёный Differential gate.
    """

    FUNCTIONAL = "Source/NLBPresentation/Private/nlb/platform/PlatformBridge.cpp"

    def run_main(self, argv, files):
        """main() с подменённым git diff: возвращает значения $GITHUB_OUTPUT"""
        # mkstemp, а не TemporaryDirectory: очистка каталога в Windows-песочнице падает на chmod
        handle, path = tempfile.mkstemp(prefix="github-output-", suffix=".txt")
        os.close(handle)
        try:
            with mock.patch.object(select_routes, "changed_files", return_value=files), \
                    mock.patch.dict(os.environ, {"GITHUB_OUTPUT": path}), \
                    mock.patch.object(sys, "argv", ["select_routes.py"] + argv), \
                    contextlib.redirect_stderr(io.StringIO()):
                select_routes.main()
            text = Path(path).read_text(encoding="utf-8")
        finally:
            Path(path).unlink(missing_ok=True)
        return dict(line.split("=", 1) for line in text.splitlines() if "=" in line)

    def test_docs_only_pull_request_runs_nothing(self):
        values = self.run_main(["--base", "base", "--head", "head"],
                               ["tests/differential/README.md", "docs/state-log.md"])
        self.assertEqual(values["count"], "0")
        self.assertEqual(values["achievements"], "false")
        self.assertEqual(values["build"], "false")
        self.assertIn("документ", values["reason"])

    def test_functional_change_plus_docs_commit_still_runs(self):
        # последний push — документация, но в PR есть функциональное изменение
        values = self.run_main(["--base", "base", "--head", "head"],
                               [self.FUNCTIONAL, "docs/state-log.md"])
        self.assertNotEqual(values["count"], "0")
        self.assertEqual(values["achievements"], "true")
        self.assertEqual(values["build"], "true")

    def test_previous_ci_result_cannot_be_covered_by_docs_commit(self):
        # previous_ci — не вход выбора: после успеха, падения, отмены или отсутствия прогона
        # коммит из одних .md не меняет решения — runtime выполняется заново
        for previous in ("success", "failure", "cancelled", "missing"):
            values = self.run_main(["--base", "base", "--head", "head"],
                                   [self.FUNCTIONAL, "docs/state-log.md"])
            self.assertEqual(values["build"], "true", previous)
            self.assertEqual(values["achievements"], "true", previous)

    def test_unknown_base_after_docs_commit_runs_everything(self):
        # истории нет / база недоступна — выбираются все маршруты и проверка достижений
        values = self.run_main(["--base", "unknown", "--head", "head"], None)
        self.assertEqual(values["count"], str(len(select_routes.load_routes())))
        self.assertEqual(values["achievements"], "true")
        self.assertEqual(values["build"], "true")

    def test_achievement_test_only_change_is_count_zero_with_achievements(self):
        values = self.run_main(["--base", "base", "--head", "head"],
                               [select_routes.ACHIEVEMENT_TEST])
        self.assertEqual(values["count"], "0")
        self.assertEqual(values["achievements"], "true")
        self.assertEqual(values["build"], "true")

    def test_docs_only_shortcut_flag_is_rejected(self):
        # старый флаг удалён: иначе им снова можно было бы обойти gate
        with self.assertRaises(SystemExit):
            self.run_main(["--base", "base", "--docs-only-since", "before"],
                          [self.FUNCTIONAL])

    def test_workflow_does_not_reintroduce_docs_only_shortcut(self):
        workflow = (Path(select_routes.ROOT) / ".github/workflows/nlb-sdl-test.yml").read_text(
            encoding="utf-8")
        self.assertNotIn("docs-only-since", workflow)
        # pull request решается только по базе и текущей голове
        self.assertIn('--base "$PR_BASE" --head HEAD', workflow)


if __name__ == "__main__":
    unittest.main()
