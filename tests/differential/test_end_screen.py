"""Экран конца игры: screen "finished" в state-log и сверка с INSTEAD (синтетические данные, без движков).

screen — состояние экрана: текст концовки, пока его читают, — "page" (как в INSTEAD); "finished" —
показан экран конца VN (GameScreen::showGameOver). «Ходов больше нет» — поле finished
(docs/state-log.md).
"""

import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import run_route  # noqa: E402
from route_validation import validate_route  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
ENDING = "Вы бросились на врага и, сдавив ему горло, держали, пока он не затих."


def instead(page, index, count, text="", screen="page"):
    return {"page_id": page, "portion_index": index, "portion_count": count, "screen": screen,
            "text": text, "rendered": text, "variables": {}}


def nlb(page, index, count, text="", screen="page", finished=False):
    return {"page_id": page, "portion_index": index, "portion_count": count, "screen": screen,
            "finished": finished, "text": text, "variables": {}}


def state_record(screen, finished, index=0, model_finished=None):
    return {"schema": 1, "index": index, "command": "click:1,1" if index else "", "ok": True, "pageId": "p",
            "screen": screen, "finished": finished,
            "portion": {"index": 0, "count": 1, "more": False, "visible": []}, "variables": {},
            "model": {"layout": "vn", "pageId": "p", "pageImage": {}, "pageText": [], "reactionText": [],
                      "choices": [], "inventory": [], "sceneObjects": [],
                      "finished": finished if model_finished is None else model_finished}}


class SequenceItemTest(unittest.TestCase):
    def test_finished_is_carried(self):
        record = state_record("page", True)
        item = run_route.nlbnavigator_sequence_item(record)
        self.assertEqual(item["screen"], "page")
        self.assertIs(item["finished"], True)

    def test_old_log_without_field_takes_model(self):
        record = state_record("page", True)
        del record["finished"]
        self.assertIs(run_route.nlbnavigator_sequence_item(record)["finished"], True)


class EndingCompareTest(unittest.TestCase):
    def test_ending_text_is_page_and_compared(self):
        # текст концовки читается как "page" в обоих движках — и сравнивается
        left = [instead("end", 0, 2, ENDING), instead("end", 1, 2, "Конец истории.")]
        right = [nlb("end", 0, 2, ENDING, finished=True), nlb("end", 1, 2, "Конец истории.", finished=True)]
        self.assertIsNone(run_route.compare_advance_sequences(left, right))

    def test_wrong_ending_text_is_caught(self):
        left = [instead("end", 0, 1, ENDING)]
        right = [nlb("end", 0, 1, "Вы убежали в лес и больше не вернулись.", finished=True)]
        self.assertIn("текст NLBNavigator", run_route.compare_advance_sequences(left, right))

    def test_end_screen_matches_try_again_choices(self):
        # экран конца NLBNavigator = комната v_<id>_choices с _try_again экспорта
        left = [instead("end", 0, 1, ENDING), instead("end", 0, 1, "Попробовать ещё раз", screen="choices")]
        right = [nlb("end", 0, 1, ENDING, finished=True), nlb("end", 0, 1, screen="finished", finished=True)]
        self.assertIsNone(run_route.compare_advance_sequences(left, right))

    def test_end_screen_on_other_page_fails(self):
        a = instead("other", 0, 1, screen="choices")
        b = nlb("end", 0, 1, screen="finished", finished=True)
        self.assertFalse(run_route.same_end_screen(a, b))
        self.assertIn("page_id", run_route.compare_advance_sequences([a], [b]))

    def test_end_screen_without_finished_fails(self):
        left = [instead("end", 0, 1, screen="choices")]
        right = [nlb("end", 0, 1, screen="finished", finished=False)]
        self.assertIn("экран INSTEAD 'choices', NLBNavigator 'finished'",
                      run_route.compare_advance_sequences(left, right))

    def test_early_end_screen_against_page_fails(self):
        # экран конца, пока INSTEAD ещё показывает текст концовки
        left = [instead("end", 0, 1, ENDING)]
        right = [nlb("end", 0, 1, screen="finished", finished=True)]
        error = run_route.compare_advance_sequences(left, right)
        self.assertIn("экран INSTEAD 'page', NLBNavigator 'finished'", error)
        self.assertIn("раньше", error)

    def test_other_pairs_stay_strict(self):
        self.assertIsNotNone(run_route.compare_advance_sequences(
            [instead("end", 0, 1, screen="choices")], [nlb("end", 0, 1, finished=True)]))
        self.assertIsNotNone(run_route.compare_advance_sequences(
            [instead("end", 0, 1, screen="choices")], [nlb("end", 0, 1, screen="message", finished=True)]))


class StateLogCheckTest(unittest.TestCase):
    def run_tool(self, records):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "state.jsonl"
            path.write_text("".join(json.dumps(record) + "\n" for record in records), encoding="utf-8")
            return subprocess.run([sys.executable, str(ROOT / "tools" / "check_state_log.py"), str(path)],
                                  capture_output=True, text=True)

    def test_ending_read_then_end_screen(self):
        run = self.run_tool([state_record("page", True), state_record("finished", True, index=1)])
        self.assertEqual(run.returncode, 0, run.stdout)

    def test_end_screen_requires_finished(self):
        run = self.run_tool([state_record("finished", False)])
        self.assertNotEqual(run.returncode, 0)
        self.assertIn("экран конца", run.stdout)

    def test_finished_matches_model(self):
        run = self.run_tool([state_record("page", True, model_finished=False)])
        self.assertIn("finished записи и model.finished различаются", run.stdout)

    def test_finished_field_is_required(self):
        record = state_record("page", False)
        del record["finished"]
        self.assertIn("нет поля finished", self.run_tool([record]).stdout)


class ExpectFinishedValidationTest(unittest.TestCase):
    def route(self, finished):
        return {"id": "r", "steps": [{"id": "end", "action": {"type": "capture"},
                                      "expect": {"page_id": "end", "screen": "page", "finished": finished}}]}

    def test_bool_is_accepted(self):
        self.assertEqual([], validate_route(self.route(True)))

    def test_non_bool_is_rejected(self):
        self.assertTrue(any("expect.finished" in error for error in validate_route(self.route("yes"))))


if __name__ == "__main__":
    unittest.main()
