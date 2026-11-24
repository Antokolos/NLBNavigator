"""Windows GUI driver for differential tests: окно INSTEAD, фокус, ввод, снимок.

Используется run_route.py на win32 вместо xdotool/xwininfo/ImageMagick.
Координаты — клиентская область окна (как xwininfo + xdotool в Linux-раннере).
"""
import ctypes
import time
from ctypes import wintypes
from pathlib import Path

user32 = ctypes.WinDLL("user32", use_last_error=True)
kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)

try:
    ctypes.windll.shcore.SetProcessDpiAwareness(2)
except Exception:
    pass

EnumWindowsProc = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)


class MOUSEINPUT(ctypes.Structure):
    _fields_ = [("dx", wintypes.LONG), ("dy", wintypes.LONG), ("mouseData", wintypes.DWORD),
                ("dwFlags", wintypes.DWORD), ("time", wintypes.DWORD),
                ("dwExtraInfo", ctypes.c_size_t)]


class KEYBDINPUT(ctypes.Structure):
    _fields_ = [("wVk", wintypes.WORD), ("wScan", wintypes.WORD), ("dwFlags", wintypes.DWORD),
                ("time", wintypes.DWORD), ("dwExtraInfo", ctypes.c_size_t)]


class HARDWAREINPUT(ctypes.Structure):
    _fields_ = [("uMsg", wintypes.DWORD), ("wParamL", wintypes.WORD), ("wParamH", wintypes.WORD)]


class _INPUTUNION(ctypes.Union):
    _fields_ = [("mi", MOUSEINPUT), ("ki", KEYBDINPUT), ("hi", HARDWAREINPUT)]


class INPUT(ctypes.Structure):
    _fields_ = [("type", wintypes.DWORD), ("union", _INPUTUNION)]

KEYS = {
    "space": 0x20, "escape": 0x1B, "esc": 0x1B, "enter": 0x0D, "return": 0x0D,
    "tab": 0x09, "backspace": 0x08, "left": 0x25, "up": 0x26, "right": 0x27, "down": 0x28,
    "home": 0x24, "end": 0x23, "pageup": 0x21, "pagedown": 0x22,
}
for _index in range(1, 13):
    KEYS["f{}".format(_index)] = 0x6F + _index
for _char in "0123456789":
    KEYS[_char] = ord(_char)
for _char in "abcdefghijklmnopqrstuvwxyz":
    KEYS[_char] = ord(_char.upper())


def _window_text(hwnd):
    length = user32.GetWindowTextLengthW(hwnd)
    buffer = ctypes.create_unicode_buffer(length + 1)
    user32.GetWindowTextW(hwnd, buffer, length + 1)
    return buffer.value


def _client_bounds(hwnd):
    rect = wintypes.RECT()
    if not user32.GetClientRect(hwnd, ctypes.byref(rect)):
        return None
    width, height = rect.right - rect.left, rect.bottom - rect.top
    point = wintypes.POINT(0, 0)
    if not user32.ClientToScreen(hwnd, ctypes.byref(point)):
        return None
    return point.x, point.y, width, height


def _windows_of(pid):
    result = []

    def callback(hwnd, _):
        wpid = wintypes.DWORD()
        user32.GetWindowThreadProcessId(hwnd, ctypes.byref(wpid))
        if wpid.value == pid and user32.IsWindowVisible(hwnd):
            bounds = _client_bounds(hwnd)
            if bounds and bounds[2] >= 640 and bounds[3] >= 400:
                result.append((bounds[2] * bounds[3], hwnd, bounds))
        return True

    user32.EnumWindows(EnumWindowsProc(callback), 0)
    return result


def find_window(pid, timeout=25.0, poll=0.5):
    """(hwnd, (x, y, width, height)) самого большого видимого окна процесса."""
    deadline = time.time() + timeout
    while time.time() < deadline:
        candidates = _windows_of(pid)
        if candidates:
            _, hwnd, bounds = max(candidates)
            return hwnd, bounds
        time.sleep(poll)
    return None


def focus(hwnd):
    user32.ShowWindow(hwnd, 9)  # SW_RESTORE
    user32.AllowSetForegroundWindow(0xFFFFFFFF)  # ASFW_ANY
    if user32.SetForegroundWindow(hwnd):
        return True
    # Иногда Windows запрещает смену фокуса: присоединяем ввод к потоку окна
    foreground = user32.GetForegroundWindow()
    current = kernel32.GetCurrentThreadId()
    target = user32.GetWindowThreadProcessId(foreground, None)
    attached = bool(user32.AttachThreadInput(current, target, True))
    try:
        return bool(user32.SetForegroundWindow(hwnd))
    finally:
        if attached:
            user32.AttachThreadInput(current, target, False)


def key_code(name):
    """VK-код клавиши по имени маршрута; ValueError для неизвестной"""
    name = str(name).lower()
    if name not in KEYS:
        raise ValueError("Неизвестная клавиша для Windows-драйвера: " + name)
    return KEYS[name]


def click_lparam(x, y):
    return (int(y) << 16) | (int(x) & 0xFFFF)


def key_lparams(vk, scan):
    return 1 | (scan << 16), 0xC0000001 | (scan << 16)


def key(hwnd, name):
    vk = key_code(name)
    scan = user32.MapVirtualKeyW(vk, 0)
    focus(hwnd)
    # Сообщения окну: SDL2 обрабатывает WM_KEYDOWN/WM_KEYUP в своём оконной процедуре;
    # SendInput зависит от фокуса, который Windows не всегда отдаёт фоновому процессу.
    down, up = key_lparams(vk, scan)
    user32.PostMessageW(hwnd, 0x0100, vk, down)  # WM_KEYDOWN
    time.sleep(0.05)
    user32.PostMessageW(hwnd, 0x0101, vk, up)    # WM_KEYUP


def click(hwnd, x, y):
    lparam = click_lparam(x, y)
    user32.PostMessageW(hwnd, 0x0200, 0, lparam)        # WM_MOUSEMOVE
    user32.PostMessageW(hwnd, 0x0201, 0x0001, lparam)   # WM_LBUTTONDOWN
    time.sleep(0.05)
    user32.PostMessageW(hwnd, 0x0202, 0, lparam)        # WM_LBUTTONUP


def close_window(hwnd):
    """Штатное закрытие окна (WM_CLOSE): даёт игре сохранить слоты и сбросить trace."""
    user32.PostMessageW(hwnd, 0x0010, 0, 0)


def capture(bounds, path):
    from PIL import ImageGrab

    x, y, width, height = bounds
    image = ImageGrab.grab(bbox=(x, y, x + width, y + height))
    if image.size != (width, height):
        raise RuntimeError("Не удалось снять клиентскую область INSTEAD: {} вместо {}".format(
            image.size, (width, height)))
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    image.save(path)
    return path
