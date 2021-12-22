#ifndef AEDIF_COMPILE_H_
#define AEDIF_COMPILE_H_

#include <lua.h>
#include <lauxlib.h>

#include <dynString.h>

// A lua function that is written in C at here
// This function will register in the lua vm
int lua_RestoreSettings(lua_State* L);
int lua_Execute(lua_State* L);
int lua_Compile(lua_State* L);

#endif // AEDIF_COMPILE_H_
