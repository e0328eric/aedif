#include "os_functions.h"

// It takes two or three values. First two parameters are file or directory
// location. Third value sets whether overriding an existing file or directory
// possible. True makes it valid, false or nil makes not.
int aedif_os_copy(lua_State* L)
{
#ifdef _WIN32
    const char* orig_tmp = luaL_checkstring(L, 1);
    const char* new_tmp = luaL_checkstring(L, 2);

    const wchar_t* orig_filename = malloc(sizeof(wchar_t) * PATH_CAPACITY);
    const wchar_t* new_filename = malloc(sizeof(wchar_t) * PATH_CAPACITY);

    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, orig_tmp, -1,
                            orig_filename, PATH_CAPACITY - 1) == 0 ||
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, new_tmp, -1,
                            new_filename, PATH_CAPACITY - 1) == 0)
    {
        free(orig_filename);
        free(new_filename);
        lua_pushstring(L, AEDIF_ERROR_PREFIX "cannot encode a directory name");
        return lua_error(L);
    }

    orig_filename[PATH_CAPACITY - 1] = L'\0';
    new_filename[PATH_CAPACITY - 1] = L'\0';
#else
    const char* orig_filename = luaL_checkstring(L, 1);
    const char* new_filename = luaL_checkstring(L, 2);
#endif

    bool do_not_override;
    switch (lua_type(L, 3))
    {
    case LUA_TNIL:
        do_not_override = true;
        break;

    case LUA_TBOOLEAN:
        do_not_override = !lua_toboolean(L, 3);
        break;

    // TODO: better error message
    default:
        lua_pushstring(L, AEDIF_WARN_PREFIX);
        luaL_where(L, 1);
        lua_pushstring(L, " type mismatched.\n" AEDIF_PADDING_PREFIX
                          "expected nil or boolean, got ");
        lua_pushstring(L, lua_typename(L, 3));
        lua_pushstring(L, ".");
        lua_concat(L, 5);
        printf("%s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
        lua_pushboolean(L, false);
        return 1;
        break;
    }

#ifdef _WIN32
    lua_pushboolean(
        L, CopyFileW(orig_filename, new_filename, do_not_override) != 0);

    free(orig_filename);
    free(new_filename);
#else
#endif

    return 1;
}
