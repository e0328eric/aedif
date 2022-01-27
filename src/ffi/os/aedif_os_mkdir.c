#include "os_functions.h"

typedef struct PathIter
{
#ifdef _WIN32
    const wchar_t* start;
    const wchar_t* current;
#else
    const char* start;
    const char* current;
#endif
} PathIter;

int aedif_os_mkdir(lua_State* L)
{
#ifdef _WIN32
    const char* dirname_tmp = luaL_checkstring(L, 1);
    wchar_t* dirname = malloc(sizeof(wchar_t) * PATH_CAPACITY);

    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, dirname_tmp, -1,
                            dirname, PATH_CAPACITY - 1) == 0)
    {
        free(dirname);
        lua_pushstring(L, AEDIF_ERROR_PREFIX "cannot encode a directory name");
        return lua_error(L);
    }

    wchar_t buffer[PATH_CAPACITY];
    PathIter iter = {dirname, dirname};
    wchar_t* p_buf_iter = buffer;

    dirname[PATH_CAPACITY - 1] = L'\0';
    buffer[PATH_CAPACITY - 1] = L'\0';

    while (*iter.start != L'\0')
    {
        while (*iter.current != L'/' && *iter.current != L'\\' &&
               *iter.current != L'\0')
        {
            ++iter.current;
        }

        memcpy(p_buf_iter, iter.start,
               sizeof(wchar_t) * (size_t)(iter.current - iter.start));
        p_buf_iter += (size_t)(iter.current - iter.start);
        iter.start = iter.current++;
        *p_buf_iter = L'\0';

        if (CreateDirectoryW(buffer, NULL) == 0 &&
            GetLastError() == ERROR_PATH_NOT_FOUND)
        {
            free(dirname);
            lua_pushboolean(L, false);
            return 1;
        }
    }

    free(dirname);
#else
    const char* dirname = luaL_checkstring(L, 1);

    char buffer[PATH_CAPACITY];
    PathIter iter = {dirname, dirname};
    char* p_buf_iter = buffer;

    while (*iter.start != '\0')
    {
        while (*iter.current != '/' && *iter.current != '\0')
        {
            ++iter.current;
        }

        memcpy(p_buf_iter, iter.start,
               sizeof(char) * (size_t)(iter.current - iter.start));
        p_buf_iter += (size_t)(iter.current - iter.start);
        iter.start = iter.current++;
        *p_buf_iter = '\0';

        if (mkdir(buffer, 0755) != 0 && errno != EEXIST)
        {
            lua_pushboolean(L, false);
            return 1;
        }
    }
#endif

    lua_pushboolean(L, true);
    return 1;
}
