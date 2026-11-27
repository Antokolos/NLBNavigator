"""Граничные случаи trace INSTEAD при save/load (issue #69)."""
import sys
import unittest
from unittest import mock
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import run_route  # noqa: E402


def record(seq, command="look", kind="scene", rendered="Текст сцены", page="page-1",
           portion=0, **variables):
    item = {"seq": seq, "kind": kind, "command": command, "rendered": rendered,
            "page_id": page, "screen_kind": "page", "portion_index": portion,
            "portion_count": 0, "more": False, "background": "", "end_screen": False}
    for name in run_route.TRACE_VARIABLES:
        item[name] = str(variables.get(name, ""))
    return item


def empty_scene(seq, page="page-1", portion=0, **variables):
    return record(seq, rendered="", page=page, portion=portion, **variables)


def save_evidence(seq):
    return record(seq, command="save D:/saves/save9", kind="event", rendered="")


def load_evidence(seq, rendered="Загруженный текст", page="page-1", portion=0):
    return record(seq, command="load D:/saves/save9", rendered=rendered, page=page, portion=portion)


def load_command(seq):
    """Запись команды load без сцены; в реальной трассе обычно совмещена со scene."""
    return record(seq, command="load D:/saves/save9", kind="event", rendered="")


class InsteadMenuKindTest(unittest.TestCase):
    def test_only_f8_is_save(self):
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
    def previous(self):
        return run_route.semantic_state(record(1, rendered="старый текст"))

    def test_save_uses_confirmed_scene_when_only_empty_scene_appeared(self):
        after = [record(1, rendered="старый текст"), save_evidence(2), empty_scene(3)]
        item, errors = run_route.pick_instead_state_record(
            "menu", "save", after, [save_evidence(2), empty_scene(3)], self.previous())
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
    def test_load_without_fresh_scene_rejected(self):
        after = [load_command(2)]
        item, errors = run_route.pick_instead_state_record(
            "menu", "load", after, [load_command(2)], None)
        self.assertIsNone(item)
        self.assertTrue(any("свежей сцены" in error for error in errors))

    def test_load_with_fresh_empty_rendered_rejected(self):
        after = [load_command(2), empty_scene(3)]
        item, errors = run_route.pick_instead_state_record(
            "menu", "load", after, [load_command(2), empty_scene(3)], None)
        self.assertIsNone(item)
        self.assertTrue(any("свежей сцены" in error for error in errors))

    def test_load_with_fresh_scene_succeeds(self):
        after = [load_command(2), record(3, rendered="загруженный текст", portion=1)]
        item, errors = run_route.pick_instead_state_record(
            "menu", "load", after,
            [load_command(2), record(3, rendered="загруженный текст", portion=1)], None)
        self.assertEqual(item["seq"], 3)
        self.assertEqual(errors, [])

    def test_load_record_that_is_scene_with_text_accepted(self):
        # Реальная трасса INSTEAD: команда load и сцена — одна запись scene.
        after = [load_evidence(2)]
        item, errors = run_route.pick_instead_state_record(
            "menu", "load", after, [load_evidence(2)], None)
        self.assertEqual(item["seq"], 2)
        self.assertEqual(errors, [])

    def test_load_without_evidence_rejected(self):
        after = [record(1, rendered="старый текст"), record(3, rendered="свежий")]
        item, errors = run_route.pick_instead_state_record(
            "menu", "load", after, [record(3, rendered="свежий")], None)
        self.assertIsNone(item)
        self.assertTrue(any("не подтверждено" in error for error in errors))

    def test_load_with_wrong_expected_state_rejected(self):
        after = [load_command(2), record(3, rendered="загруженный текст", page="page-B")]
        item, errors = run_route.pick_instead_state_record(
            "menu", "load", after,
            [load_command(2), record(3, rendered="загруженный текст", page="page-B")],
            None, expected={"page_id": "page-A"})
        self.assertEqual(item["seq"], 3)
        self.assertTrue(any("ожидалась" in error for error in errors))

    def test_load_with_correct_expected_state(self):
        after = [load_command(2), record(3, rendered="загруженный текст", page="page-A", portion=1)]
        item, errors = run_route.pick_instead_state_record(
            "menu", "load", after,
            [load_command(2), record(3, rendered="загруженный текст", page="page-A", portion=1)],
            None, expected={"page_id": "page-A", "portion_index": 1})
        self.assertEqual(item["seq"], 3)
        self.assertEqual(errors, [])


    def test_load_rejects_scene_before_command(self):
        scenes = [record(2), load_command(3)]
        item, errors = run_route.pick_instead_state_record(
            "menu", "load", scenes, scenes, None)
        self.assertIsNone(item)
        self.assertTrue(any("свежей сцены" in error for error in errors))

    def test_wrong_or_incomplete_expected_scene_is_rejected(self):
        expected = {"instead_page_id": "page-A", "instead_screen_kind": "page",
                    "portion_index": 0, "portion_count": 2,
                    "variables": dict(zip(run_route.TRACE_VARIABLES, (0, 2, False, False, False, False)))}
        valid = load_evidence(2, page="page-A")
        valid.update(portion_count=2, wolfiness="0", hp="2", barExit="false",
                     forestExit="false", wolvesTalk="false", guardDone="false")
        for field, value in (("page_id", "page-B"), ("screen_kind", "choices"),
                             ("portion_index", 1), ("portion_count", 4), ("hp", "3"),
                             ("forestExit", None), ("portion_count", None)):
            with self.subTest(field=field, value=value):
                candidate = dict(valid)
                if value is None:
                    del candidate[field]
                else:
                    candidate[field] = value
                _, errors = run_route.pick_instead_state_record(
                    "menu", "load", [candidate], [candidate], None, expected=expected)
                self.assertTrue(errors)
        item, errors = run_route.pick_instead_state_record(
            "menu", "load", [valid], [valid], None, expected=expected)
        self.assertEqual(item, valid)
        self.assertEqual(errors, [])


