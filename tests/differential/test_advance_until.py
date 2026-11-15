"""Действие advance_until, нормализация типографики и сверка движков по порциям (без движков)."""

import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
sys.path.insert(0, str(Path(__file__).resolve().parent / "ci"))

import run_route  # noqa: E402
import select_routes  # noqa: E402
from route_validation import validate_route  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
EXAMPLE = Path(__file__).resolve().parent / "examples" / "wolfman-police-hospital-doctor-advance.json"


def item(page, index, count, text="", screen="page", **variables):
    return {"page_id": page, "portion_index": index, "portion_count": count, "screen": screen,
            "text": text, "rendered": text, "variables": {k: str(v) for k, v in variables.items()}}


class TypographyTest(unittest.TestCase):
    def test_dashes_and_quotes_as_game_screen(self):
        self.assertEqual(run_route.typography("Я -- монстр"), "Я — монстр")
        self.assertEqual(run_route.typography("a---b"), "a---b")
        self.assertEqual(run_route.typography('"текст" и _"x"_'), "«текст» и «x»")
        self.assertEqual(run_route.typography(",,низ''"), "„низ”")

    def test_comparable_text_collapses_whitespace(self):
        self.assertEqual(run_route.comparable_text("  А --\n\nБ  "), "А — Б")


class SequenceCompareTest(unittest.TestCase):
    def test_same_sequences(self):
        left = [item("p1", 1, 3, "— Надо проверить"), item("p1", 2, 3, "Конец")]
        right = [item("p1", 1, 3, run_route.comparable_text("-- Надо проверить")), item("p1", 2, 3, "Конец")]
        self.assertIsNone(run_route.compare_advance_sequences(left, right))

    def test_raw_dashes_would_mismatch(self):
        # без типографики текст NLBNavigator «--» не нашёлся бы в порции INSTEAD «—»
        left = [item("p1", 1, 3, "— Надо проверить")]
        right = [item("p1", 1, 3, "-- Надо проверить")]
        self.assertIn("текст", run_route.compare_advance_sequences(left, right))

    def test_first_difference_is_reported(self):
        left = [item("p1", 1, 3), item("p2", 0, 2)]
        right = [item("p1", 1, 3), item("p2", 0, 4)]
        self.assertEqual(run_route.compare_advance_sequences(left, right),
                         "щелчок 2: portion_count INSTEAD 2, NLBNavigator 4")

    def test_click_count_and_variables(self):
        self.assertIn("щелчков INSTEAD 1, NLBNavigator 2",
                      run_route.compare_advance_sequences([item("p", 0, 2)], [item("p", 0, 2), item("p", 1, 2)]))
        self.assertIn("переменная hp", run_route.compare_advance_sequences(
            [item("p", 0, 1, hp=2)], [item("p", 0, 1, hp=3)]))

    def test_overlay_is_page(self):
        self.assertIsNone(run_route.compare_advance_sequences([item("p", 0, 1, screen="overlay")], [item("p", 0, 1)]))


class ExpectPagesTest(unittest.TestCase):
    STEP = {"id": "s", "action": {"type": "advance_until", "expect_pages": [
        {"page_id": "p1", "portion_count": 3}, {"page_id": "p2"}]}}

    def test_page_sequence_merges_consecutive_portions(self):
        sequence = [item("p1", 1, 3), item("p1", 2, 3), item("p2", 0, 2), item("p2", 1, 2, screen="choices")]
        self.assertEqual(run_route.page_sequence(sequence),
                         [{"page_id": "p1", "portion_count": 3}, {"page_id": "p2", "portion_count": 2}])
        self.assertEqual(run_route.check_expect_pages(self.STEP, sequence, "INSTEAD"), [])

    def test_wrong_pages_and_counts(self):
        self.assertTrue(run_route.check_expect_pages(self.STEP, [item("p1", 1, 3)], "INSTEAD"))
        errors = run_route.check_expect_pages(self.STEP, [item("p1", 1, 4), item("p2", 0, 1)], "NLBNavigator")
        self.assertEqual(errors, ["s: NLBNavigator: на странице p1 порций 4, ожидалось 3"])


