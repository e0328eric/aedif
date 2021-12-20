#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "conversion.h"
#include "dyn_string.h"
#include "err_print_syntax.h"
#include "lua_debug.h"
#include "predefined_vars.h"
#include "project_data.h"
#include "registered_funcs.h"

#define ASSERT(cond, msg)                                                      \
    if (!(cond))                                                               \
    {                                                                          \
        fprintf(stderr, "%s\n", (msg));                                        \
        __asm__("int $3");                                                     \
    }

#define ASSIGN_VAR(_from, _to)                                                 \
    lua_getglobal(L, _from);                                                   \
    lua_setglobal(L, _to)

#define CHECK_IS_WHITESPACE(_str) ((*_str) == ' ' || (*_str) == '\t')

#define RAISE_ERR(_fmt_msg, ...)                                               \
    do                                                                         \
    {                                                                          \
        freeInnerProjectData(&pdata);                                          \
        freeBuildData(&bdata);                                                 \
        if ((line_number = getLineNumber(L)) < 0)                              \
        {                                                                      \
            ASSERT(false, AEDIF_INTERNAL_ERR_PREFIX                            \
                   "cannot get the line number from the 'getLineNumber' "      \
                   "function.");                                               \
            return 0;                                                          \
        }                                                                      \
        got_typename = luaL_typename(L, 1);                                    \
        snprintf(err_buffer, sizeof(err_buffer), AEDIF_ERROR_PREFIX _fmt_msg,  \
                 __VA_ARGS__);                                                 \
        lua_pushstring(L, err_buffer);                                         \
        lua_error(L);                                                          \
        return 0;                                                              \
    } while (false)

#define RAISE_TYPE_MISMATCHED_ERR(_type_str)                                   \
    RAISE_ERR("Type mismatched at line %d. expected " _type_str ", got '%s'.", \
              line_number, got_typename)

#define RAISE_GIVEN_NUMBER_INVALID_ERR(_expc_nums, _got_num)                   \
    RAISE_ERR("Numbers '%s' are expected at line %d. Got %d instead.",         \
              _expc_nums, line_number, _got_num)

#define INIT_MEMBER(_N, _member, _size)                                        \
    do                                                                         \
    {                                                                          \
        switch (lua_type(L, _N))                                               \
        {                                                                      \
        case LUA_TNIL:                                                         \
            break;                                                             \
                                                                               \
        case LUA_TSTRING:                                                      \
            bdata._member = malloc(sizeof(const char*));                       \
            *bdata._member = lua_tostring(L, _N);                              \
            bdata._size = 1;                                                   \
            break;                                                             \
                                                                               \
        case LUA_TTABLE:                                                       \
            lua_len(L, _N);                                                    \
            bdata._size = (size_t)lua_tointeger(L, -1);                        \
            lua_pop(L, 1);                                                     \
            bdata._member = malloc(sizeof(const char*) * bdata._size);         \
                                                                               \
            for (size_t i = 0; i < bdata._size; ++i)                           \
            {                                                                  \
                lua_pushnumber(L, i + 1);                                      \
                lua_gettable(L, _N);                                           \
                bdata._member[i] = lua_tostring(L, -1);                        \
                lua_pop(L, 1);                                                 \
            }                                                                  \
            break;                                                             \
                                                                               \
        default:                                                               \
            RAISE_TYPE_MISMATCHED_ERR("'nil', 'string' or 'table");            \
            break;                                                             \
        }                                                                      \
    } while (false)

/***********************************/
/* Structure and Enum  Definitions */
/***********************************/
typedef enum BuildType
{
    BUILD_TYPE_BINARY = 0,
    BUILD_TYPE_STATIC,
    BUILD_TYPE_DYNAMIC,
} BuildType;

typedef struct BuildData
{
    const char* targetName;
    const char** srcs;
    size_t srcsSize;
    const char** libs;
    size_t libsSize;
    const char** libDirs;
    size_t libDirsSize;
    const char** includes;
    size_t includesSize;
    BuildType buildType;
} BuildData;

/**********************/
/* Function Signature */
/**********************/
void freeBuildData(BuildData* bdata);
const char* stdType2str(StdType st);
const char* optLevel2str(OptLevel ol);
const String* getObjName(const char* src_name, const char* target_name);

