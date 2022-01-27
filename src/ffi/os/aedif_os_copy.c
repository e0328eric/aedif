#include "os_functions.h"

#ifndef _WIN32
#define CHECK_IS_SUCESSED(_bool)                                               \
    if (!(_bool))                                                              \
    {                                                                          \
        lua_pushstring(L, AEDIF_ERROR_PREFIX);                                 \
        luaL_where(L, 1);                                                      \
        lua_pushstring(L, " cannot copy a file `");                            \
        lua_pushstring(L, orig_filename);                                      \
        lua_pushstring(L, "` into `");                                         \
        lua_pushstring(L, new_filename);                                       \
        lua_pushstring(L, "`\n" AEDIF_PADDING_PREFIX);                         \
        lua_pushstring(L, strerror(errno));                                    \
        lua_concat(L, 8);                                                      \
        return lua_error(L);                                                   \
    }
#endif

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

    bool can_overwrite;
    switch (lua_type(L, 3))
    {
    case LUA_TNIL:
        can_overwrite = false;
        break;

    case LUA_TBOOLEAN:
        can_overwrite = lua_toboolean(L, 3);
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
    lua_pushboolean(L, CopyFileW(orig_filename, new_filename, !can_overwrite) !=
                           0);

    free(orig_filename);
    free(new_filename);
#else
    int orig_file = open(orig_filename, O_RDONLY);
    int new_file = open(new_filename, O_WRONLY | O_CREAT, 0664);

    CHECK_IS_SUCESSED(orig_file >= 0);
    CHECK_IS_SUCESSED(new_file >= 0);

    struct stat orig_file_stat;
    struct stat new_file_stat;
    fstat(new_file, &new_file_stat);

    if (can_overwrite || new_file_stat.st_size == 0)
    {
        char* buffer = malloc(orig_file_stat.st_size);
        read(orig_file, buffer, orig_file_stat.st_size);
        write(new_file, buffer, orig_file_stat.st_size);
        free(buffer);
        lua_pushboolean(L, true);
    }
    else
    {
        lua_pushstring(L, AEDIF_WARN_PREFIX);
        luaL_where(L, 1);
        lua_pushstring(L, " cannot overwrite `");
        lua_pushstring(L, new_filename);
        lua_pushstring(L, "`\n");
        lua_pushstring(
            L, AEDIF_PADDING_PREFIX
            "the user specified not to overwrite a destination file.");
        lua_concat(L, 6);
        printf("%s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
        lua_pushboolean(L, false);
    }

    CHECK_IS_SUCESSED(close(new_file) == 0);
    CHECK_IS_SUCESSED(close(orig_file) == 0);
#endif

    return 1;
}
