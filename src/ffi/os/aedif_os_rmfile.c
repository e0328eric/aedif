#include "os_functions.h"

int aedif_os_rmfile(lua_State* L)
{
#ifdef _WIN32
    const char* filename = luaL_checkstring(L, 1);
    wchar_t buffer[PATH_CAPACITY];

    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, filename, -1, buffer,
                            PATH_CAPACITY - 1) == 0)
    {
        free(buffer);
        lua_pushstring(L, AEDIF_ERROR_PREFIX "cannot encode a directory name");
        return lua_error(L);
    }

    buffer[PATH_CAPACITY - 1] = L'\0';

    lua_pushboolean(L, DeleteFileW(buffer) != 0);
#else
#endif

    return 1;
}
