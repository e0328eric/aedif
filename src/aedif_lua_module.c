#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dynString.h>

#include "aedif_lua_module.h"
#include "conversion.h"
#include "err_print_syntax.h"
#include "lua_debug.h"
#include "predefined_vars.h"
#include "project_data.h"

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

// Since there is no way to input the build_dir inside lua_Compile,
// we made a global variable.
const char** build_dir = NULL;

// Same manner for is_clean
extern bool* is_clean;

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
static int lua_RestoreSettings(lua_State* L);
static int lua_Execute(lua_State* L);
static int lua_Compile(lua_State* L);
static void freeBuildData(BuildData* bdata);
static bool initMember(lua_State* L, int num, const char*** member,
                       size_t* size);
static const char* stdType2str(StdType st);
static const char* optLevel2str(OptLevel ol);
static const String* getObjName(const char* src_name, const char* target_name);
static const char* getCurrentOs(void);

/********************************/
/* Main Function Implementation */
/********************************/
void linkAedifModule(lua_State* L)
{
    lua_pushglobaltable(L);
    lua_setglobal(L, "aedif");

    lua_getglobal(L, "aedif");

    lua_pushstring(L, "compile");
    lua_pushcfunction(L, lua_Compile);
    lua_settable(L, -3);

    lua_pushstring(L, "restoreSettings");
    lua_pushcfunction(L, lua_RestoreSettings);
    lua_settable(L, -3);

    lua_pushstring(L, "execute");
    lua_pushcfunction(L, lua_Execute);
    lua_settable(L, -3);

    lua_pushstring(L, "ostype");
    lua_pushstring(L, getCurrentOs());
    lua_settable(L, -3);

	lua_pushstring(L, "isclean");
	if (*is_clean)
	{
		lua_pushboolean(L, true);
	}
	else
	{
		lua_pushboolean(L, false);
	}
	lua_settable(L, -3);

    lua_pop(L, 1);
}

static int lua_RestoreSettings(lua_State* L)
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

static int lua_Execute(lua_State* L)
{
    if (lua_type(L, 1) == LUA_TSTRING)
    {
        const char* exec_str = lua_tostring(L, 1);
        system(exec_str);
    }
    return 0;
}

