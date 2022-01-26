#include "os_functions.h"

int aedif_os_issym(lua_State* L)
{
    const char* filename = luaL_checkstring(L, 1);

#ifdef _WIN32
#else
    struct stat buffer;

    if (stat(filename, &buffer) == 0)
    {
        lua_pushboolean(L, (buffer.st_mode & S_IFLNK) != 0);
    }
    else
    {
        lua_pushnil(L);
    }
#endif

    return 1;
}