class InsteadReachedTest(unittest.TestCase):
    def test_conditions(self):
        page = {"page_id": "p", "screen_kind": "page", "more": True}
        self.assertFalse(run_route.instead_reached("choices", page, "p"))
        self.assertTrue(run_route.instead_reached("choices", dict(page, screen_kind="choices"), "p"))
        self.assertFalse(run_route.instead_reached("page_end", page, "p"))
        self.assertTrue(run_route.instead_reached("page_end", dict(page, more=False), "p"))
        self.assertTrue(run_route.instead_reached("page_change", dict(page, page_id="q"), "p"))
        self.assertFalse(run_route.instead_reached("choices", None, "p"))


class ScriptAndValidationTest(unittest.TestCase):
    def test_example_route_is_valid_and_short(self):
        route = run_route.load_route(EXAMPLE)
        self.assertEqual(len(route["steps"]), 7)
        script, checkpoints, count = run_route.script_for_nlbnavigator(route, Path("/tmp/out"))
        self.assertIn("advance-until:choices,40,960,540\nsettle:", script)
        self.assertIn("advance-until:choices,60,960,540\nsettle:", script)
        self.assertEqual(len(checkpoints), 7)

    def test_validation_errors(self):
        def route(action):
            return {"steps": [{"id": "a", "action": action}]}
        good = {"type": "advance_until", "until": "choices", "max_clicks": 10, "x_ratio": 0.5, "y_ratio": 0.5}
        self.assertEqual(validate_route(route(good)), [])
        for bad, fragment in (
            (dict(good, until="end"), "until"),
            (dict(good, max_clicks=0), "max_clicks"),
            (dict(good, max_clicks=True), "max_clicks"),
            (dict(good, x_ratio=1.5), "x_ratio"),
            (dict(good, expect_pages=[]), "expect_pages"),
            (dict(good, expect_pages=[{"portion_count": 1}]), "page_id"),
            (dict(good, expect_pages=[{"page_id": "p", "portion_count": -1}]), "portion_count"),
            (dict(good, expect_pages=[{"page_id": "p", "count": 1}]), "неизвестные поля"),
            (dict(good, expect_pages=[{"page_id": "p"}, {"page_id": "p"}]), "дважды"),
        ):
            errors = validate_route(route(bad))
            self.assertTrue(any(fragment in error for error in errors), (bad, errors))

    def test_timeouts_count_max_clicks(self):
        relative = EXAMPLE.relative_to(ROOT)
        timeouts = select_routes.timeouts(str(relative))
        route = json.loads(EXAMPLE.read_text(encoding="utf-8"))
        clicks = sum(step["action"].get("max_clicks", 0) for step in route["steps"])
        steps = len(route["steps"])
        self.assertEqual(timeouts, {"instead_timeout": 120 + 8 * steps + 3 * clicks,
                                    "nlb_timeout": 90 + 5 * steps + clicks})


class StateLogCheckTest(unittest.TestCase):
    def test_min_commands(self):
        record = {"schema": 1, "index": 0, "command": "", "ok": True, "pageId": "p", "screen": "page",
                  "portion": {"index": 0, "count": 0, "more": False, "visible": []}, "variables": {},
                  "model": {"layout": "vn", "pageId": "p", "pageImage": {}, "pageText": [], "reactionText": [],
                            "choices": [], "inventory": [], "sceneObjects": [], "finished": False}}
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "state.jsonl"
            lines = [dict(record, index=i, command="click:1,1" if i else "") for i in range(4)]
            path.write_text("".join(json.dumps(line) + "\n" for line in lines), encoding="utf-8")
            tool = str(ROOT / "tools" / "check_state_log.py")
            run = lambda *extra: subprocess.run([sys.executable, tool, str(path), *extra], capture_output=True, text=True)
            self.assertEqual(run("--min-commands", "2").returncode, 0)
            self.assertNotEqual(run("--min-commands", "4").returncode, 0)
            self.assertNotEqual(run("--commands", "2").returncode, 0)


if __name__ == "__main__":
    unittest.main()
