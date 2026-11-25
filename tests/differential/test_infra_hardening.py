"""Unit-тесты инфраструктуры: безопасность путей, каталог результатов, захват, завершение."""
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent))
import local_route  # noqa: E402
import run_route  # noqa: E402

if sys.platform == "win32":
    import win_gui
else:
    win_gui = None


class OutputDirectoryTest(unittest.TestCase):
    def test_new_directory_is_created(self):
        with tempfile.TemporaryDirectory() as directory:
            target = Path(directory) / "new"
            self.assertEqual(local_route.prepare_output(target), target)
            self.assertTrue(target.is_dir())

    def test_existing_non_empty_directory_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            target = Path(directory)
            (target / "old.png").write_text("x", encoding="utf-8")
            with self.assertRaises(SystemExit):
                local_route.prepare_output(target)

    def test_reuse_flag_allows_existing(self):
        with tempfile.TemporaryDirectory() as directory:
            target = Path(directory)
            (target / "old.png").write_text("x", encoding="utf-8")
            self.assertEqual(local_route.prepare_output(target, reuse=True), target)


class SeedHomeTest(unittest.TestCase):
    def test_relative_path_inside_home(self):
        with tempfile.TemporaryDirectory() as directory:
            home = Path(directory)
            target = run_route.safe_seed_path(home, ".nlbnavigator/wolfman.saves/slot1.sav")
            self.assertTrue(str(target).startswith(str(home.resolve())))
            self.assertEqual(target.name, "slot1.sav")

    def test_absolute_path_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaises(ValueError):
                run_route.safe_seed_path(directory, str(Path(directory).resolve() / "x"))

    def test_parent_traversal_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaises(ValueError):
                run_route.safe_seed_path(directory, "../outside.sav")
            with self.assertRaises(ValueError):
                run_route.safe_seed_path(directory, "a/../../outside.sav")

    def test_drive_relative_and_unc_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            for value in ("C:evil.sav", "\\\\server\\share\\evil.sav", "//server/share/evil.sav"):
                with self.assertRaises(ValueError):
                    run_route.safe_seed_path(directory, value)

    def test_empty_and_dot_paths_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            for value in ("", ".", "./"):
                if value == "":
                    with self.assertRaises(ValueError):
                        run_route.safe_seed_path(directory, value)


class RemoveTreeTest(unittest.TestCase):
    def test_symlink_to_source_is_not_followed(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "source"
            source.mkdir()
            (source / "data.bin").write_text("original", encoding="utf-8")
            overlay = root / "overlay"
            overlay.mkdir()
            link = overlay / "source-link"
            try:
                link.symlink_to(source, target_is_directory=True)
            except OSError:
                self.skipTest("symlink недоступен")
            run_route.remove_tree(overlay)
            self.assertFalse(overlay.exists())
            self.assertEqual((source / "data.bin").read_text(encoding="utf-8"), "original")


@unittest.skipUnless(win_gui is not None, "проверка кадра — Windows")
class CaptureStateTest(unittest.TestCase):
    def state(self, **overrides):
        value = {"exists": True, "pid": 42, "visible": True, "iconic": False,
                 "bounds": (10, 20, 1920, 1080), "covered": False, "foreground": True}
        value.update(overrides)
        return value

    def test_valid_state(self):
        self.assertEqual(win_gui.check_capture_state(self.state(), 42, (10, 20, 1920, 1080)), [])

    def test_invalid_states(self):
        cases = [
            (self.state(exists=False), "HWND"),
            (self.state(pid=43), "PID"),
            (self.state(visible=False), "невидимо"),
            (self.state(iconic=True), "свёрнуто"),
            (self.state(bounds=(0, 0, 800, 600)), "геометрия"),
            (self.state(covered=True), "перекрыто"),
            (self.state(foreground=False), "переднем плане"),
        ]
        for state, message in cases:
            errors = win_gui.check_capture_state(state, 42, (10, 20, 1920, 1080))
            self.assertTrue(any(message in error for error in errors), (message, errors))


@unittest.skipUnless(win_gui is not None, "завершение INSTEAD — Windows")
class StopProcessTest(unittest.TestCase):
    class FakeProcess:
        pid = 123
        _nlb_hwnd = 456

        def __init__(self, wait_results):
            self.wait_results = list(wait_results)
            self.calls = []

        def poll(self):
            return None

        def wait(self, timeout=None):
            self.calls.append(("wait", timeout))
            result = self.wait_results.pop(0)
            if isinstance(result, BaseException):
                raise result
            return result

        def terminate(self):
            self.calls.append(("terminate",))

        def kill(self):
            self.calls.append(("kill",))

    def run_stop(self, process, window_alive=True, close_ok=True):
        gui = SimpleNamespace(is_window_alive=lambda hwnd, pid: window_alive,
                              close_window=lambda hwnd: close_ok)
        with mock.patch.object(run_route, "IS_WINDOWS", True), \
                mock.patch.object(run_route, "win_gui", gui):
            run_route.stop_process_group(process, "INSTEAD", Path("cleanup.log"))
        return process.calls

    def test_graceful_close_waits_before_any_terminate(self):
        process = self.FakeProcess([0])
        calls = self.run_stop(process)
        self.assertEqual(calls, [("wait", 5)])
        self.assertNotIn(("terminate",), calls)

    def test_timeout_terminates_after_close(self):
        process = self.FakeProcess([subprocess.TimeoutExpired("x", 5), 0])
        calls = self.run_stop(process)
        self.assertEqual(calls, [("wait", 5), ("terminate",), ("wait", 2)])

    def test_terminate_timeout_kills(self):
        process = self.FakeProcess([subprocess.TimeoutExpired("x", 5),
                                    subprocess.TimeoutExpired("x", 2), 0])
        calls = self.run_stop(process)
        self.assertEqual(calls, [("wait", 5), ("terminate",), ("wait", 2), ("kill",), ("wait", 2)])

    def test_missing_window_terminates(self):
        process = self.FakeProcess([0])
        calls = self.run_stop(process, window_alive=False)
        self.assertEqual(calls, [("terminate",), ("wait", 2)])

    def test_close_failure_terminates(self):
        process = self.FakeProcess([0])
        calls = self.run_stop(process, close_ok=False)
        self.assertEqual(calls, [("terminate",), ("wait", 2)])


if __name__ == "__main__":
    unittest.main()
