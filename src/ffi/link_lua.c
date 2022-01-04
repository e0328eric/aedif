#include <lauxlib.h>
#include <lualib.h>

#include "os/os_functions.h"

// clang-format off
static struct
{
    const char* name;
    lua_CFunction func;
} link_func_list[] = {
    {"isfile", aedif_os_isfile},
    {"isdir", aedif_os_isdir},
    /* {"issym", aedif_os_issym}, */
    /* {"abspath", aedif_os_abspath}, */
    /* {"mkdir", aedif_os_mkdir}, */
    /* {"copy", aedif_os_copy}, */
    /* {"rename", aedif_os_rename}, */
    /* {"remove", aedif_os_remove}, */
    /* {"ostype", aedif_os_ostype}, */
    {NULL, NULL},
    /* {"compile", aedif_compile}, */
    /* {"recompile", aedif_recompile}, */
    /* {"isclean", aedif_isclean}, */
    {NULL, NULL}, // END OF THE LIST
};
// clang-format on

static void makeLuaSublibrary(lua_State* L, const char* lib_name, size_t* idx)
{
    lua_pushglobaltable(L);
    lua_setglobal(L, lib_name);

    lua_pushstring(L, lib_name);
    lua_getglobal(L, lib_name);

    for (; link_func_list[*idx].name; ++*idx)
    {
        lua_pushstring(L, link_func_list[*idx].name);
        lua_pushcfunction(L, link_func_list[*idx].func);
        lua_settable(L, -3);
    }

    lua_settable(L, -3);
}

void makeAedifLuaLibrary(lua_State* L)
{
    size_t idx = 0;

    lua_pushglobaltable(L);
    lua_setglobal(L, "aedif");
    lua_getglobal(L, "aedif");

    makeLuaSublibrary(L, "os", &idx);

    // link main functions
    for (++idx; link_func_list[idx].name; ++idx)
    {
        lua_pushstring(L, link_func_list[idx].name);
        lua_pushcfunction(L, link_func_list[idx].func);
        lua_settable(L, -3);
    }

    lua_pop(L, 1);
}
