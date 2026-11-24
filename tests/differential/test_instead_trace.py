"""Граничные случаи trace INSTEAD при save/load (issue #69)."""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import run_route  # noqa: E402


def scene(seq, rendered="Текст сцены", page="page-1", portion=0):
    return {"seq": seq, "kind": "scene", "command": "look", "rendered": rendered,
            "page_id": page, "screen_kind": "page", "portion_index": portion}


def empty_scene(seq, page="page-1", portion=0):
    return scene(seq, rendered="", page=page, portion=portion)


class InsteadMenuKindTest(unittest.TestCase):
    def test_save_keys(self):
        self.assertEqual(run_route.instead_menu_kind({"type": "menu", "instead": [{"key": "F8"}]}), "save")
        self.assertEqual(run_route.instead_menu_kind({"type": "menu", "instead": [{"key": "f5"}]}), "save")

    def test_load_keys(self):
        self.assertEqual(run_route.instead_menu_kind({"type": "menu", "instead": [{"key": "F9"}]}), "load")
        self.assertEqual(run_route.instead_menu_kind({"type": "menu", "instead": [{"key": "f6"}]}), "load")

    def test_other_menu_and_actions(self):
        self.assertIsNone(run_route.instead_menu_kind({"type": "menu", "instead": [{"x_ratio": 0.5}]}))
        self.assertIsNone(run_route.instead_menu_kind({"type": "click"}))


class PickInsteadStateTest(unittest.TestCase):
    """Сохранение — прежнее подтверждённое состояние; загрузка — только свежая сцена с текстом."""

    def test_save_uses_last_scene_with_text(self):
        after = [scene(1, "старый текст"), empty_scene(2)]
        record, errors = run_route.pick_instead_state_record("menu", "save", after, [empty_scene(2)], None)
        self.assertEqual(record["seq"], 1)
        self.assertEqual(errors, [])

    def test_save_without_any_text_scene_fails(self):
        record, errors = run_route.pick_instead_state_record("menu", "save", [empty_scene(1)], [], None)
        self.assertIsNone(record)
        self.assertTrue(errors)

    def test_save_that_changed_state_fails(self):
        previous = run_route.trace_state_signature(scene(1, "старый текст"))
        after = [scene(2, "другой текст")]
        record, errors = run_route.pick_instead_state_record("menu", "save", after, [], previous)
        self.assertIsNotNone(record)
        self.assertTrue(any("изменило" in error for error in errors))

    def test_load_without_fresh_scene_fails_even_if_old_text_exists(self):
        after = [scene(1, "старый текст")]
        record, errors = run_route.pick_instead_state_record("menu", "load", after, [], None)
        self.assertIsNone(record)
        self.assertTrue(any("свежей сцены" in error for error in errors))

    def test_load_with_fresh_empty_rendered_fails(self):
        after = [scene(1, "старый текст"), empty_scene(2)]
        record, errors = run_route.pick_instead_state_record("menu", "load", after, [empty_scene(2)], None)
        self.assertIsNone(record)
        self.assertTrue(errors)

    def test_load_with_fresh_scene_succeeds(self):
        after = [scene(1, "старый текст"), scene(2, "загруженный текст", portion=1)]
        record, errors = run_route.pick_instead_state_record("menu", "load", after, [scene(2, "загруженный текст", portion=1)], None)
        self.assertEqual(record["seq"], 2)
        self.assertEqual(record["portion_index"], 1)
        self.assertEqual(errors, [])

    def test_trace_reset_rejected_for_save_and_load(self):
        after = [scene(1, "текст")]
        for kind in ("save", "load"):
            record, errors = run_route.pick_instead_state_record("menu", kind, after, [], None, trace_reset=True)
            self.assertIsNone(record)
            self.assertTrue(any("переинициализирована" in error for error in errors))

    def test_capture_falls_back_to_last_scene(self):
        after = [scene(1, "текст"), empty_scene(2)]
        record, errors = run_route.pick_instead_state_record("capture", None, after, [], None)
        self.assertEqual(record["seq"], 2)
        self.assertEqual(errors, [])


if __name__ == "__main__":
    unittest.main()
