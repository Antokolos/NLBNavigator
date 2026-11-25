"""Реальные отрицательные сценарии захвата: свёрнутое/перекрытое окно, невалидный HWND."""
import ctypes
import os
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
if sys.platform == "win32":
    import win_gui
else:
    win_gui = None


@unittest.skipUnless(win_gui is not None, "нужен Windows")
class RealWindowStateTest(unittest.TestCase):
    def setUp(self):
        try:
            import tkinter
        except ImportError:
            self.skipTest("tkinter недоступен")
        self.tk = tkinter
        self.root = tkinter.Tk()
        self.root.geometry("300x200+100+100")
        self.root.update()
        self.pid = os.getpid()
        self.hwnd = win_gui.user32.GetParent(self.root.winfo_id()) or self.root.winfo_id()

    def tearDown(self):
        try:
            self.root.destroy()
        except Exception:
            pass

    def test_invalid_hwnd(self):
        state = win_gui.window_state(0, self.pid)
        self.assertFalse(state["exists"])
        self.assertTrue(win_gui.check_capture_state(state, self.pid, (0, 0, 1, 1)))

    def test_minimized_window(self):
        self.root.iconify()
        self.root.update()
        state = win_gui.window_state(self.hwnd, self.pid)
        self.assertTrue(state["exists"])
        self.assertTrue(state["iconic"] or not state["visible"])

    def test_covered_window(self):
        top = self.tk.Toplevel(self.root)
        top.geometry("300x200+100+100")
        top.attributes("-topmost", True)
        top.update()
        state = win_gui.window_state(self.hwnd, self.pid)
        self.assertTrue(state["covered"], state)
        top.destroy()


    def test_small_overlap_between_control_points(self):
        top = self.tk.Toplevel(self.root)
        # 24x24 в точке, не совпадающей с пятью контрольными (центр/углы 10%/90%)
        top.geometry("24x24+250+250")
        top.attributes("-topmost", True)
        top.update()
        state = win_gui.window_state(self.hwnd, self.pid)
        self.assertTrue(state["covered"], state)
        top.destroy()


if __name__ == "__main__":
    unittest.main()
