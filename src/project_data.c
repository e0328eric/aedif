#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define DYN_STRING_IMPL
#include <dynString.h>

#include "err_print_syntax.h"
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

#define CHECK_IS_WHITESPACE(str) ((*str) == ' ' || (*str) == '\t')

/****************/
/* Declarations */
/****************/
typedef enum ParseTableErrkind
{
    PARSE_TABLE_ERR_KIND_OK = 0,
    PARSE_TABLE_ERR_KIND_INNER_TYPE_MISMATCHED,
    PARSE_TABLE_ERR_KIND_OUTER_TYPE_MISMATCHED,
} ParseTableErrkind;

static ParseTableErrkind parseTableData(lua_State* L, const char* var_name,
                                        const String*** data, size_t* data_len);

static bool isCppName(const char* name);
static bool isCName(const char* name);
static StdType int2StdType(int val, LangType language);

/*********************************/
/* Main Function implementations */
/*********************************/
#define INIT_FAILED(msg)                                                       \
    do                                                                         \
    {                                                                          \
        *output = (msg);                                                       \
        return pdata;                                                          \
    } while (false)

ProjectData getProjectData(const char** output, lua_State* L)
{
    static bool is_first_called = true;

    ProjectData pdata;
    int lua_output_type;

    // make every entries of pdata to be zero
    memset(&pdata, 0, sizeof(pdata));

    /******************/
    /* Parse Language */
    /******************/
    lua_getglobal(L, "LANGUAGE");
    if (!lua_isstring(L, -1))
    {
        INIT_FAILED("LANGUAGE is not found or the type is not 'string'");
    }

    const char* lang_name = lua_tostring(L, -1);
    if (isCppName(lang_name))
    {
        pdata.language = LANG_TYPE_CPP;
    }
    else if (isCName(lang_name))
    {
        pdata.language = LANG_TYPE_C;
    }
    else
    {
        INIT_FAILED("Invalid LANGUAGE name is given");
    }
    lua_pop(L, 1);

    /******************/
    /* Parse Compiler */
    /******************/
    lua_getglobal(L, "COMPILER");
    if (!lua_isstring(L, -1))
    {
        INIT_FAILED("COMPILER is not found or the type is not 'string'");
    }

    const char* compiler_name = lua_tostring(L, -1);
    pdata.compiler = mkString(compiler_name);
    lua_pop(L, 1);

    /*************/
    /* Parse Std */
    /*************/
    lua_output_type = lua_getglobal(L, "STD");
    switch (lua_output_type)
    {
    case LUA_TNIL:
        pdata.std = pdata.language == LANG_TYPE_C ? STD_TYPE_C_PLAIN
                                                  : STD_TYPE_CPP_PLAIN;
        break;

    case LUA_TNUMBER: {
        int std_lua_val = (int)lua_tonumber(L, -1);
        pdata.std = int2StdType(std_lua_val, pdata.language);
        break;
    }

    default:
        INIT_FAILED("The type of STD is neither 'nil' nor 'number'");
        break;
    }
    lua_pop(L, 1);

    /************************/
    /* Parse Optimize Level */
    /************************/
    lua_output_type = lua_getglobal(L, "OPT_LEVEL");
    switch (lua_output_type)
    {
    case LUA_TNIL:
        pdata.optLevel = OPT_LEVEL_NO_OPTIMIZE;
        break;

    case LUA_TNUMBER: {
        int opt_val = (int)lua_tonumber(L, -1);
        if (opt_val < 0 || opt_val > 3)
        {
            INIT_FAILED("OPT_LEVEL can be either 0, 1, 2, 3 or \"s\"");
        }
        pdata.optLevel = (OptLevel)(opt_val);
        break;
    }

    case LUA_TSTRING: {
        const char* opt_val = lua_tostring(L, -1);
        // TODO: "s" is the only valid of the OPT_LEVEL
        if (strncmp(opt_val, "s\0", 2) != 0)
        {
            INIT_FAILED("OPT_LEVEL can be either 0, 1, 2, 3 or \"s\"");
        }
        pdata.optLevel = OPT_LEVEL_SIZE;
        break;
    }

    default:
        INIT_FAILED(
            "The type of OPT_LEVEL is neither 'nil', 'number' nor 'string'");
        break;
    }
    lua_pop(L, 1);

    /******************/
    /* Parse Warnings */
    /******************/
    switch (parseTableData(L, "WARNINGS", &pdata.warnings, &pdata.warningsSize))
    {
    case PARSE_TABLE_ERR_KIND_OK:
        break;

    case PARSE_TABLE_ERR_KIND_INNER_TYPE_MISMATCHED:
        INIT_FAILED("The type of elements of WARNINGS must be 'string'");
        break;

    case PARSE_TABLE_ERR_KIND_OUTER_TYPE_MISMATCHED:
        INIT_FAILED("The type of WARNINGS is neither 'nil' nor 'table'");
        break;
    }

    /****************/
    /* Parse Errors */
    /****************/
    switch (parseTableData(L, "ERRORS", &pdata.errors, &pdata.errorsSize))
    {
    case PARSE_TABLE_ERR_KIND_OK:
        break;

    case PARSE_TABLE_ERR_KIND_INNER_TYPE_MISMATCHED:
        INIT_FAILED("The type of elements of ERRORS must be 'string'");
        break;

    case PARSE_TABLE_ERR_KIND_OUTER_TYPE_MISMATCHED:
        INIT_FAILED("The type of ERRORS is neither 'nil' nor 'table'");
        break;
    }

    /***********************/
    /* Parse Compile Flags */
    /***********************/
    switch (parseTableData(L, "COMPILE_FLAGS", &pdata.compileFlags,
                           &pdata.compileFlagsSize))
    {
    case PARSE_TABLE_ERR_KIND_OK:
        break;

    case PARSE_TABLE_ERR_KIND_INNER_TYPE_MISMATCHED:
        INIT_FAILED("The type of elements of COMPILE_FLAGS must be 'string'");
        break;

    case PARSE_TABLE_ERR_KIND_OUTER_TYPE_MISMATCHED:
        INIT_FAILED("The type of COMPILE_FLAGS is neither 'nil' nor 'table'");
        break;
    }

    /********************/
    /* Parse Link Flags */
    /********************/
    switch (
        parseTableData(L, "LINK_FLAGS", &pdata.linkFlags, &pdata.linkFlagsSize))
    {
    case PARSE_TABLE_ERR_KIND_OK:
        break;

    case PARSE_TABLE_ERR_KIND_INNER_TYPE_MISMATCHED:
        INIT_FAILED("The type of elements of LINK_FLAGS must be 'string'");
        break;

    case PARSE_TABLE_ERR_KIND_OUTER_TYPE_MISMATCHED:
        INIT_FAILED("The type of LINK_FLAGS is neither 'nil' nor 'table'");
        break;
    }

    /***************/
    /* Parse Flags */
    /***************/
    switch (parseTableData(L, "FLAGS", &pdata.flags, &pdata.flagsSize))
    {
    case PARSE_TABLE_ERR_KIND_OK:
        break;

    case PARSE_TABLE_ERR_KIND_INNER_TYPE_MISMATCHED:
        INIT_FAILED("The type of elements of FLAGS must be 'string'");
        break;

    case PARSE_TABLE_ERR_KIND_OUTER_TYPE_MISMATCHED:
        INIT_FAILED("The type of FLAGS is neither 'nil' nor 'table'");
        break;
    }

    // Set output to NULL so that we can check whether constructing ProjectData
    // is successed
    *output = NULL;

    if (is_first_called)
    {
        ASSIGN_VAR("LANGUAGE", AEDIF_LANGUAGE);
        ASSIGN_VAR("COMPILER", AEDIF_COMPILER);
        ASSIGN_VAR("STD", AEDIF_STD);
        ASSIGN_VAR("OPT_LEVEL", AEDIF_OPT_LEVEL);
        ASSIGN_VAR("WARNINGS", AEDIF_WARNINGS);
        ASSIGN_VAR("ERRORS", AEDIF_ERRORS);
        ASSIGN_VAR("COMPILE_FLAGS", AEDIF_COMPILE_FLAGS);
        ASSIGN_VAR("LINK_FLAGS", AEDIF_LINK_FLAGS);
        ASSIGN_VAR("FLAGS", AEDIF_FLAGS);
        is_first_called = false;
    }

    return pdata;
}

