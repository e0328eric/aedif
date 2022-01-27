#include "os_functions.h"

#ifndef _WIN32
#define CHECK_IS_SUCESSED(_bool)                                               \
    if (!(_bool))                                                              \
    {                                                                          \
        lua_pushstring(L, AEDIF_ERROR_PREFIX);                                 \
        luaL_where(L, 1);                                                      \
        lua_pushstring(L, " cannot remove a directory `");                     \
        lua_pushstring(L, dirname);                                            \
        lua_pushstring(L, "`\n" AEDIF_PADDING_PREFIX);                         \
        lua_pushstring(L, strerror(errno));                                    \
        lua_concat(L, 6);                                                      \
        return lua_error(L);                                                   \
    }
#endif

int aedif_os_rmdir(lua_State* L)
{
    const char* dirname = luaL_checkstring(L, 1);

#ifdef _WIN32
    wchar_t buffer[PATH_CAPACITY];

    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, dirname, -1, buffer,
                            PATH_CAPACITY - 1) == 0)
    {
        free(buffer);
        lua_pushstring(L, AEDIF_ERROR_PREFIX "cannot encode a directory name");
        return lua_error(L);
    }

    buffer[PATH_CAPACITY - 1] = L'\0';

    lua_pushboolean(L, RemoveDirectoryW(buffer) != 0);
#else
    CHECK_IS_SUCESSED(rmdir(dirname) == 0 || errno == ENOENT);
    lua_pushboolean(L, true);
#endif

    return 1;
}
