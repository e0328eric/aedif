#ifndef AEDIF_PARSE_ARGS_H_
#define AEDIF_PARSE_ARGS_H_

#include <stdbool.h>

// Global Variables
//
// These values may called in the aedif lua functions

extern bool* g_is_build;
extern bool* g_build_help;
extern const char** g_build_dir;
extern bool* g_no_make_build_dir;

extern bool* g_is_clean;
extern bool* g_clean_help;
extern const char** g_clean_dir;
extern bool* g_clean_force;
extern bool* g_read_lua_file;

extern bool* g_is_install;
extern bool* g_install_help;
extern const char** g_install_dir;

extern bool* g_main_help;
extern const char** g_main_build;
extern bool* g_no_make_build_dir_main;

int parseArgs(int argc, char** argv);

#endif // AEDIF_PARSE_ARGS_H_
