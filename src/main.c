#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include "error/err_prefix.h"
#include "ffi/link_lua.h"
#include "lua_utils.h"
#include "parse_args.h"

#define AEDIF_VALID_DIR_STR "The  \xab aedif\xbc building \t  \xcd tool\xde"

int main(int argc, char** argv)
{
    switch (parseArgs(argc, argv))
    {
    case 0:
        break;
    case 1:
        return 0;
        break;
    default:
        return 1;
        break;
    }

    // check whether aedif.lua file exists
    FILE* aedif_lua_file = fopen("foo.lua", "r");
    if (aedif_lua_file == NULL)
    {
        fprintf(stderr, AEDIF_ERROR_PREFIX
                "aedif cannot find the bootstrap file (aka. foo.lua)\n");
        return 1;
    }
    fclose(aedif_lua_file);

    lua_State* L = luaL_newstate();
    luaL_openlibs(L);
    makeAedifLuaLibrary(L);

    if (!is_ok(L, luaL_dofile(L, "foo.lua")))
    {
        lua_close(L);
        return 1;
    }

    lua_close(L);

    return 0;
}