class ExpectedCommandTest(unittest.TestCase):
    def test_save_confirmed_only_by_load_rejected(self):
        item, errors = run_route.pick_instead_state_record(
            "menu", "save", [load_evidence(2)], [load_evidence(2)],
            run_route.semantic_state(record(1, rendered="старый текст")))
        self.assertIsNone(item)
        self.assertTrue(any("save" in error for error in errors))

    def test_load_confirmed_only_by_save_rejected(self):
        item, errors = run_route.pick_instead_state_record(
            "menu", "load", [save_evidence(2), record(3)], [save_evidence(2), record(3)], None)
        self.assertIsNone(item)
        self.assertTrue(any("load" in error for error in errors))

    def test_autosave_and_other_slots_do_not_confirm_quick_commands(self):
        for verb in ("save", "load"):
            for target in ("/tmp/game/autosave", r"D:\Game Saves\save1",
                           "/tmp/game/save90", "/tmp/game/save9.bak", "/tmp/save9/", ""):
                with self.subTest(verb=verb, target=target):
                    old = record(1)
                    events = [record(2, command=f"{verb} {target}".rstrip()), record(3)]
                    item, errors = run_route.pick_instead_state_record(
                        "menu", verb, [old, *events], events, run_route.semantic_state(old))
                    self.assertIsNone(item)
                    self.assertTrue(any("не подтверждено" in error for error in errors))

    def test_fmt_does_not_confirm_save_even_with_an_unrelated_scene(self):
        old = record(1)
        for command in ("save", "save /tmp/game/save9"):
            with self.subTest(command=command):
                diagnostic = record(2, command=command, kind="fmt", rendered="")
                for events in ([diagnostic], [diagnostic, record(3)]):
                    item, errors = run_route.pick_instead_state_record(
                        "menu", "save", [old, *events], events, run_route.semantic_state(old))
                    self.assertIsNone(item)
                    self.assertTrue(any("не подтверждено" in error for error in errors))

    def test_save_needs_a_scene_at_or_after_the_command(self):
        old = record(1)
        # Сцена текущего шага, записанная до команды, тоже не подтверждает результат.
        events = [empty_scene(2), save_evidence(3)]
        item, errors = run_route.pick_instead_state_record(
            "menu", "save", [old, *events], events, run_route.semantic_state(old))
        self.assertIsNone(item)
        self.assertTrue(any("свежей сцены" in error for error in errors))

    def test_save_rejects_incomplete_and_changed_scene(self):
        old = record(1)
        for field, bad_value in (("portion_count", None), ("guardDone", None),
                                 ("portion_count", 2), ("more", True),
                                 ("screen_kind", "choices"), ("background", "other.png"),
                                 ("end_screen", True)):
            with self.subTest(field=field, value=bad_value):
                saved = dict(empty_scene(2), command="save /tmp/game/save9")
                if bad_value is None:
                    del saved[field]
                else:
                    saved[field] = bad_value
                item, errors = run_route.pick_instead_state_record(
                    "menu", "save", [old, saved], [saved], run_route.semantic_state(old))
                self.assertIsNone(item)
                self.assertTrue(errors)

    def test_save_with_empty_rendered_and_load_with_text_on_both_platforms(self):
        old = record(1)
        for target in ("/tmp/game with spaces/save9", r"D:\Game Saves\save9"):
            with self.subTest(target=target):
                saved = dict(empty_scene(2), command=f"save {target}")
                loaded = record(3, command=f"load {target}", rendered="Текст после загрузки")
                for verb, event, expected in (("save", saved, old), ("load", loaded, loaded)):
                    item, errors = run_route.pick_instead_state_record(
                        "menu", verb, [old, event], [event], run_route.semantic_state(old))
                    self.assertEqual((item, errors), (expected, []))

    def test_wait_ignores_wrong_slot_and_fmt_until_real_load(self):
        old = [record(1)]
        wrong_slot = [*old, record(2, command=r"load D:\Game Saves\save1")]
        diagnostic = [*wrong_slot, record(3, command=r"load D:\Game Saves\save9", kind="fmt"),
                      record(4)]
        loaded = [*diagnostic, record(5, command=r"load D:\Game Saves\save9")]
        with mock.patch.object(run_route, "read_instead_trace",
                               side_effect=[wrong_slot, diagnostic, loaded]):
            result = run_route.wait_instead_fresh_scene(
                Path("trace.tsv"), 1, expected_command="load",
                previous_identity=run_route.trace_identity(old))
        self.assertEqual(result, (loaded, False))


