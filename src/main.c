#if !defined(__APPLE__)
#error "Does not tested in the other operator systems"
#endif

#include <dirent.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __APPLE__
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include "commandline_parser.h"
#include "err_print_syntax.h"
#include "lua_debug.h"
#include "predefined_vars.h"
#include "registered_funcs.h"

#define AEDIF_VALID_DIR_STR "The  \xab aedif\xbc building \t  \xcd tool\xde"

static bool checkIsValidDirectory(void);
static void mkValidDirectoy(void);

// TODO(#1): make a subcommand
// whether build, clean and install the project
int main(int argc, const char** argv)
{
    // aedif forces to use the build directory in the main position (where
    // aedif.lua file exists)
    DIR* check_build_exists = opendir("./build");
    if (check_build_exists != NULL)
    {
        if (!checkIsValidDirectory())
        {
            fprintf(stderr,
                    AEDIF_ERROR_PREFIX "'./build' directory is already exists. "
                                       "cannot build with aedif\n");
            fprintf(
                stderr, AEDIF_NOTE_PREFIX
                "current './build' directory is not made from aedif, and aedif "
                "uses the name of directory './build'.\n");
            closedir(check_build_exists);
            return 1;
        }
    }
    else
    {
        mkValidDirectoy();
    }

    ExecKind kind;
    switch (kind = cmdParser(argc, argv))
    {
    case EXEC_KIND_CLEAN:
        printf("Cleaning all buildings\n");
        system("rm -r ./build");
        return 0;

    case EXEC_KIND_BUILD:
    case EXEC_KIND_INSTALL: {
        lua_State* L = luaL_newstate();
        predefineVars(L);
        lua_register(L, "Compile", lua_Compile);
        lua_register(L, "RestoreSettings", lua_RestoreSettings);
        lua_register(L, "Execute", lua_Execute);
        luaL_openlibs(L);

        if (!is_ok(L, luaL_dofile(L, "aedif.lua")))
        {
            lua_close(L);
            return 1;
        }

        lua_close(L);

        if (kind == EXEC_KIND_INSTALL)
        {
            printf("\nInstalled in '~/.local/bin'. Add this directory to PATH "
                   "to use aedif\n");
			system("mkdir -p ~/.local/bin");
            system("mv ./build/bin/* ~/.local/bin/");
        }

        break;
    }
    default:
        fprintf(stderr, AEDIF_INTERNAL_ERR_PREFIX "Unreatchable (main)\n");
        return 1;
    }

    return 0;
}

static bool checkIsValidDirectory(void)
{
    char buffer[40];
    size_t len;
    memset(buffer, 0, sizeof(buffer));

    FILE* dummy = fopen("./build/.aedif", "rb");
    if (dummy == NULL)
    {
        return false;
    }
    fseek(dummy, 0, SEEK_END);
    len = (size_t)ftell(dummy);
    len = len >= 40 ? 40 : len;
    rewind(dummy);
    fread(buffer, len, 1, dummy);
    buffer[39] = '\0';

    return (bool)(strcmp(buffer, AEDIF_VALID_DIR_STR) == 0);
}

static void mkValidDirectoy(void)
{
    mkdir("./build", S_IRWXU | S_IRGRP | S_IXGRP | S_IXOTH);
    mkdir("./build/lib", S_IRWXU | S_IRGRP | S_IXGRP | S_IXOTH);
    mkdir("./build/obj", S_IRWXU | S_IRGRP | S_IXGRP | S_IXOTH);
    mkdir("./build/bin", S_IRWXU | S_IRGRP | S_IXGRP | S_IXOTH);

    FILE* dummy = fopen("./build/.aedif", "wb");

    fwrite(AEDIF_VALID_DIR_STR, strlen(AEDIF_VALID_DIR_STR), 1, dummy);

    fclose(dummy);
}
