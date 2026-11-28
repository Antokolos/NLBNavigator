#include <stdio.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
int main(int argc, char **argv) {
    if (argc < 2) return 2;
    lua_State *lua = luaL_newstate();
    if (!lua) return 2;
    luaL_openlibs(lua);
    lua_createtable(lua, argc - 2, 0);
    for (int i = 2; i < argc; ++i) {
        lua_pushstring(lua, argv[i]);
        lua_rawseti(lua, -2, i - 1);
    }
    lua_setglobal(lua, "arg");
    int rc = luaL_dofile(lua, argv[1]);
    if (rc) fprintf(stderr, "%s\n", lua_tostring(lua, -1));
    lua_close(lua);
    return rc ? 1 : 0;
}
