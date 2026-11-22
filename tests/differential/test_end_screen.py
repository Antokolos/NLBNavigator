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


TRACE_HEADER = ["seq", "kind", "command", "arguments", "rendered", "page_id", "location_id", "room_id",
                "screen_kind", "page_name", "background", "portion_index", "portion_count", "more", "room_text",
                "action_text", "objects_text", "player_text", "wolfiness", "hp", "barExit", "forestExit",
                "wolvesTalk", "guardDone"]


def trace_line(seq, screen_kind, end_screen=None, room="v_end"):
    values = {"seq": str(seq), "kind": "scene", "page_id": "end", "room_id": room, "screen_kind": screen_kind,
              "portion_index": "7", "portion_count": "8", "more": "false"}
    fields = [values.get(name, "") for name in TRACE_HEADER]
    if end_screen is not None:
        fields.append("true" if end_screen else "false")
    return "\t".join(fields) + "\n"


class EndScreenUntilTest(unittest.TestCase):
    """advance_until until "end_screen": экран конца, а не обычный экран вариантов"""

    def test_instead_stops_only_on_try_again(self):
        ending = {"page_id": "end", "screen_kind": "page", "more": False, "end_screen": False}
        choices = {"page_id": "end", "screen_kind": "choices", "more": False, "end_screen": False}
        try_again = dict(choices, end_screen=True)
        self.assertFalse(run_route.instead_reached("end_screen", ending, "end"))
        self.assertFalse(run_route.instead_reached("end_screen", choices, "end"), "обычный экран вариантов")
        self.assertTrue(run_route.instead_reached("end_screen", try_again, "end"))
        self.assertTrue(run_route.instead_reached("choices", choices, "end"), "choices — как прежде")
        self.assertFalse(run_route.instead_reached("end_screen", None, "end"))

    def test_trace_end_screen_column(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "trace.tsv"
            path.write_text("# header\n" + trace_line(1, "page", False) + trace_line(2, "choices", True, "v_end_choices")
                            + trace_line(3, "choices", None, "v_end_choices"), encoding="utf-8")
            records = run_route.read_instead_trace(path)
        self.assertEqual([record["end_screen"] for record in records], [False, True, False],
                         "старая трасса без столбца — не экран конца")
        self.assertIs(run_route.instead_sequence_item(records[1])["end_screen"], True)

    def test_nlbnavigator_script(self):
        route = {"id": "r", "window": {"width": 1920, "height": 1080}, "steps": [{"id": "end", "screenshot": "end.png", "action": {
            "type": "advance_until", "until": "end_screen", "max_clicks": 12, "x_ratio": 0.5, "y_ratio": 0.5}}]}
        script, checkpoints, _ = run_route.script_for_nlbnavigator(route, Path("/tmp/out"))
        self.assertIn("advance-until:end_screen,12,960,540\n", script)
        self.assertEqual(len(checkpoints), 1)

    def test_validation_accepts_end_screen(self):
        step = {"id": "end", "action": {"type": "advance_until", "until": "end_screen", "max_clicks": 12,
                                        "x_ratio": 0.5, "y_ratio": 0.5,
                                        "expect_pages": [{"page_id": "end", "portion_count": 8}]}}
        self.assertEqual([], validate_route({"id": "r", "steps": [step]}))

    def test_whole_step_ending_then_end_screen(self):
        # щелчки по концовке — page/page с текстом, последний — экран конца в обоих движках
        texts = ["Вы бросились на врага.", "Негодяй барахтался.", "Жизнь вернулась в прежнее русло."]
        left = [instead("end", i, 3, text) for i, text in enumerate(texts)]
        right = [nlb("end", i, 3, text, finished=True) for i, text in enumerate(texts)]
        left.append(dict(instead("end", 2, 3, screen="choices"), end_screen=True))
        right.append(nlb("end", 2, 3, screen="finished", finished=True))
        self.assertIsNone(run_route.compare_advance_sequences(left, right))
        self.assertEqual(run_route.page_sequence(right), [{"page_id": "end", "portion_count": 3}],
                         "expect_pages: страница концовки и число порций её текста")
        # подмена текста концовки
        wrong = list(right)
        wrong[1] = nlb("end", 1, 3, "Негодяй убежал.", finished=True)
        self.assertIn("текст NLBNavigator", run_route.compare_advance_sequences(left, wrong))
        # экран конца раньше: INSTEAD ещё на тексте
        early = list(right)
        early[2] = nlb("end", 2, 3, screen="finished", finished=True)
        self.assertIn("раньше", run_route.compare_advance_sequences(left, early))

    def test_end_screen_against_ordinary_choices_fails(self):
        a = dict(instead("end", 0, 1, screen="choices"), end_screen=False)
        b = nlb("end", 0, 1, screen="finished", finished=True)
        self.assertFalse(run_route.same_end_screen(a, b))
        self.assertIn("экран INSTEAD 'choices', NLBNavigator 'finished'", run_route.compare_advance_sequences([a], [b]))


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
