"""Граничные случаи trace INSTEAD при save/load (issue #69).

Проверяются именно отрицательные сценарии: изменение состояния при сохранении (в том числе
с пустым rendered), отсутствие подтверждения команды, загрузка без свежей сцены, неверное
состояние после загрузки и переинициализация трассы.
"""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import run_route  # noqa: E402


def record(seq, command="look", kind="scene", rendered="Текст сцены", page="page-1",
           portion=0, **variables):
    item = {"seq": seq, "kind": kind, "command": command, "rendered": rendered,
            "page_id": page, "screen_kind": "page", "portion_index": portion}
    for name in run_route.TRACE_VARIABLES:
        item[name] = str(variables.get(name, ""))
    return item


def empty_scene(seq, page="page-1", portion=0, **variables):
    return record(seq, rendered="", page=page, portion=portion, **variables)


def save_evidence(seq):
    return record(seq, command="save D:/saves/save9", kind="fmt", rendered="")


def load_evidence(seq, rendered="Загруженный текст", page="page-1", portion=0):
    return record(seq, command="load D:/saves/save9", rendered=rendered, page=page, portion=portion)


class InsteadMenuKindTest(unittest.TestCase):
    def test_only_f8_is_save(self):
        # F5 в INSTEAD 3.5.2 — look, а не сохранение (src/game.c, kbd_instead)
        self.assertEqual(run_route.instead_menu_kind(
            {"type": "menu", "instead": [{"key": "F8"}]}), "save")
        self.assertIsNone(run_route.instead_menu_kind(
            {"type": "menu", "instead": [{"key": "F5"}]}))

    def test_only_f9_is_load(self):
        self.assertEqual(run_route.instead_menu_kind(
            {"type": "menu", "instead": [{"key": "F9"}]}), "load")
        self.assertIsNone(run_route.instead_menu_kind(
            {"type": "menu", "instead": [{"key": "F6"}]}))

    def test_other_menu_and_actions(self):
        self.assertIsNone(run_route.instead_menu_kind({"type": "menu", "instead": [{"x_ratio": 0.5}]}))
        self.assertIsNone(run_route.instead_menu_kind({"type": "click"}))


class PickSaveTest(unittest.TestCase):
    """Сохранение: нужна свежая запись save и неизменное semantic_state."""

    def previous(self):
        return run_route.semantic_state(record(1, rendered="старый текст"))

    def test_save_uses_confirmed_scene_when_only_empty_scene_appeared(self):
        after = [record(1, rendered="старый текст"), save_evidence(2), empty_scene(3)]
        new_events = [save_evidence(2), empty_scene(3)]
        item, errors = run_route.pick_instead_state_record(
            "menu", "save", after, new_events, self.previous())
        self.assertEqual(item["seq"], 1)
        self.assertEqual(errors, [])

    def test_save_without_command_evidence_rejected(self):
        after = [record(1, rendered="старый текст"), empty_scene(2)]
        item, errors = run_route.pick_instead_state_record(
            "menu", "save", after, [empty_scene(2)], self.previous())
        self.assertIsNone(item)
        self.assertTrue(any("не подтверждено" in error for error in errors))

    def test_save_with_empty_rendered_but_changed_page_rejected(self):
        after = [record(1, rendered="старый текст"), save_evidence(2),
                 empty_scene(3, page="page-B")]
        item, errors = run_route.pick_instead_state_record(
            "menu", "save", after, [save_evidence(2), empty_scene(3, page="page-B")], self.previous())
        self.assertIsNone(item)
        self.assertTrue(any("изменило состояние" in error for error in errors))

    def test_save_with_empty_rendered_but_changed_portion_rejected(self):
        after = [record(1, rendered="старый текст"), save_evidence(2),
                 empty_scene(3, portion=1)]
        item, errors = run_route.pick_instead_state_record(
            "menu", "save", after, [save_evidence(2), empty_scene(3, portion=1)], self.previous())
        self.assertIsNone(item)
        self.assertTrue(any("изменило состояние" in error for error in errors))

    def test_save_with_empty_rendered_but_changed_variables_rejected(self):
        after = [record(1, rendered="старый текст"), save_evidence(2),
                 empty_scene(3, wolfiness=2)]
        item, errors = run_route.pick_instead_state_record(
            "menu", "save", after, [save_evidence(2), empty_scene(3, wolfiness=2)], self.previous())
        self.assertIsNone(item)
        self.assertTrue(any("изменило состояние" in error for error in errors))

    def test_save_of_arbitrary_old_scene_without_fresh_evidence_rejected(self):
        after = [record(1, rendered="старый текст")]
        item, errors = run_route.pick_instead_state_record(
            "menu", "save", after, [], self.previous())
        self.assertIsNone(item)
        self.assertTrue(errors)


