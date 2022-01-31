#ifndef AEDIF_ERR_PRINT_SYNTAX_H_
#define AEDIF_ERR_PRINT_SYNTAX_H_

#include <lua.h>

#include <dynString.h>

typedef enum
{
    AEDIF_ERROR = 0,
    AEDIF_WARNING,
    AEDIF_NOTE,
    AEDIF_INTERNAL_ERROR,
    AEDIF_ERROR_KIND_COUNT,
} AedifErrKind;

String* get_error_str(lua_State* L, AedifErrKind errkind, const char* msg);
void print_error(lua_State* L, AedifErrKind errkind, const char* msg);

#endif // AEDIF_ERR_PRINT_SYNTAX_H_