static ParseTableErrkind parseTableData(lua_State* L, const char* var_name,
                                        const String*** data, size_t* data_len)
{
    int lua_output_type = lua_getglobal(L, var_name);

    switch (lua_output_type)
    {
    case LUA_TNIL:
        break;

    case LUA_TTABLE: {
        lua_len(L, -1);
        *data_len = (size_t)lua_tointeger(L, -1);
        lua_pop(L, 1);

        *data = malloc(sizeof(String*) * *data_len);

        for (size_t i = 0; i < *data_len; ++i)
        {
            lua_pushinteger(L, (lua_Integer)(i + 1));
            lua_gettable(L, -2);
            if (lua_type(L, -1) != LUA_TSTRING)
            {
                return PARSE_TABLE_ERR_KIND_INNER_TYPE_MISMATCHED;
            }
            (*data)[i] = mkString(lua_tostring(L, -1));
            lua_pop(L, 1);
        }

        break;
    }

    default:
        return PARSE_TABLE_ERR_KIND_OUTER_TYPE_MISMATCHED;
    }
    lua_pop(L, 1);

    return PARSE_TABLE_ERR_KIND_OK;
}

#undef INIT_FAILED

void freeInnerProjectData(ProjectData* pdata)
{
    size_t i;

    freeString((String*)pdata->compiler);

    for (i = 0; i < pdata->warningsSize; ++i)
    {
        freeString((String*)pdata->warnings[i]);
    }
    free((void*)pdata->warnings);
    pdata->warnings = NULL;

    for (i = 0; i < pdata->errorsSize; ++i)
    {
        freeString((String*)pdata->errors[i]);
    }
    free((void*)pdata->errors);
    pdata->errors = NULL;

    for (i = 0; i < pdata->compileFlagsSize; ++i)
    {
        freeString((String*)pdata->compileFlags[i]);
    }
    free((void*)pdata->compileFlags);
    pdata->compileFlags = NULL;

    for (i = 0; i < pdata->linkFlagsSize; ++i)
    {
        freeString((String*)pdata->linkFlags[i]);
    }
    free((void*)pdata->linkFlags);
    pdata->linkFlags = NULL;

    for (i = 0; i < pdata->flagsSize; ++i)
    {
        freeString((String*)pdata->flags[i]);
    }
    free((void*)pdata->flags);
    pdata->flags = NULL;
}

