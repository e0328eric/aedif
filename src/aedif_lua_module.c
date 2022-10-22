#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dynString.h>

#include "aedif_lua_module.h"
#include "build_data.h"
#include "conversion.h"
#include "err_print_syntax.h"
#include "lua_utils.h"
#include "predefined_vars.h"
#include "project_data.h"

/*****************/
/* Define Macros */
/*****************/
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

////////////////////////////////////////////////////////////////////////////////

/********************/
/* Global Variables */
/********************/
// Since there is no way to input the BUILD_DIR inside lua_Compile,
// we made a global variable.
const char** BUILD_DIR = NULL;

// same for target name (heap allocated)
char* TARGET_NAME = NULL;

// Same manner for IS_CLEAN
extern bool* IS_CLEAN;

/**********************/
/* Function Signature */
/**********************/
static int lua_RestoreSettings(lua_State* L);
static int lua_Execute(lua_State* L);
static int lua_Compile(lua_State* L);

static void build_binary(String* cmdline, const String** objs,
                         const ProjectData* pdata, const BuildData* bdata);
static void build_static_lib(String* cmdline, const String** objs,
                             const BuildData* bdata);
static void build_dynamic_lib(String* cmdline, const String* premire,
                              const String** objs, const BuildData* bdata);

static const String* getFlags(const String** inner, size_t len);

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
    if (*IS_CLEAN)
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
    const char* bd_err_msg;

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

    BuildData bdata = newBuildData(L, &bd_err_msg);
    if (bd_err_msg != NULL)
    {
        snprintf(err_buffer, sizeof(err_buffer), AEDIF_ERROR_PREFIX "%s\n",
                 bd_err_msg);
        freeInnerProjectData(&pdata);
        freeInnerBuildData(&bdata);
        lua_pushstring(L, err_buffer);
        lua_error(L);
        return 0;
    }

    // copy the target name
    size_t targetLen = strlen(bdata.targetName);
    TARGET_NAME = malloc(targetLen + 1);
    memcpy(TARGET_NAME, bdata.targetName, targetLen);
    TARGET_NAME[targetLen] = '\0';

    /******************************/
    /* Actual Build Entire System */
    /******************************/
    String* cmdline = mkString(NULL);
    String* premire = mkString(NULL);

    printf("\n    \x1b[1m\x1b[4mBuilding %s\x1b[0m\n", bdata.targetName);

    appendStr(cmdline, "mkdir -p ");
    appendStr(cmdline, *BUILD_DIR);
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

    concatFreeString(
        premire, (String*)getFlags(pdata.compileFlags, pdata.compileFlagsSize));
    concatFreeString(premire, (String*)getFlags(pdata.flags, pdata.flagsSize));

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
        build_binary(cmdline, obj_container, &pdata, &bdata);
        break;

    case BUILD_TYPE_STATIC: {
        build_static_lib(cmdline, obj_container, &bdata);
        break;
    }

    case BUILD_TYPE_DYNAMIC: {
        build_dynamic_lib(cmdline, premire, obj_container, &bdata);
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
        freeInnerBuildData(&bdata);
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
    freeInnerBuildData(&bdata);
    freeInnerProjectData(&pdata);
    return 0;
}

static void build_binary(String* cmdline, const String** objs,
                         const ProjectData* pdata, const BuildData* bdata)
{
    concatString(cmdline, pdata->compiler);
    appendChar(cmdline, ' ');
    concatFreeString(cmdline,
                     (String*)getFlags(pdata->linkFlags, pdata->linkFlagsSize));
    concatFreeString(cmdline,
                     (String*)getFlags(pdata->flags, pdata->flagsSize));
    appendStr(cmdline, "-o ");
    appendStr(cmdline, bdata->targetName);
    appendChar(cmdline, ' ');

    for (size_t i = 0; i < bdata->srcsSize; ++i)
    {
        concatString(cmdline, objs[i]);
        appendChar(cmdline, ' ');
    }

    for (size_t i = 0; i < bdata->libDirsSize; ++i)
    {
        appendStr(cmdline, "-L");
        appendStr(cmdline, bdata->libDirs[i]);
        appendChar(cmdline, ' ');
    }
    for (size_t i = 0; i < bdata->libsSize; ++i)
    {
        appendStr(cmdline, "-l");
        appendStr(cmdline, bdata->libs[i]);
        appendChar(cmdline, ' ');
    }

    printf(DYNS_FMT "\n", DYNS_ARG(cmdline));
    system(getStr(cmdline));

    clearEntireString(cmdline);
    appendStr(cmdline, "mv ");
    appendStr(cmdline, bdata->targetName);
    appendChar(cmdline, ' ');
    appendStr(cmdline, *BUILD_DIR);
    appendStr(cmdline, "/bin");
    printf(DYNS_FMT "\n", DYNS_ARG(cmdline));
    system(getStr(cmdline));
}

static void build_static_lib(String* cmdline, const String** objs,
                             const BuildData* bdata)
{
    String* library_target_filename = mkString("lib");
    appendStr(library_target_filename, bdata->targetName);
    appendStr(library_target_filename, ".a");

    appendStr(cmdline, "ar rcu ");
    concatString(cmdline, library_target_filename);
    appendChar(cmdline, ' ');
    for (size_t i = 0; i < bdata->srcsSize; ++i)
    {
        concatString(cmdline, objs[i]);
        appendChar(cmdline, ' ');
    }
    printf(DYNS_FMT "\n", DYNS_ARG(cmdline));
    system(getStr(cmdline));

    clearEntireString(cmdline);
    appendStr(cmdline, "mv ");
    concatString(cmdline, library_target_filename);
    appendChar(cmdline, ' ');
    appendStr(cmdline, *BUILD_DIR);
    appendStr(cmdline, "/lib");

    printf(DYNS_FMT "\n", DYNS_ARG(cmdline));
    system(getStr(cmdline));

    freeString(library_target_filename);
}

static void build_dynamic_lib(String* cmdline, const String* premire,
                              const String** objs, const BuildData* bdata)
{
    String* library_target_filename = mkString("lib");
    appendStr(library_target_filename, bdata->targetName);
    appendStr(library_target_filename, ".so");

    concatString(cmdline, premire);
    for (size_t i = 0; i < bdata->libDirsSize; ++i)
    {
        appendStr(cmdline, "-L");
        appendStr(cmdline, bdata->libDirs[i]);
        appendChar(cmdline, ' ');
    }
    for (size_t i = 0; i < bdata->libsSize; ++i)
    {
        appendStr(cmdline, "-l");
        appendStr(cmdline, bdata->libs[i]);
        appendChar(cmdline, ' ');
    }
    for (size_t i = 0; i < bdata->srcsSize; ++i)
    {
        concatString(cmdline, objs[i]);
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
    appendStr(cmdline, *BUILD_DIR);
    appendStr(cmdline, "/lib");

    printf(DYNS_FMT "\n", DYNS_ARG(cmdline));
    system(getStr(cmdline));

    freeString(library_target_filename);
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
    appendStrBack(output, *BUILD_DIR);
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

static const String* getFlags(const String** inner, size_t len)
{
    String* output = mkString(NULL);

    for (size_t i = 0; i < len; ++i)
    {
        concatString(output, inner[i]);
        appendChar(output, ' ');
    }

    return output;
}
