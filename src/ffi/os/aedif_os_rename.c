#include "os_functions.h"

int aedif_os_rename(lua_State* L)
{
#ifdef _WIN32
    const char* orig_tmp = luaL_checkstring(L, 1);
    const char* new_tmp = luaL_checkstring(L, 2);

    wchar_t* orig_file = malloc(sizeof(wchar_t) * PATH_CAPACITY);
    wchar_t* new_file = malloc(sizeof(wchar_t) * PATH_CAPACITY);

    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, orig_tmp, -1,
                            orig_file, PATH_CAPACITY - 1) == 0 ||
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, new_tmp, -1,
                            new_file, PATH_CAPACITY - 1) == 0)
    {
        free(orig_file);
        free(new_file);
        lua_pushstring(L, AEDIF_ERROR_PREFIX "cannot encode a directory name");
        return lua_error(L);
    }

    orig_file[PATH_CAPACITY - 1] = L'\0';
    new_file[PATH_CAPACITY - 1] = L'\0';

    lua_pushboolean(L, MoveFileW(orig_file, new_file) != 0);

    free(orig_file);
    free(new_file);
#else
#endif

    return 1;
}
