"""Запуск существующего Lua-стенда через C API установленной LuaJIT."""
import ctypes
import json

from execute import ENV, REPO, ROOT

lib = ctypes.CDLL(str(next((ENV / "root/usr/lib/x86_64-linux-gnu").glob("libluajit-5.1.so.2.1.*"))))
ptr = ctypes.c_void_p
lib.luaL_newstate.restype = ptr
lib.luaL_openlibs.argtypes = [ptr]
lib.luaL_loadstring.argtypes = [ptr, ctypes.c_char_p]
lib.lua_pcall.argtypes = [ptr, ctypes.c_int, ctypes.c_int, ctypes.c_int]
lib.lua_tolstring.argtypes = [ptr, ctypes.c_int, ptr]
lib.lua_tolstring.restype = ctypes.c_char_p
lib.lua_close.argtypes = [ptr]
repo = REPO / "tests/differential"
root = ROOT / "lua-lifecycle"
root.mkdir()
for scenario in ("unchanged", "cmd", "fmt", "both", "nested", "scene", "repeat", "inflight",
                 "global-reset", "append", "new-file"):
    out = root / scenario
    out.mkdir()
    args = [str(repo / "instead_trace.lua"), str(out), scenario]
    code = ("arg={" + ",".join(map(json.dumps, args)) + "}; print(jit.version); dofile("
            + json.dumps(str(repo / "check_instead_trace_lifecycle.lua")) + ")")
    state = lib.luaL_newstate()
    assert state
    try:
        lib.luaL_openlibs(state)
        result = lib.luaL_loadstring(state, code.encode()) or lib.lua_pcall(state, 0, 0, 0)
        if result:
            raise RuntimeError(lib.lua_tolstring(state, -1, None).decode())
    finally:
        lib.lua_close(state)