class PickLoadTest(unittest.TestCase):
    """Загрузка: нужна свежая сцена с текстом после записи load."""

    def test_load_without_fresh_scene_rejected(self):
        after = [load_evidence(2), record(1, rendered="старый текст")]
        item, errors = run_route.pick_instead_state_record(
            "menu", "load", after, [load_evidence(2)], None)
        self.assertIsNone(item)
        self.assertTrue(any("свежей сцены" in error for error in errors))

    def test_load_with_fresh_empty_rendered_rejected(self):
        after = [load_evidence(2), empty_scene(3)]
        item, errors = run_route.pick_instead_state_record(
            "menu", "load", after, [load_evidence(2), empty_scene(3)], None)
        self.assertIsNone(item)
        self.assertTrue(any("свежей сцены" in error for error in errors))

    def test_load_with_fresh_scene_succeeds(self):
        after = [load_evidence(2), record(3, rendered="загруженный текст", portion=1)]
        item, errors = run_route.pick_instead_state_record(
            "menu", "load", after, [load_evidence(2), record(3, rendered="загруженный текст", portion=1)], None)
        self.assertEqual(item["seq"], 3)
        self.assertEqual(errors, [])

    def test_load_without_evidence_rejected(self):
        after = [record(1, rendered="старый текст"), record(3, rendered="свежий")]
        item, errors = run_route.pick_instead_state_record(
            "menu", "load", after, [record(3, rendered="свежий")], None)
        self.assertIsNone(item)
        self.assertTrue(any("не подтверждено" in error for error in errors))

    def test_load_with_wrong_expected_state_rejected(self):
        after = [load_evidence(2), record(3, rendered="загруженный текст", page="page-B")]
        item, errors = run_route.pick_instead_state_record(
            "menu", "load", after, [load_evidence(2), record(3, rendered="загруженный текст", page="page-B")],
            None, expected={"page_id": "page-A"})
        self.assertEqual(item["seq"], 3)
        self.assertTrue(any("ожидалась" in error for error in errors))

    def test_load_with_correct_expected_state(self):
        after = [load_evidence(2), record(3, rendered="загруженный текст", page="page-A", portion=1)]
        item, errors = run_route.pick_instead_state_record(
            "menu", "load", after, [load_evidence(2), record(3, rendered="загруженный текст", page="page-A", portion=1)],
            None, expected={"page_id": "page-A", "portion_index": 1})
        self.assertEqual(item["seq"], 3)
        self.assertEqual(errors, [])


class TraceLifecycleTest(unittest.TestCase):
    def identity(self, records):
        return [(item["seq"], item["kind"], item.get("command", ""), item.get("page_id", ""),
                 item.get("rendered", ""), item.get("portion_index", 0)) for item in records]

    def test_appended_records_are_not_reset(self):
        before = self.identity([record(1), record(2)])
        after = self.identity([record(1), record(2), record(3)])
        self.assertFalse(run_route.trace_reinitialized(before, after))

    def test_rewritten_trace_with_same_length_and_new_prefix_is_reset(self):
        before = self.identity([record(1, rendered="старый"), record(2, rendered="старый-2")])
        after = self.identity([load_evidence(1), record(2, rendered="загруженный")])
        self.assertTrue(run_route.trace_reinitialized(before, after))

    def test_rewritten_trace_longer_than_cursor_is_reset(self):
        before = self.identity([record(1), record(2)])
        after = self.identity([load_evidence(1), record(2), record(3), record(4)])
        self.assertTrue(run_route.trace_reinitialized(before, after))

    def test_reset_rejected_for_save_and_load(self):
        for kind in ("save", "load"):
            item, errors = run_route.pick_instead_state_record(
                "menu", kind, [record(1)], [], None, trace_reset=True)
            self.assertIsNone(item)
            self.assertTrue(any("переинициализирована" in error for error in errors))

    def test_capture_falls_back_to_last_scene(self):
        after = [record(1, rendered="текст"), empty_scene(2)]
        item, errors = run_route.pick_instead_state_record("capture", None, after, [], None)
        self.assertEqual(item["seq"], 2)
        self.assertEqual(errors, [])


if __name__ == "__main__":
    unittest.main()