/********************************/
/* Main Function Implementation */
/********************************/
int lua_RestoreSettings(lua_State* L)
{
    ASSIGN_VAR(AEDIF_LANGUAGE, "LANGUAGE");
    ASSIGN_VAR(AEDIF_COMPILER, "COMPILER");
    ASSIGN_VAR(AEDIF_STD, "STD");
    ASSIGN_VAR(AEDIF_OPT_LEVEL, "OPT_LEVEL");
    ASSIGN_VAR(AEDIF_WARNINGS, "WARNINGS");
    ASSIGN_VAR(AEDIF_ERRORS, "ERRORS");
    ASSIGN_VAR(AEDIF_FLAGS, "FLAGS");

    return 0;
}

int lua_Execute(lua_State* L)
{
    if (lua_type(L, 1) == LUA_TSTRING)
    {
        const char* exec_str = lua_tostring(L, 1);
        system(exec_str);
    }
    return 0;
}

int lua_Compile(lua_State* L)
{
    // Import basic compiling settings
    char err_buffer[150];
    const char* pb_err_msg;
    const char* got_typename;
    int line_number;

    BuildData bdata;
    memset(&bdata, 0, sizeof(BuildData));

    ProjectData pdata = getProjectData(&pb_err_msg, L);
    if (pb_err_msg != NULL)
    {
        snprintf(err_buffer, sizeof(err_buffer), AEDIF_ERROR_PREFIX "%s\n",
                 pb_err_msg);
        freeInnerProjectData(&pdata);
        lua_pushstring(L, err_buffer);
        lua_error(L);
        return 0;
    }

    /*******************/
    /* Get target name */
    /*******************/
    if (lua_type(L, 1) != LUA_TSTRING)
    {
        RAISE_TYPE_MISMATCHED_ERR("'string'");
    }
    bdata.targetName = lua_tostring(L, 1);

    /************/
    /* Get srcs */
    /************/
    switch (lua_type(L, 2))
    {
    case LUA_TSTRING:
        bdata.srcs = malloc(sizeof(const char*));
        *bdata.srcs = lua_tostring(L, 2);
        bdata.srcsSize = 1;
        break;

    case LUA_TTABLE:
        lua_len(L, 2);
        bdata.srcsSize = (size_t)lua_tointeger(L, -1);
        lua_pop(L, 1);
        bdata.srcs = malloc(sizeof(const char*) * bdata.srcsSize);

        for (size_t i = 0; i < bdata.srcsSize; ++i)
        {
            lua_pushnumber(L, i + 1);
            lua_gettable(L, 2);
            bdata.srcs[i] = lua_tostring(L, -1);
            lua_pop(L, 1);
        }
        break;

    default:
        RAISE_TYPE_MISMATCHED_ERR("'string' or 'table'");
        break;
    }

    /******************************/
    /* Initializing other members */
    /******************************/
    INIT_MEMBER(3, libs, libsSize);
    INIT_MEMBER(4, libDirs, libDirsSize);
    INIT_MEMBER(5, includes, includesSize);

    /*********************/
    /* Get Building type */
    /*********************/
    switch (lua_type(L, 6))
    {
    case LUA_TNIL:
        bdata.buildType = BUILD_TYPE_BINARY;
        break;

    case LUA_TNUMBER:
        bdata.buildType = (BuildType)lua_tonumber(L, 6);
        if (bdata.buildType < 0 || bdata.buildType > 3)
        {
            RAISE_GIVEN_NUMBER_INVALID_ERR("0, 1, 2 or 3", bdata.buildType);
        }
        break;

    case LUA_TSTRING: {
        const char* build_type_str = lua_tostring(L, 6);
        if (strncmp(build_type_str, "static\0", 7) == 0 ||
            strncmp(build_type_str, "s\0", 2) == 0)
        {
            bdata.buildType = BUILD_TYPE_STATIC;
        }
        else if (strncmp(build_type_str, "dynamic\0", 8) == 0 ||
                 strncmp(build_type_str, "d\0", 2) == 0)
        {
            bdata.buildType = BUILD_TYPE_DYNAMIC;
        }
        else if (strncmp(build_type_str, "binary\0", 7) == 0 ||
                 strncmp(build_type_str, "bin\0", 4) == 0 ||
                 strncmp(build_type_str, "b\0", 2) == 0)
        {
            bdata.buildType = BUILD_TYPE_BINARY;
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
        RAISE_TYPE_MISMATCHED_ERR("'nil', 'number' or 'string'");
        break;
    }

    /******************************/
    /* Actual Build Entire System */
    /******************************/
    String* command_line = mkString("");
    String* premire = mkString("");
    String* library_target_filename = NULL;

    printf("\n    \x1b[1m\x1b[4mBuilding %s\x1b[0m\n", bdata.targetName);

    appendStr(command_line, "mkdir -p ./build/obj/");
    appendStr(command_line, bdata.targetName);
    printf(DYNS_FMT "\n", DYNS_ARG(command_line));
    system(getStr(command_line));

    concatString(premire, pdata.compiler);
    appendChar(premire, ' ');
    appendStr(premire, stdType2str(pdata.std));
    appendChar(premire, ' ');
    appendStr(premire, optLevel2str(pdata.optLevel));
    appendChar(premire, ' ');
    for (size_t i = 0; i < pdata.warningsSize; ++i)
    {
        appendStr(premire, "-W");
        concatString(premire, pdata.warnings[i]);
        appendChar(premire, ' ');
    }
    for (size_t i = 0; i < pdata.errorsSize; ++i)
    {
        appendStr(premire, "-Werror=");
        concatString(premire, pdata.errors[i]);
        appendChar(premire, ' ');
    }
    for (size_t i = 0; i < pdata.flagsSize; ++i)
    {
        concatString(premire, pdata.flags[i]);
        appendChar(premire, ' ');
    }

    clearEntireString(command_line);
    concatString(command_line, premire);
    for (size_t i = 0; i < bdata.includesSize; ++i)
    {
        appendStr(command_line, "-I");
        appendStr(command_line, bdata.includes[i]);
        appendChar(command_line, ' ');
    }
    if (bdata.buildType == BUILD_TYPE_DYNAMIC)
    {
        appendStr(command_line, "-fpic ");
    }

    // before concatinating src strings, make a container
    // that contains associates object files
    const String** obj_container = malloc(sizeof(String*) * bdata.srcsSize);

    size_t prev_location = getLen(command_line);
    for (size_t i = 0; i < bdata.srcsSize; ++i)
    {
        clearStringAfter(command_line, (ssize_t)prev_location);
        appendStr(command_line, "-c ");
        appendStr(command_line, bdata.srcs[i]);
        obj_container[i] = getObjName(bdata.srcs[i], bdata.targetName);
        appendStr(command_line, " -o ");
        concatString(command_line, obj_container[i]);
        printf(DYNS_FMT "\n", DYNS_ARG(command_line));
        system(getStr(command_line));
    }

    clearEntireString(command_line);

    switch (bdata.buildType)
    {
    case BUILD_TYPE_BINARY:
        concatString(command_line, premire);
        for (size_t i = 0; i < bdata.libDirsSize; ++i)
        {
            appendStr(command_line, "-L");
            appendStr(command_line, bdata.libDirs[i]);
            appendChar(command_line, ' ');
        }
        for (size_t i = 0; i < bdata.libsSize; ++i)
        {
            appendStr(command_line, "-l");
            appendStr(command_line, bdata.libs[i]);
            appendChar(command_line, ' ');
        }
        for (size_t i = 0; i < bdata.srcsSize; ++i)
        {
            concatString(command_line, obj_container[i]);
            appendChar(command_line, ' ');
        }
        appendStr(command_line, "-o ");
        appendStr(command_line, bdata.targetName);

        printf(DYNS_FMT "\n", DYNS_ARG(command_line));
        system(getStr(command_line));

        clearEntireString(command_line);
        appendStr(command_line, "mv ");
        appendStr(command_line, bdata.targetName);
        appendStr(command_line, " ./build/bin");
        printf(DYNS_FMT "\n", DYNS_ARG(command_line));
        system(getStr(command_line));
        break;

    case BUILD_TYPE_STATIC: {
        library_target_filename = mkString("lib");
        appendStr(library_target_filename, bdata.targetName);
        appendStr(library_target_filename, ".a");

        appendStr(command_line, "ar rcu ");
        concatString(command_line, library_target_filename);
        appendChar(command_line, ' ');
        for (size_t i = 0; i < bdata.srcsSize; ++i)
        {
            concatString(command_line, obj_container[i]);
            appendChar(command_line, ' ');
        }
        printf(DYNS_FMT "\n", DYNS_ARG(command_line));
        system(getStr(command_line));

        clearEntireString(command_line);
        appendStr(command_line, "mv ");
        concatString(command_line, library_target_filename);
        appendStr(command_line, " ./build/lib");

        printf(DYNS_FMT "\n", DYNS_ARG(command_line));
        system(getStr(command_line));

        freeString(library_target_filename);
        break;
    }

    case BUILD_TYPE_DYNAMIC: {
        library_target_filename = mkString("lib");
        appendStr(library_target_filename, bdata.targetName);
        appendStr(library_target_filename, ".so");

        concatString(command_line, premire);
        for (size_t i = 0; i < bdata.libDirsSize; ++i)
        {
            appendStr(command_line, "-L");
            appendStr(command_line, bdata.libDirs[i]);
            appendChar(command_line, ' ');
        }
        for (size_t i = 0; i < bdata.libsSize; ++i)
        {
            appendStr(command_line, "-l");
            appendStr(command_line, bdata.libs[i]);
            appendChar(command_line, ' ');
        }
        for (size_t i = 0; i < bdata.srcsSize; ++i)
        {
            concatString(command_line, obj_container[i]);
            appendChar(command_line, ' ');
        }
        appendStr(command_line, "-o ");
        concatString(command_line, library_target_filename);

        printf(DYNS_FMT "\n", DYNS_ARG(command_line));
        system(getStr(command_line));

        clearEntireString(command_line);
        appendStr(command_line, "mv ");
        concatString(command_line, library_target_filename);
        appendStr(command_line, " ./build/lib");

        printf(DYNS_FMT "\n", DYNS_ARG(command_line));
        system(getStr(command_line));

        freeString(library_target_filename);
        break;
    }

    default:
        for (size_t i = 0; i < bdata.srcsSize; ++i)
        {
            freeString((String*)obj_container[i]);
        }
        free(obj_container);
        freeString(premire);
        freeString(command_line);
        freeBuildData(&bdata);
        freeInnerProjectData(&pdata);
        ASSERT(false, AEDIF_INTERNAL_ERR_PREFIX "Unreatchable (lua_Compile)");
        return 0;
    }

    /*************************/
    /* Deallocating memories */
    /*************************/
    for (size_t i = 0; i < bdata.srcsSize; ++i)
    {
        freeString((String*)obj_container[i]);
    }
    free(obj_container);
    freeString(premire);
    freeString(command_line);
    freeBuildData(&bdata);
    freeInnerProjectData(&pdata);
    return 0;
}

void freeBuildData(BuildData* bdata)
{
    free(bdata->srcs);
    free(bdata->libs);
    free(bdata->libDirs);
    free(bdata->includes);
}

const char* stdType2str(StdType st)
{
    switch (st)
    {
    case STD_TYPE_C_99:
        return "-std=c99";
    case STD_TYPE_C_11:
        return "-std=c11";
    case STD_TYPE_C_14:
        return "-std=c14";
    case STD_TYPE_C_17:
        return "-std=c17";
    case STD_TYPE_C_23:
        return "-std=c23";
    case STD_TYPE_CPP_11:
        return "-std=c++11";
    case STD_TYPE_CPP_14:
        return "-std=c++14";
    case STD_TYPE_CPP_17:
        return "-std=c++17";
    case STD_TYPE_CPP_20:
        return "-std=c++20";
    case STD_TYPE_CPP_23:
        return "-std=c++23";
    default:
        return "";
    }
}

const char* optLevel2str(OptLevel ol)
{
    switch (ol)
    {
    case OPT_LEVEL_1:
        return "-O1";
    case OPT_LEVEL_2:
        return "-O2";
    case OPT_LEVEL_3:
        return "-O3";
    case OPT_LEVEL_SIZE:
        return "-Os";
    default:
        return "";
    }
}

const String* getObjName(const char* src_name, const char* target_name)
{
    String* output = mkString(src_name);

    ssize_t pos = findCharReverse(output, '/');
    if (pos < 0)
    {
        return NULL;
    }
    clearStringBefore(output, pos);

    pos = findCharReverse(output, '.');
    if (pos < 0)
    {
        return NULL;
    }
    clearStringAfter(output, pos);

    appendStrBack(output, target_name);
    appendStrBack(output, "./build/obj/");
    appendStr(output, ".o");

    return output;
}
