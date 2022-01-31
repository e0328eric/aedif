#include <stdio.h>
#include <stdlib.h>

#include <lauxlib.h>

#define DYN_STRING_IMPL
#include "print_error.h"

#define AEDIF_INTERNAL_ERR_PREFIX                                              \
    "\033[1m\033[97;41m[aedif internal error]\033[0m\n"
#define AEDIF_ERROR_PREFIX "\033[1m\033[91m[aedif error]\033[0m    "
#define AEDIF_WARN_PREFIX "\033[1m\033[93m[aedif warning]\033[0m  "
#define AEDIF_NOTE_PREFIX "\033[1m\033[96m[aedif note]\033[0m     "

String* get_error_str(lua_State* L, AedifErrKind errkind, const char* msg)
{
    String* concat_msg = NULL;

    if (L != NULL)
    {
        luaL_where(L, 1);
    }

    switch (errkind)
    {
    case AEDIF_ERROR:
        concat_msg = mkString(AEDIF_ERROR_PREFIX);
        if (L != NULL)
        {
            appendStr(concat_msg, luaL_checkstring(L, -1));
            appendChar(concat_msg, ' ');
        }
        appendStr(concat_msg, msg);
        break;

    case AEDIF_WARNING:
        concat_msg = mkString(AEDIF_WARN_PREFIX);
        if (L != NULL)
        {
            appendStr(concat_msg, luaL_checkstring(L, -1));
            appendChar(concat_msg, ' ');
        }
        appendStr(concat_msg, msg);
        break;

    case AEDIF_NOTE:
        concat_msg = mkString(AEDIF_NOTE_PREFIX);
        if (L != NULL)
        {
            appendStr(concat_msg, luaL_checkstring(L, -1));
            appendChar(concat_msg, ' ');
        }
        appendStr(concat_msg, msg);
        break;

    case AEDIF_INTERNAL_ERROR:
        concat_msg = mkString(AEDIF_INTERNAL_ERR_PREFIX);
        if (L != NULL)
        {
            appendStr(concat_msg, luaL_checkstring(L, -1));
            appendChar(concat_msg, ' ');
        }
        appendStr(concat_msg, msg);
        break;

    default:
        fprintf(stderr,
                AEDIF_INTERNAL_ERR_PREFIX "Invalid errkind was found\n");
        return NULL;
    }

    if (L != NULL)
    {
        lua_pop(L, 1);
    }
    return concat_msg;
}

void print_error(lua_State* L, AedifErrKind errkind, const char* msg)
{
    String* str = get_error_str(L, errkind, msg);
    fprintf(stderr, DYNS_FMT "\n", DYNS_ARG(str));
    freeString(str);
}
