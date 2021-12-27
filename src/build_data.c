#include <stdlib.h>
#include <string.h>

#include "build_data.h"
#include "err_print_syntax.h"
#include "lua_utils.h"

/*****************/
/* Define Macros */
/*****************/
#define ASSERT(cond, msg)                                                      \
    if (!(cond))                                                               \
    {                                                                          \
        fprintf(stderr, "%s\n", (msg));                                        \
        __asm__("int $3");                                                     \
    }

/*****************************/
/* Static Function Signature */
/*****************************/
static bool initMember(lua_State* L, int num, const char*** member,
                       size_t* size);
static void raiseTypeMismatchedErr(lua_State* L, const char** err_msg,
                                   const char* msg);
static void raiseInvalidNumberErr(lua_State* L, const char** err_msg,
                                  const char* expected, int got);

/*********************************/
/* Main Function Implementations */
/*********************************/
BuildData newBuildData(lua_State* L, const char** err_msg)
{
    BuildData output;
    memset(&output, 0, sizeof(BuildData));

    /*******************/
    /* Get target name */
    /*******************/
    if (lua_type(L, 1) != LUA_TSTRING)
    {
        raiseTypeMismatchedErr(L, err_msg, "'string'");
        return output;
    }
    output.targetName = lua_tostring(L, 1);

    /************/
    /* Get srcs */
    /************/
    switch (lua_type(L, 2))
    {
    case LUA_TSTRING:
        output.srcs = malloc(sizeof(const char*));
        *output.srcs = lua_tostring(L, 2);
        output.srcsSize = 1;
        break;

    case LUA_TTABLE:
        lua_len(L, 2);
        output.srcsSize = (size_t)lua_tointeger(L, -1);
        lua_pop(L, 1);
        output.srcs = malloc(sizeof(const char*) * output.srcsSize);

        for (size_t i = 0; i < output.srcsSize; ++i)
        {
            lua_pushinteger(L, (lua_Integer)(i + 1));
            lua_gettable(L, 2);
            output.srcs[i] = lua_tostring(L, -1);
            lua_pop(L, 1);
        }
        break;

    default:
        raiseTypeMismatchedErr(L, err_msg, "'string' or 'table'");
        return output;
    }

    /******************************/
    /* Initializing other members */
    /******************************/
    if (!initMember(L, 3, &output.libs, &output.libsSize))
    {
        raiseTypeMismatchedErr(L, err_msg, "'nil', 'string' or 'table'");
        return output;
    }
    if (!initMember(L, 4, &output.libDirs, &output.libDirsSize))
    {
        raiseTypeMismatchedErr(L, err_msg, "'nil', 'string' or 'table'");
        return output;
    }
    if (!initMember(L, 5, &output.includes, &output.includesSize))
    {
        raiseTypeMismatchedErr(L, err_msg, "'nil', 'string' or 'table'");
        return output;
    }

    /*********************/
    /* Get Building type */
    /*********************/
    switch (lua_type(L, 6))
    {
    case LUA_TNIL:
        output.buildType = BUILD_TYPE_BINARY;
        break;

    case LUA_TNUMBER:
        output.buildType = (BuildType)lua_tonumber(L, 6);
        if (output.buildType < 0 || output.buildType > 3)
        {
            raiseInvalidNumberErr(L, err_msg, "0, 1, 2 or 3",
                                  (int)output.buildType);
            return output;
        }
        break;

    case LUA_TSTRING: {
        const char* build_type_str = lua_tostring(L, 6);
        if (strncmp(build_type_str, "static\0", 7) == 0 ||
            strncmp(build_type_str, "s\0", 2) == 0)
        {
            output.buildType = BUILD_TYPE_STATIC;
        }
        else if (strncmp(build_type_str, "dynamic\0", 8) == 0 ||
                 strncmp(build_type_str, "d\0", 2) == 0)
        {
            output.buildType = BUILD_TYPE_DYNAMIC;
        }
        else if (strncmp(build_type_str, "binary\0", 7) == 0 ||
                 strncmp(build_type_str, "bin\0", 4) == 0 ||
                 strncmp(build_type_str, "b\0", 2) == 0)
        {
            output.buildType = BUILD_TYPE_BINARY;
        }
        else
        {
            fprintf(stderr, AEDIF_WARN_PREFIX
                    "Invalid build type "
                    "string. Compiling it with the type 'binary', "
                    "instead.\n" AEDIF_NOTE_PREFIX "The possible build types "
                    "are 'static', 'dynamic' or 'binary'.\n");
        }
        break;
    }

    default:
        raiseTypeMismatchedErr(L, err_msg, "'nil', 'number' or 'string'");
        break;
    }

    *err_msg = NULL;
    return output;
}

void freeInnerBuildData(BuildData* bdata)
{
    free(bdata->srcs);
    free(bdata->libs);
    free(bdata->libDirs);
    free(bdata->includes);
}

static bool initMember(lua_State* L, int num, const char*** member,
                       size_t* size)
{
    switch (lua_type(L, num))
    {
    case LUA_TNIL:
        break;

    case LUA_TSTRING:
        *member = malloc(sizeof(const char*));
        **member = lua_tostring(L, num);
        *size = 1;
        break;

    case LUA_TTABLE:
        lua_len(L, num);
        *size = (size_t)lua_tointeger(L, -1);
        lua_pop(L, 1);
        *member = malloc(sizeof(const char*) * *size);

        for (size_t i = 0; i < *size; ++i)
        {
            lua_pushinteger(L, (lua_Integer)(i + 1));
            lua_gettable(L, num);
            (*member)[i] = lua_tostring(L, -1);
            lua_pop(L, 1);
        }
        break;

    default:
        return false;
        break;
    }

    return true;
}

static void raiseTypeMismatchedErr(lua_State* L, const char** err_msg,
                                   const char* msg)
{
    char err_buffer[150];
    int line_number;
    const char* got_typename;

    memset(err_buffer, 0, sizeof(err_buffer));

    if ((line_number = getLineNumber(L)) < 0)
    {
        ASSERT(false, AEDIF_INTERNAL_ERR_PREFIX
               "cannot get the line number from the 'getLineNumber' "
               "function.");
        return;
    }

    got_typename = luaL_typename(L, -1);
    snprintf(err_buffer, sizeof(err_buffer),
             AEDIF_ERROR_PREFIX
             "Type mismatched at line %d. expected %s, got '%s'.",
             line_number, msg, got_typename);

    char* tmp = malloc(sizeof(err_buffer));
    memcpy(tmp, err_buffer, sizeof(err_buffer));
    *err_msg = tmp;
}

static void raiseInvalidNumberErr(lua_State* L, const char** err_msg,
                                  const char* expected, int got)
{
    char err_buffer[200];
    int line_number;

    memset(err_buffer, 0, sizeof(err_buffer));

    if ((line_number = getLineNumber(L)) < 0)
    {
        ASSERT(false, AEDIF_INTERNAL_ERR_PREFIX
               "cannot get the line number from the 'getLineNumber' "
               "function.");
        return;
    }

    snprintf(err_buffer, sizeof(err_buffer),
             AEDIF_ERROR_PREFIX
             "Numbers '%s' are expected at line %d. Got %d instead.",
             expected, line_number, got);

    char* tmp = malloc(sizeof(err_buffer));
    memcpy(tmp, err_buffer, sizeof(err_buffer));
    *err_msg = tmp;
}
