#include "os_functions.h"

int aedif_os_isdir(lua_State* L)
{
    const char* dirname = luaL_checkstring(L, 1);
    if (strlen(dirname) == 0)
    {
        lua_pushboolean(L, true);
        return 1;
    }

#ifdef _WIN32
    wchar_t buffer[PATH_CAPACITY];
    DWORD attr;

    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, dirname, -1, buffer,
                            PATH_CAPACITY - 1) == 0)
    {
        lua_pushstring(L, AEDIF_ERROR_PREFIX "cannot encode a dirname");
        return lua_error(L);
    }

    attr = GetFileAttributesW(buffer);
    if (attr != INVALID_FILE_ATTRIBUTES)
    {
        lua_pushboolean(L, (attr & FILE_ATTRIBUTE_DIRECTORY) != 0);
    }
    else
    {
        lua_pushnil(L);
    }
#else
    struct stat buffer;

    if (stat(dirname, &buffer) == 0)
    {
        lua_pushboolean(L, (buffer.st_mode & S_IFDIR) != 0);
    }
    else
    {
        lua_pushnil(L);
    }
#endif

    return 1;
}
