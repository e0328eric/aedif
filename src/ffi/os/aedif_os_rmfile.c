#include "os_functions.h"

#ifndef _WIN32
#define CHECK_IS_SUCESSED(_bool)                                               \
    if (!(_bool))                                                              \
    {                                                                          \
        snprintf(msg, 250, "cannot remove a file `%s`\n%s\n", filename,        \
                 strerror(errno));                                             \
        String* err_msg = get_error_str(L, AEDIF_ERROR, msg);                  \
        lua_pushstring(L, getStr(err_msg));                                    \
        freeString(err_msg);                                                   \
        return lua_error(L);                                                   \
    }
#endif

int aedif_os_rmfile(lua_State* L)
{
    char msg[250];
    const char* filename = luaL_checkstring(L, 1);

#ifdef _WIN32
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
    if (unlink(filename) != 0)
    {
        switch (errno)
        {
        case ENOENT:
            snprintf(msg, 250, "The file `%s` is already removed\n", filename);
            print_error(L, AEDIF_NOTE, msg);
            lua_pushboolean(L, false);
            return 1;

        default:
            CHECK_IS_SUCESSED(false);
        }
    }
    lua_pushboolean(L, true);
#endif

    return 1;
}
