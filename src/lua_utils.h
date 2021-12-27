#ifndef AEDIF_LUA_DEBUG_H_
#define AEDIF_LUA_DEBUG_H_

#include <stdbool.h>

#include <lua.h>
#include <lauxlib.h>

int getLineNumber(lua_State* L);
void dumpStack(lua_State* L);
bool is_ok(lua_State* L, int result);

#endif // AEDIF_LUA_DEBUG_H_
