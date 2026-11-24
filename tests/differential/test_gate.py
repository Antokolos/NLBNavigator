"""Правила Differential gate (tests/differential/ci/gate.py).

Главное: пропуск обязательного job не должен давать зелёный gate, а docs-only — должен.
"""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent / "ci"))
import gate  # noqa: E402

DOCS_ONLY = dict(validate="success", plan="success", build="skipped", route="skipped",
                 report="skipped", achievement="skipped", count="0", achievements="false")
ROUTES = dict(DOCS_ONLY, build="success", route="success", report="success", count="22")
ACHIEVEMENT = dict(DOCS_ONLY, build="success", achievement="success", report="success",
                   achievements="true")


def verdict(**overrides):
    data = dict(DOCS_ONLY)
    data.update(overrides)
    return gate.verdict(data["validate"], data["plan"], data["build"], data["route"],
                        data["report"], data["achievement"], data["count"], data["achievements"])


class GateTest(unittest.TestCase):
    def test_docs_only_passes(self):
        self.assertEqual(verdict(), [])

    def test_full_route_run_passes(self):
        self.assertEqual(verdict(**ROUTES), [])

    def test_achievements_only_run_passes(self):
        # COUNT=0, но проверка достижений выполнена — этого достаточно
        self.assertEqual(verdict(**ACHIEVEMENT), [])

    def test_validate_failure_fails(self):
        self.assertTrue(verdict(validate="failure"))

    def test_plan_failure_fails(self):
        self.assertTrue(verdict(plan="failure"))

    def test_empty_count_is_plan_error_not_zero(self):
        problems = verdict(count="")
        self.assertTrue(problems)
        self.assertIn("count", problems[0])

    def test_unknown_achievements_flag_fails(self):
        self.assertTrue(verdict(achievements=""))

    def test_skipped_build_when_routes_selected_fails(self):
        problems = verdict(**dict(ROUTES, build="skipped"))
        self.assertTrue(any("build" in problem for problem in problems))

    def test_skipped_build_when_achievements_selected_fails(self):
        problems = verdict(**dict(ACHIEVEMENT, build="skipped"))
        self.assertTrue(any("build" in problem for problem in problems))

    def test_skipped_achievement_is_failure(self):
        # неожиданный пропуск обязательного job — не зелёный gate
        problems = verdict(**dict(ACHIEVEMENT, achievement="skipped"))
        self.assertTrue(problems)
        self.assertTrue(any("achievement" in problem for problem in problems))

    def test_failed_achievement_is_failure(self):
        problems = verdict(**dict(ACHIEVEMENT, achievement="failure"))
        self.assertTrue(any("achievement" in problem for problem in problems))

    def test_achievement_not_planned_is_fine(self):
        self.assertEqual(verdict(**ROUTES), [])

    def test_route_failure_fails(self):
        problems = verdict(**dict(ROUTES, route="failure"))
        self.assertTrue(any("route" in problem for problem in problems))

    def test_docs_commit_cannot_hide_functional_change(self):
        # в функциональном PR выбраны маршруты; пропуск route (как было при старом docs-only
        # skip) обязан провалить gate, а не дать зелёный
        problems = verdict(**dict(ROUTES, route="skipped"))
        self.assertTrue(problems)
        self.assertTrue(any("route" in problem for problem in problems))

    def test_report_skipped_when_something_ran_fails(self):
        for data in (dict(ROUTES, report="skipped"), dict(ACHIEVEMENT, report="skipped")):
            problems = verdict(**data)
            self.assertTrue(any("report" in problem for problem in problems), data)

    def test_summary_reports_failure(self):
        lines, problems = gate.summary(dict(ACHIEVEMENT, achievement="skipped"))
        self.assertTrue(problems)
        self.assertIn("**Провал:**", "\n".join(lines))
        lines, problems = gate.summary(dict(DOCS_ONLY))
        self.assertEqual(problems, [])
        self.assertIn("Проверка пройдена.", "\n".join(lines))


if __name__ == "__main__":
    unittest.main()
