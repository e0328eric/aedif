#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#endif

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include "error/err_prefix.h"

#define PATH_CAPACITY 4096

int aedif_os_isfile(lua_State* L);
int aedif_os_isdir(lua_State* L);
int aedif_os_issym(lua_State* L);
int aedif_os_abspath(lua_State* L);
int aedif_os_mkdir(lua_State* L);
int aedif_os_copy(lua_State* L);
int aedif_os_rename(lua_State* L);
int aedif_os_rmfile(lua_State* L);
int aedif_os_rmdir(lua_State* L);
int aedif_os_ostype(lua_State* L);