static int lua_Compile(lua_State* L)
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
            lua_pushinteger(L, (lua_Integer)(i + 1));
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
    if (!initMember(L, 3, &bdata.libs, &bdata.libsSize))
    {
        RAISE_TYPE_MISMATCHED_ERR("'nil', 'string' or 'table");
    }
    if (!initMember(L, 4, &bdata.libDirs, &bdata.libDirsSize))
    {
        RAISE_TYPE_MISMATCHED_ERR("'nil', 'string' or 'table");
    }
    if (!initMember(L, 5, &bdata.includes, &bdata.includesSize))
    {
        RAISE_TYPE_MISMATCHED_ERR("'nil', 'string' or 'table");
    }

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
    String* cmdline = mkString("");
    String* premire = mkString("");
    String* library_target_filename = NULL;

    printf("\n    \x1b[1m\x1b[4mBuilding %s\x1b[0m\n", bdata.targetName);

    appendStr(cmdline, "mkdir -p ");
    appendStr(cmdline, *build_dir);
    appendStr(cmdline, "/obj/");
    appendStr(cmdline, bdata.targetName);
    printf(DYNS_FMT "\n", DYNS_ARG(cmdline));
    system(getStr(cmdline));

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

    clearEntireString(cmdline);
    concatString(cmdline, premire);
    for (size_t i = 0; i < bdata.includesSize; ++i)
    {
        appendStr(cmdline, "-I");
        appendStr(cmdline, bdata.includes[i]);
        appendChar(cmdline, ' ');
    }
    if (bdata.buildType == BUILD_TYPE_DYNAMIC)
    {
        appendStr(cmdline, "-fpic ");
    }

    // before concatinating src strings, make a container
    // that contains associates object files
    const String** obj_container = malloc(sizeof(String*) * bdata.srcsSize);

    size_t prev_location = getLen(cmdline);
    for (size_t i = 0; i < bdata.srcsSize; ++i)
    {
        clearStringAfter(cmdline, (ssize_t)prev_location);
        appendStr(cmdline, "-c ");
        appendStr(cmdline, bdata.srcs[i]);
        obj_container[i] = getObjName(bdata.srcs[i], bdata.targetName);
        appendStr(cmdline, " -o ");
        concatString(cmdline, obj_container[i]);
        printf(DYNS_FMT "\n", DYNS_ARG(cmdline));
        system(getStr(cmdline));
    }

    clearEntireString(cmdline);

    switch (bdata.buildType)
    {
    case BUILD_TYPE_BINARY:
	concatString(cmdline, pdata.compiler);
	appendStr(cmdline, " -o ");
	appendStr(cmdline, bdata.targetName);
	appendChar(cmdline, ' ');
	
        for (size_t i = 0; i < bdata.srcsSize; ++i)
        {
            concatString(cmdline, obj_container[i]);
            appendChar(cmdline, ' ');
        }

	for (size_t i = 0; i < bdata.libDirsSize; ++i)
        {
            appendStr(cmdline, "-L");
            appendStr(cmdline, bdata.libDirs[i]);
            appendChar(cmdline, ' ');
        }
        for (size_t i = 0; i < bdata.libsSize; ++i)
        {
            appendStr(cmdline, "-l");
            appendStr(cmdline, bdata.libs[i]);
            appendChar(cmdline, ' ');
        }

        printf(DYNS_FMT "\n", DYNS_ARG(cmdline));
        system(getStr(cmdline));

        clearEntireString(cmdline);
        appendStr(cmdline, "mv ");
        appendStr(cmdline, bdata.targetName);
        appendChar(cmdline, ' ');
        appendStr(cmdline, *build_dir);
        appendStr(cmdline, "/bin");
        printf(DYNS_FMT "\n", DYNS_ARG(cmdline));
        system(getStr(cmdline));
        break;

    case BUILD_TYPE_STATIC: {
        library_target_filename = mkString("lib");
        appendStr(library_target_filename, bdata.targetName);
        appendStr(library_target_filename, ".a");

        appendStr(cmdline, "ar rcu ");
        concatString(cmdline, library_target_filename);
        appendChar(cmdline, ' ');
        for (size_t i = 0; i < bdata.srcsSize; ++i)
        {
            concatString(cmdline, obj_container[i]);
            appendChar(cmdline, ' ');
        }
        printf(DYNS_FMT "\n", DYNS_ARG(cmdline));
        system(getStr(cmdline));

        clearEntireString(cmdline);
        appendStr(cmdline, "mv ");
        concatString(cmdline, library_target_filename);
        appendChar(cmdline, ' ');
        appendStr(cmdline, *build_dir);
        appendStr(cmdline, "/lib");

        printf(DYNS_FMT "\n", DYNS_ARG(cmdline));
        system(getStr(cmdline));

        freeString(library_target_filename);
        break;
    }

    case BUILD_TYPE_DYNAMIC: {
        library_target_filename = mkString("lib");
        appendStr(library_target_filename, bdata.targetName);
        appendStr(library_target_filename, ".so");

        concatString(cmdline, premire);
        for (size_t i = 0; i < bdata.libDirsSize; ++i)
        {
            appendStr(cmdline, "-L");
            appendStr(cmdline, bdata.libDirs[i]);
            appendChar(cmdline, ' ');
        }
        for (size_t i = 0; i < bdata.libsSize; ++i)
        {
            appendStr(cmdline, "-l");
            appendStr(cmdline, bdata.libs[i]);
            appendChar(cmdline, ' ');
        }
        for (size_t i = 0; i < bdata.srcsSize; ++i)
        {
            concatString(cmdline, obj_container[i]);
            appendChar(cmdline, ' ');
        }
        appendStr(cmdline, "-o ");
        concatString(cmdline, library_target_filename);

        printf(DYNS_FMT "\n", DYNS_ARG(cmdline));
        system(getStr(cmdline));

        clearEntireString(cmdline);
        appendStr(cmdline, "mv ");
        concatString(cmdline, library_target_filename);
        appendChar(cmdline, ' ');
        appendStr(cmdline, *build_dir);
        appendStr(cmdline, "/lib");

        printf(DYNS_FMT "\n", DYNS_ARG(cmdline));
        system(getStr(cmdline));

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
        freeString(cmdline);
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
    freeString(cmdline);
    freeBuildData(&bdata);
    freeInnerProjectData(&pdata);
    return 0;
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

static void freeBuildData(BuildData* bdata)
{
    free(bdata->srcs);
    free(bdata->libs);
    free(bdata->libDirs);
    free(bdata->includes);
}

static const char* stdType2str(StdType st)
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

static const char* optLevel2str(OptLevel ol)
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

static const String* getObjName(const char* src_name, const char* target_name)
{
    String* output = mkString(src_name);

    ssize_t pos = findCharReverse(output, '/');
    if (pos < 0)
    {
        pos = 0;
    }
    clearStringBefore(output, pos);

    pos = findCharReverse(output, '.');
    if (pos < 0)
    {
        return NULL;
    }
    clearStringAfter(output, pos);

    appendStrBack(output, target_name);
    appendStrBack(output, "/obj/");
    appendStrBack(output, *build_dir);
    appendStr(output, ".o");

    return output;
}

static const char* getCurrentOs(void)
{
#if defined(_WIN32) || defined(_WIN64)
    return "windows";
#elif defined(__linux__)
    return "linux";
#elif defined(__APPLE__)
    return "macos";
#elif defined(__FreeBSD__)
    return "freeBSD";
#else
    return "";
#endif
}
