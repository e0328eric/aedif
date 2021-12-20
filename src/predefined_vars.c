#include <lauxlib.h>

#include "predefined_vars.h"

#define ASSIGN_VAR(_var)                                                       \
    lua_pushnil(L);                                                            \
    lua_setglobal(L, _var)

void predefineVars(lua_State* L)
{
    ASSIGN_VAR(AEDIF_LANGUAGE);
    ASSIGN_VAR(AEDIF_COMPILER);
    ASSIGN_VAR(AEDIF_STD);
    ASSIGN_VAR(AEDIF_OPT_LEVEL);
    ASSIGN_VAR(AEDIF_WARNINGS);
    ASSIGN_VAR(AEDIF_ERRORS);
	ASSIGN_VAR(AEDIF_FLAGS);
}

#undef ASSIGN_VAR
