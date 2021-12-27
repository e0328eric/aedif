#if !defined(__APPLE__) && !defined(__linux__)
#error "This tests only at macos and linux"
#endif

#include <dirent.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(__APPLE__) || defined(__linux__)
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#define DRAPEAU_IMPL
#include <drapeau.h>
#include <dynString.h>

#include "aedif_lua_module.h"
#include "err_print_syntax.h"
#include "lua_utils.h"
#include "predefined_vars.h"

#define AEDIF_VALID_DIR_STR "The  \xab aedif\xbc building \t  \xcd tool\xde"

// Global variables
bool* is_clean;
extern const char** build_dir;

static bool checkIsValidDirectory(const char* dir);
static void mkValidDirectoy(const char* dir);

int main(int argc, char** argv)
{
    // Parse the commandline
    drapeauStart("aedif", "A tiny C/C++ building tool");

    bool* is_build = drapeauSubcmd("build", "build the project");
    bool* build_help =
        drapeauBool("help", false, "show the help message", "build");
    build_dir =
        drapeauStr("dir", "./build/", "sets the directory to build", "build");
    bool* no_make_build_dir =
        drapeauBool("nomake", false, "no to make build directories", "build");

    is_clean = drapeauSubcmd("clean", "clean buildings");
    bool* clean_help =
        drapeauBool("help", false, "show the help message", "clean");
    const char** clean_dir =
        drapeauStr("dir", "./build/", "sets the directory to clean", "clean");
    bool* clean_force =
        drapeauBool("force", false, "do not ask the deletion warning", "clean");
    bool* read_lua_file = drapeauBool(
        "readlua", false, "Read aedif.lua to run custom clean build", "clean");

    bool* is_install = drapeauSubcmd("install", "install the project");
    bool* install_help =
        drapeauBool("help", false, "show the help message", "install");
    const char** install_dir =
        drapeauStr("dir", "~/.local/bin/",
                   "the directory to install the project", "install");

    bool* main_help = drapeauBool("help", false, "show the help message", NULL);
    // if any other arguments are given, then sets build_dir = main_build
    const char** main_build =
        drapeauStr("dir", "./build/", "sets the directory to build", NULL);
    bool* no_make_build_dir_main =
        drapeauBool("nomake", false, "no to make build directories", NULL);

    drapeauParse(argc, argv);

    // If any other commandlines are given, then sets the defalut running state
    // will be 'build' subcommand
    if (!*is_build && !*is_clean && !*is_install)
    {
        *is_build = true;
        build_dir = main_build;
        no_make_build_dir = no_make_build_dir_main;
    }

    ///////////////////////////////////////////////////////////////////////////////////

    const char* err;
    if ((err = drapeauPrintErr()) != NULL)
    {
        fprintf(stderr, "%s\n", err);
        drapeauClose();
        return 1;
    }

    if (*build_help || *clean_help || *install_help || *main_help)
    {
        drapeauPrintHelp(stdout);
        drapeauClose();
        return 0;
    }

    // check whether aedif.lua file exists
    FILE* aedif_lua_file = fopen("aedif.lua", "r");
    if (aedif_lua_file == NULL)
    {
        fprintf(stderr, AEDIF_ERROR_PREFIX
                "aedif cannot find the bootstrap file (aka. aedif.lua)\n");
        drapeauClose();
        return 1;
    }
    fclose(aedif_lua_file);

    // If build or install subcommand is given, make ./build file
    if (*is_build || *is_install)
    {
        DIR* check_build_exists = opendir(*build_dir);
        if (check_build_exists != NULL)
        {
            if (!checkIsValidDirectory(*build_dir))
            {
                fprintf(stderr,
                        AEDIF_ERROR_PREFIX "'%s' directory is already exists. "
                                           "cannot build with aedif\n",
                        *build_dir);
                fprintf(
                    stderr,
                    AEDIF_NOTE_PREFIX
                    "current '%s' directory is not made from aedif, and aedif "
                    "uses the name of directory '%s'.\n",
                    *build_dir, *build_dir);
                closedir(check_build_exists);
                drapeauClose();
                return 1;
            }
        }
        else if (!*no_make_build_dir)
        {
            mkValidDirectoy(*build_dir);
        }
    }

    // Read the aedif.lua script
    if (!*is_clean || *read_lua_file)
    {
        lua_State* L = luaL_newstate();
        predefineVars(L);
        linkAedifModule(L);
        luaL_openlibs(L);

        if (!is_ok(L, luaL_dofile(L, "aedif.lua")))
        {
            lua_close(L);
            return 1;
        }

        lua_close(L);
    }

    if (*is_clean)
    {
        String* cmdline = mkString("rm -vr ");
        appendStr(cmdline, *clean_dir);

        if (*clean_force)
        {
            printf("Cleaning all buildings\n");
            fprintf(stdout, "%s\n", getStr(cmdline));
            system(getStr(cmdline));
        }
        else
        {
            char buffer[50];
            fprintf(stdout, "\nDo you really want to execute '%s'? [y/N]: ",
                    getStr(cmdline));
            fgets(buffer, 49, stdin);
            buffer[49] = '\0';
            while (strlen(buffer) > 2 ||
                   (tolower(buffer[0]) != 'y' && tolower(buffer[0]) != 'n' &&
                    buffer[0] != '\n'))
            {
                fprintf(stdout, "\nEnter only 'y' or 'n', please [y/N]: ");
                fgets(buffer, 49, stdin);
                buffer[49] = '\0';
            }

            if (tolower(buffer[0]) == 'y')
            {
                printf("Cleaning all buildings\n");
                fprintf(stdout, "%s\n", getStr(cmdline));
                system(getStr(cmdline));
            }
        }

        freeString(cmdline);
        drapeauClose();
        return 0;
    }

    if (*is_install)
    {
        String* cmdline = mkString("mkdir -p ");
        fprintf(stdout,
                "\nInstalled in '%s'. Add this directory to PATH "
                "to use aedif\n",
                *install_dir);
        appendStr(cmdline, *install_dir);
        fprintf(stdout, "%s\n", getStr(cmdline));
        system(getStr(cmdline));

        clearEntireString(cmdline);
        appendStr(cmdline, "mv ");
        appendStr(cmdline, *build_dir);
        appendStr(cmdline, "/bin/* ");
        appendStr(cmdline, *install_dir);
        fprintf(stdout, "%s\n", getStr(cmdline));
        system(getStr(cmdline));
        freeString(cmdline);
    }

    // clean drapeau
    drapeauClose();
    return 0;
}