/****************************/
/* Function implementations */
/****************************/
static bool isCppName(const char* name)
{
    if (strncmp(name, "Cpp\0", 4) == 0)
    {
        return true;
    }
    if (strncmp(name, "cpp\0", 4) == 0)
    {
        return true;
    }
    if (strncmp(name, "C++\0", 4) == 0)
    {
        return true;
    }
    if (strncmp(name, "c++\0", 4) == 0)
    {
        return true;
    }

    return false;
}

static bool isCName(const char* name)
{
    if (strncmp(name, "C\0", 2) == 0)
    {
        return true;
    }
    if (strncmp(name, "c\0", 2) == 0)
    {
        return true;
    }

    return false;
}

static StdType int2StdType(int val, LangType langType)
{
    switch (langType)
    {
    case LANG_TYPE_C:
        switch (val)
        {
        case 99:
            return STD_TYPE_C_99;
        case 11:
            return STD_TYPE_C_11;
        case 14:
            return STD_TYPE_C_14;
        case 17:
            return STD_TYPE_C_17;
        case 23:
            return STD_TYPE_C_23;
        default:
            fprintf(stderr, AEDIF_WARN_PREFIX
                    "Invalid std found. Setting C plain instead\n");
            return STD_TYPE_C_PLAIN;
        }
    case LANG_TYPE_CPP:
        switch (val)
        {
        case 11:
            return STD_TYPE_CPP_11;
        case 14:
            return STD_TYPE_CPP_14;
        case 17:
            return STD_TYPE_CPP_17;
        case 20:
            return STD_TYPE_CPP_20;
        case 23:
            return STD_TYPE_CPP_23;
        default:
            fprintf(stderr, AEDIF_WARN_PREFIX
                    "Invalid std found. Setting C++ plain instead\n");
            return STD_TYPE_CPP_PLAIN;
        }
    default:
        ASSERT(false, "Unreatchable (int2StdType)");
        // make compiler happy
        return (StdType)0;
    }
}
