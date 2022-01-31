#include "os_functions.h"

#ifndef _WIN32
#define EXIT_WITH_ERROR                                                        \
    snprintf(msg, 250, "cannot remove a directory `%s`\n%s\n", dirname,        \
             strerror(errno));                                                 \
    String* err_msg = get_error_str(L, AEDIF_ERROR, msg);                      \
    lua_pushstring(L, getStr(err_msg));                                        \
    freeString(err_msg);                                                       \
    return lua_error(L)
#endif

#ifndef _WIN32
static bool rmdir_recursive(const char* dir_name);
#endif

int aedif_os_rmdir(lua_State* L)
{
    char msg[250];
    const char* dirname = luaL_checkstring(L, 1);

#ifdef _WIN32
    wchar_t buffer[PATH_CAPACITY];

    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, dirname, -1, buffer,
                            PATH_CAPACITY - 1) == 0)
    {
        free(buffer);
        err_msg =
            get_error_str(L, AEDIF_ERROR, "cannot encode a directory name");
        lua_pushstring(L, getStr(err_msg));
        freeString(err_msg);
        return lua_error(L);
    }

    buffer[PATH_CAPACITY - 1] = L'\0';

    lua_pushboolean(L, RemoveDirectoryW(buffer) != 0);
#else // END _WIN32

    bool remove_recursive;

    switch (lua_type(L, 2))
    {
    case LUA_TNIL:
        remove_recursive = false;
        break;

    case LUA_TBOOLEAN:
        remove_recursive = lua_toboolean(L, 2);
        break;

    default:
        snprintf(msg, 250,
                 "Type mismatched. Expected nil or boolean, got %s. So, do "
                 "nothing.\n",
                 lua_typename(L, 2));
        print_error(L, AEDIF_WARNING, msg);
        lua_pushboolean(L, false);
        return 1;
        break;
    }

    if (remove_recursive)
    {
        if (!rmdir_recursive(dirname))
        {
            switch (errno)
            {
            case ENOENT:
                snprintf(msg, 250, "The directory `%s` is already removed\n",
                         dirname);
                print_error(L, AEDIF_NOTE, msg);
                lua_pushboolean(L, false);
                return 1;

            default:
                EXIT_WITH_ERROR;
            }
        }
    }
    else
    {
        if (rmdir(dirname) != 0)
        {
            switch (errno)
            {
            case ENOTEMPTY:
                snprintf(msg, 250, "The directory `%s` is not empty\n",
                         dirname);
                print_error(L, AEDIF_WARNING, msg);
                lua_pushboolean(L, false);
                return 1;

            case ENOENT:
                snprintf(msg, 250, "The directory `%s` is already removed\n",
                         dirname);
                print_error(L, AEDIF_NOTE, msg);
                lua_pushboolean(L, false);
                return 1;

            default:
                EXIT_WITH_ERROR;
            }
        }
    }
    lua_pushboolean(L, true);
#endif

    return 1;
}

#ifndef _WIN32
bool rmdir_recursive(const char* dir_name)
{
    size_t dirname_len = strlen(dir_name);
    DIR* dirp = opendir(dir_name);
    struct dirent* dp = NULL;
    char* file_dir = NULL;
    struct stat st;
    size_t filename_len;

    if (dirp == NULL)
    {
        goto HANDLE_ERROR;
    }

    while ((dp = readdir(dirp)) != NULL)
    {
        if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
        {
            continue;
        }

        filename_len = dirname_len + dp->d_namlen + 2;
        file_dir = (char*)malloc(filename_len);
        snprintf(file_dir, filename_len, "%s/%s", dir_name, dp->d_name);

        if (stat(file_dir, &st) < 0)
        {
            goto HANDLE_ERROR;
        }
        if ((st.st_mode & S_IFDIR) != 0)
        {
            if (!rmdir_recursive(file_dir))
            {
                goto HANDLE_ERROR;
            }
        }
        else
        {
            if (unlink(file_dir) < 0)
            {
                goto HANDLE_ERROR;
            }
        }

        free(file_dir);
    }

    closedir(dirp);
    if (rmdir(dir_name) < 0)
    {
        goto HANDLE_ERROR;
    }

    return true;

HANDLE_ERROR:
    free(file_dir);
    if (dirp != NULL)
    {
        closedir(dirp);
    }
    return false;
}
#endif
