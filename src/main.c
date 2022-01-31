#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include "error/print_error.h"
#include "ffi/link_lua.h"
#include "lua_utils.h"
#include "parse_args.h"

#define AEDIF_VALID_DIR_STR "The  \xab aedif\xbc building \t  \xcd tool\xde"

#ifdef _WIN32
// Some old MinGW/CYGWIN distributions don't define this:
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

static HANDLE stdoutHandle;
static DWORD outModeInit;

// copied from:
// https://stackoverflow.com/questions/62784691/coloring-text-in-cmd-c
void setupConsole(void);
void restoreConsole(void);
#endif

int main(int argc, char** argv)
{
#ifdef _WIN32
    setupConsole();
#endif

    switch (parseArgs(argc, argv))
    {
    case 0:
        break;
    case 1:
        return 0;
        break;
    default:
        return 1;
        break;
    }

    // check whether aedif.lua file exists
    FILE* aedif_lua_file = fopen("foo.lua", "r");
    if (aedif_lua_file == NULL)
    {
        print_error(NULL, AEDIF_ERROR,
                    "aedif cannot find the bootstrap file (aka. foo.lua)\n");
        return 1;
    }
    fclose(aedif_lua_file);

    lua_State* L = luaL_newstate();
    luaL_openlibs(L);
    makeAedifLuaLibrary(L);

    if (!is_ok(L, luaL_dofile(L, "foo.lua")))
    {
        lua_close(L);
        return 1;
    }

    lua_close(L);

#ifdef _WIN32
    restoreConsole();
#endif

    return 0;
}

// copied from:
// https://stackoverflow.com/questions/62784691/coloring-text-in-cmd-c
#ifdef _WIN32
void setupConsole(void)
{
    DWORD outMode = 0;
    stdoutHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    if (stdoutHandle == INVALID_HANDLE_VALUE)
    {
        exit(GetLastError());
    }

    if (!GetConsoleMode(stdoutHandle, &outMode))
    {
        exit(GetLastError());
    }

    outModeInit = outMode;

    // Enable ANSI escape codes
    outMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;

    if (!SetConsoleMode(stdoutHandle, outMode))
    {
        exit(GetLastError());
    }
}

void restoreConsole(void)
{
    // Reset colors
    printf("\x1b[0m");

    // Reset console mode
    if (!SetConsoleMode(stdoutHandle, outModeInit))
    {
        exit(GetLastError());
    }
}
#endif
