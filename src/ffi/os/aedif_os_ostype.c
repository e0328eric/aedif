#include "os_functions.h"

int aedif_os_ostype(lua_State* L)
{
#if defined(_WIN32)
    lua_pushstring(L, "windows");
#elif defined(__APPLE__)
    lua_pushstring(L, "macos");
#elif defined(__linux__)
    lua_pushstring(L, "linux");
#elif defined(__FreeBSD__)
    lua_pushstring(L, "freeBSD");
#else
    lua_pushstring(L, "");
#endif

    return 1;
}
