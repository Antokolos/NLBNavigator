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

# Явные сигнатуры: на x64 HWND/WPARAM/LPARAM — 64-битные, без argtypes ctypes
# может обрезать дескрипторы и координаты.
HWND = wintypes.HWND
UINT = wintypes.UINT
BOOL = wintypes.BOOL
DWORD = wintypes.DWORD
LONG = wintypes.LONG
WPARAM = ctypes.c_size_t
LPARAM = ctypes.c_ssize_t
for _name, _result, _args in [
    ("GetWindowThreadProcessId", DWORD, [HWND, ctypes.POINTER(DWORD)]),
    ("IsWindow", BOOL, [HWND]),
    ("IsWindowVisible", BOOL, [HWND]),
    ("IsIconic", BOOL, [HWND]),
    ("GetClientRect", BOOL, [HWND, ctypes.POINTER(wintypes.RECT)]),
    ("GetWindowRect", BOOL, [HWND, ctypes.POINTER(wintypes.RECT)]),
    ("ClientToScreen", BOOL, [HWND, ctypes.POINTER(wintypes.POINT)]),
    ("ScreenToClient", BOOL, [HWND, ctypes.POINTER(wintypes.POINT)]),
    ("GetForegroundWindow", HWND, []),
    ("SetForegroundWindow", BOOL, [HWND]),
    ("ShowWindow", BOOL, [HWND, ctypes.c_int]),
    ("AllowSetForegroundWindow", BOOL, [DWORD]),
    ("AttachThreadInput", BOOL, [DWORD, DWORD, BOOL]),
    ("PostMessageW", BOOL, [HWND, UINT, WPARAM, LPARAM]),
    ("GetWindowTextLengthW", ctypes.c_int, [HWND]),
    ("GetWindowTextW", ctypes.c_int, [HWND, wintypes.LPWSTR, ctypes.c_int]),
    ("MapVirtualKeyW", UINT, [UINT, UINT]),
    ("GetCursorPos", BOOL, [ctypes.POINTER(wintypes.POINT)]),
    ("WindowFromPoint", HWND, [wintypes.POINT]),
    ("GetAncestor", HWND, [HWND, UINT]),
]:
    _func = getattr(user32, _name)
    _func.restype = _result
    _func.argtypes = _args

kernel32.GetCurrentThreadId.restype = DWORD
kernel32.GetCurrentThreadId.argtypes = []

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
        return user32.GetForegroundWindow() == hwnd
    # Иногда Windows запрещает смену фокуса: присоединяем ввод к потоку окна
    foreground = user32.GetForegroundWindow()
    current = kernel32.GetCurrentThreadId()
    target = user32.GetWindowThreadProcessId(foreground, None)
    attached = bool(user32.AttachThreadInput(current, target, True))
    try:
        return bool(user32.SetForegroundWindow(hwnd)) and user32.GetForegroundWindow() == hwnd
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
    if not focus(hwnd):
        raise RuntimeError("не удалось передать фокус окну для клавиши " + name)
    down, up = key_lparams(vk, scan)
    if not user32.PostMessageW(hwnd, 0x0100, vk, down):  # WM_KEYDOWN
        raise RuntimeError("PostMessageW WM_KEYDOWN не доставлен: " + name)
    time.sleep(0.05)
    if not user32.PostMessageW(hwnd, 0x0101, vk, up):    # WM_KEYUP
        raise RuntimeError("PostMessageW WM_KEYUP не доставлен: " + name)


def click(hwnd, x, y):
    lparam = click_lparam(x, y)
    for message, wparam in ((0x0200, 0), (0x0201, 0x0001), (0x0202, 0)):
        if not user32.PostMessageW(hwnd, message, wparam, lparam):
            raise RuntimeError("PostMessageW мыши не доставлен: 0x{:04x}".format(message))
        if message == 0x0201:
            time.sleep(0.05)


def cursor_client(hwnd):
    """Позиция курсора в координатах клиентской области окна"""
    point = wintypes.POINT()
    if not user32.GetCursorPos(ctypes.byref(point)):
        return None
    if not user32.ScreenToClient(hwnd, ctypes.byref(point)):
        return None
    return point.x, point.y


def close_window(hwnd):
    """Штатное закрытие окна (WM_CLOSE). True — сообщение поставлено в очередь."""
    return bool(user32.PostMessageW(hwnd, 0x0010, 0, 0))


def is_window_alive(hwnd, pid):
    if not hwnd or not user32.IsWindow(hwnd):
        return False
    owner = DWORD()
    user32.GetWindowThreadProcessId(hwnd, ctypes.byref(owner))
    return owner.value == pid and bool(user32.IsWindowVisible(hwnd))


class CaptureError(RuntimeError):
    """Снимок нельзя считать кадром именно этого окна."""


def check_capture_state(state, pid, bounds):
    """Ошибки состояния перед снимком; state — результат window_state()."""
    errors = []
    if not state.get("exists"):
        errors.append("HWND не существует")
    if state.get("pid") != pid:
        errors.append("окно принадлежит PID {}, ожидался {}".format(state.get("pid"), pid))
    if not state.get("visible"):
        errors.append("окно невидимо")
    if state.get("iconic"):
        errors.append("окно свёрнуто")
    if state.get("bounds") != tuple(bounds):
        errors.append("геометрия клиента изменилась: {} != {}".format(state.get("bounds"), tuple(bounds)))
    if state.get("covered"):
        errors.append("окно перекрыто другим окном")
    if not state.get("foreground"):
        errors.append("окно не на переднем плане")
    return errors


def _root(hwnd):
    return user32.GetAncestor(hwnd, 2) or hwnd  # GA_ROOT


def window_state(hwnd, pid):
    """Снимок состояния окна для проверки достоверности кадра."""
    state = {"exists": bool(user32.IsWindow(hwnd)), "pid": None, "visible": False,
             "iconic": False, "bounds": None, "covered": None, "foreground": False}
    if not state["exists"]:
        return state
    owner = DWORD()
    user32.GetWindowThreadProcessId(hwnd, ctypes.byref(owner))
    state["pid"] = owner.value
    state["visible"] = bool(user32.IsWindowVisible(hwnd))
    state["iconic"] = bool(user32.IsIconic(hwnd))
    state["bounds"] = _client_bounds(hwnd)
    state["foreground"] = user32.GetForegroundWindow() == hwnd
    if state["bounds"]:
        x, y, width, height = state["bounds"]
        covered = False
        for ratio_x, ratio_y in ((0.5, 0.5), (0.1, 0.1), (0.9, 0.1), (0.1, 0.9), (0.9, 0.9)):
            point = wintypes.POINT(x + int(width * ratio_x), y + int(height * ratio_y))
            under = user32.WindowFromPoint(point)
            if under and _root(under) != hwnd:
                covered = True
                break
        state["covered"] = covered
    return state


def capture_client(hwnd, pid, bounds, path):
    """Снимок клиентской области с проверкой, что это действительно окно pid."""
    errors = check_capture_state(window_state(hwnd, pid), pid, bounds)
    if errors:
        raise CaptureError("недостоверный кадр: " + "; ".join(errors))
    capture(bounds, path)
    # После захвата повторно проверяем окно: оно могло закрыться/перекрыться.
    errors = check_capture_state(window_state(hwnd, pid), pid, bounds)
    if errors:
        Path(path).unlink(missing_ok=True)
        raise CaptureError("кадр отброшен после захвата: " + "; ".join(errors))


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