static bool checkIsValidDirectory(const char* dir)
{
    char buffer[40];
    size_t len;
    memset(buffer, 0, sizeof(buffer));
    String* file_name = mkString(dir);
    appendStr(file_name, "/.aedif");

    FILE* dummy = fopen(getStr(file_name), "rb");
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

    freeString(file_name);

    return (bool)(strcmp(buffer, AEDIF_VALID_DIR_STR) == 0);
}

static void mkValidDirectoy(const char* dir)
{
    String* cmdline = mkString("mkdir -p ");
    appendStr(cmdline, dir);
    size_t len = getLen(cmdline);

    appendStr(cmdline, "/lib");
    fprintf(stdout, "%s\n", getStr(cmdline));
    system(getStr(cmdline));

    clearStringAfter(cmdline, (ssize_t)len);
    appendStr(cmdline, "/obj");
    fprintf(stdout, "%s\n", getStr(cmdline));
    system(getStr(cmdline));

    clearStringAfter(cmdline, (ssize_t)len);
    appendStr(cmdline, "/bin");
    fprintf(stdout, "%s\n", getStr(cmdline));
    system(getStr(cmdline));

    clearEntireString(cmdline);
    appendStr(cmdline, dir);
    appendStr(cmdline, "/.aedif");

    FILE* dummy = fopen(getStr(cmdline), "wb");

    fwrite(AEDIF_VALID_DIR_STR, strlen(AEDIF_VALID_DIR_STR), 1, dummy);

    fclose(dummy);
    freeString(cmdline);
}
