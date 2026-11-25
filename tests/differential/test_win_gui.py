"""Unit-тесты Windows-драйвера дифференциальных прогонов (без GUI и SDL)."""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

if sys.platform == "win32":
    import win_gui
else:  # на Linux модуль не импортируется: windll отсутствует
    win_gui = None


@unittest.skipUnless(win_gui is not None, "Windows-драйвер проверяется только на win32")
class KeyCodeTest(unittest.TestCase):
    def test_function_keys_used_by_routes(self):
        self.assertEqual(win_gui.key_code("f2"), 0x71)
        self.assertEqual(win_gui.key_code("f3"), 0x72)
        self.assertEqual(win_gui.key_code("f5"), 0x74)
        self.assertEqual(win_gui.key_code("f8"), 0x77)
        self.assertEqual(win_gui.key_code("f9"), 0x78)
        self.assertEqual(win_gui.key_code("space"), 0x20)
        self.assertEqual(win_gui.key_code("escape"), 0x1B)

    def test_case_insensitive(self):
        self.assertEqual(win_gui.key_code("F8"), win_gui.key_code("f8"))

    def test_unknown_key_rejected(self):
        with self.assertRaises(ValueError):
            win_gui.key_code("volume-up")

    def test_click_lparam_keeps_client_coordinates(self):
        lparam = win_gui.click_lparam(100, 200)
        self.assertEqual(lparam & 0xFFFF, 100)
        self.assertEqual((lparam >> 16) & 0xFFFF, 200)

    def test_key_lparams_down_up(self):
        down, up = win_gui.key_lparams(0x77, 0x41)
        self.assertEqual(down, 1 | (0x41 << 16))
        self.assertEqual(up, 0xC0000001 | (0x41 << 16))
        self.assertEqual(up & 0x80000000, 0x80000000)  # бит перехода вверх


@unittest.skipUnless(win_gui is not None, "Windows-драйвер проверяется только на win32")
class WindowSelectionTest(unittest.TestCase):
    def test_window_of_other_process_is_not_selected(self):
        # _windows_of фильтрует по PID: для несуществующего PID окон быть не может
        self.assertEqual(win_gui._windows_of(0xFFFFFFF0), [])

    def test_find_window_timeout_returns_none(self):
        self.assertIsNone(win_gui.find_window(0xFFFFFFF0, timeout=0.2, poll=0.05))


if __name__ == "__main__":
    unittest.main()
