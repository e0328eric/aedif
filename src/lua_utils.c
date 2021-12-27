#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "lua_utils.h"

int getLineNumber(lua_State* L)
{
    lua_Debug ar;

    if (lua_getstack(L, 1, &ar))
    {
        lua_getinfo(L, "l", &ar);
        return ar.currentline;
    }

	return -1;
}

void dumpStack(lua_State* L)
{
    int top = lua_gettop(L);
    printf("==================================================\n");
    for (int i = 1; i <= top; ++i)
    {
        printf("%d\t%s\t", i, luaL_typename(L, i));
        switch (lua_type(L, i))
        {
        case LUA_TNUMBER:
            printf("%f\n", lua_tonumber(L, i));
            break;

        case LUA_TSTRING:
            printf("%s\n", lua_tostring(L, i));
            break;

        case LUA_TBOOLEAN:
            printf("%s\n", lua_toboolean(L, i) ? "true" : "false");
            break;

        case LUA_TNIL:
            printf("nil\n");
            break;

        default:
            printf("%p\n", lua_topointer(L, i));
            break;
        }
    }
    printf("==================================================\n");
}

bool is_ok(lua_State* L, int result)
{
    if (result != LUA_OK)
    {
        const char* err_msg = lua_tostring(L, -1);
        fprintf(stderr, "%s\n", err_msg);
        return false;
    }

    return true;
}