class FullIdentityTest(unittest.TestCase):
    def test_changed_variable_in_old_record_is_detected(self):
        before = run_route.trace_identity([record(1, wolfiness=0)])
        after = run_route.trace_identity([record(1, wolfiness=2)])
        self.assertTrue(run_route.trace_reinitialized(before, after))

    def test_changed_screen_kind_is_detected(self):
        before = run_route.trace_identity([record(1)])
        after = run_route.trace_identity([record(1)])
        after[0] = tuple(sorted((k, "choices" if k == "screen_kind" else v) for k, v in after[0]))
        self.assertTrue(run_route.trace_reinitialized(before, after))

    def test_duplicate_and_non_monotonic_seq_rejected(self):
        self.assertFalse(run_route.trace_sequence_valid([record(1), record(1)]))
        self.assertFalse(run_route.trace_sequence_valid([record(2), record(1)]))
        self.assertTrue(run_route.trace_sequence_valid([record(1), record(2)]))

    def test_rewrite_during_wait_detected(self):
        before = run_route.trace_identity([record(1, rendered="старый")])
        self.assertTrue(run_route.wait_identity_invalid(before, [load_evidence(1), record(2)]))

    def test_wait_for_load_not_satisfied_by_earlier_scene(self):
        earlier = [record(1, rendered="промежуточная сцена")]
        loaded = [record(1, rendered="промежуточная сцена"), load_evidence(2)]
        with mock.patch.object(run_route, "read_instead_trace", side_effect=[earlier, loaded]):
            result = run_route.wait_instead_fresh_scene(Path("trace.tsv"), 0,
                                                         expected_command="load", timeout=2)
        self.assertEqual(result, (loaded, False))

    def test_wait_ignores_load_from_previous_step(self):
        previous = [load_evidence(1)]
        unrelated = previous + [record(2)]
        loaded = unrelated + [load_evidence(3)]
        with mock.patch.object(run_route, "read_instead_trace", side_effect=[unrelated, loaded]):
            result = run_route.wait_instead_fresh_scene(
                Path("trace.tsv"), 1, expected_command="load",
                previous_identity=run_route.trace_identity(previous))
        self.assertEqual(result, (loaded, False))

    def test_wait_rejects_rewrite_of_record_first_seen_during_wait(self):
        previous = [record(1)]
        appended = previous + [record(2, rendered="первый опрос")]
        rewritten = previous + [record(2, rendered="перезаписано"), load_evidence(3)]
        with mock.patch.object(run_route, "read_instead_trace", side_effect=[appended, rewritten]):
            result = run_route.wait_instead_fresh_scene(
                Path("trace.tsv"), 1, expected_command="load",
                previous_identity=run_route.trace_identity(previous))
        self.assertEqual(result, (rewritten, True))

    def test_wait_rejects_duplicate_even_with_fresh_load(self):
        records = [record(1), load_evidence(2), record(2)]
        with mock.patch.object(run_route, "read_instead_trace", return_value=records):
            result = run_route.wait_instead_fresh_scene(Path("trace.tsv"), 1, expected_command="load")
        self.assertEqual(result, (records, True))


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
