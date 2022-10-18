#define DRAPEAU_IMPL
#include <drapeau.h>

#include "parse_args.h"

// Global Variables
bool* g_is_build;
bool* g_build_help;
const char** g_build_dir;
bool* g_no_make_build_dir;

bool* g_is_clean;
bool* g_clean_help;
const char** g_clean_dir;
bool* g_clean_force;
bool* g_read_lua_file;

bool* g_is_install;
bool* g_install_help;
const char** g_install_dir;

bool* g_main_help;
const char** g_main_build;
bool* g_no_make_build_dir_main;

int parseArgs(int argc, char** argv)
{
#if 0
    // Parse the commandline
    drapeauStart("aedif", "A tiny C/C++ building tool");

    g_is_build = drapeauSubcmd("build", "build the project");
    g_build_help = drapeauBool("help", false, "show the help message", "build");
    g_build_dir =
        drapeauStr("dir", "./build/", "sets the directory to build", "build");
    g_no_make_build_dir =
        drapeauBool("nomake", false, "no to make build directories", "build");

    g_is_clean = drapeauSubcmd("clean", "clean buildings");
    g_clean_help = drapeauBool("help", false, "show the help message", "clean");
    g_clean_dir =
        drapeauStr("dir", "./build/", "sets the directory to clean", "clean");
    g_clean_force =
        drapeauBool("force", false, "do not ask the deletion warning", "clean");
    g_read_lua_file = drapeauBool(
        "readlua", false, "Read aedif.lua to run custom clean build", "clean");

    g_is_install = drapeauSubcmd("install", "install the project");
    g_install_help =
        drapeauBool("help", false, "show the help message", "install");
    g_install_dir =
        drapeauStr("dir", "~/.local/bin/",
                   "the directory to install the project", "install");

    g_main_help = drapeauBool("help", false, "show the help message", NULL);
    // if any other arguments are given, then sets BUILD_DIR = main_build
    g_main_build =
        drapeauStr("dir", "./build/", "sets the directory to build", NULL);
    g_no_make_build_dir_main =
        drapeauBool("nomake", false, "no to make build directories", NULL);

    drapeauParse(argc, argv, true);

    // If any other commandlines are given, then sets the defalut running state
    // will be 'build' subcommand
    if (!*g_is_build && !*g_is_clean && !*g_is_install)
    {
        *g_is_build = true;
        g_build_dir = g_main_build;
        g_no_make_build_dir = g_no_make_build_dir_main;
    }

    const char* err;
    if ((err = drapeauGetErr()) != NULL)
    {
        fprintf(stderr, "%s\n", err);
        drapeauClose();
        return -1;
    }

    if (*g_build_help || *g_clean_help || *g_install_help || *g_main_help)
    {
        drapeauPrintHelp(stdout);
        drapeauClose();
        return 1;
    }
#endif

    return 0;
}
