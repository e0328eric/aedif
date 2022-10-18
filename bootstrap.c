#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>

#define C_COMPILER "cl"
#define C_LINKER "lib"
#define C_OPTIONS "/w"
#else
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

#define C_COMPILER "gcc"
#define C_LINKER "ar"
#define C_OPTIONS                                                              \
    "-O3 -std=c11 -Wall -Wextra -Wpedantic -Wno-empty-translation-unit "       \
    "-Werror=return-type"
#endif

#define DRAPEAU_IMPL
#define DYN_STRING_IMPL
#include "./lib/drapeau/drapeau.h"
#include "./lib/dynString/dynString.h"

static String* changeExtension(const char* filename, const char* extension,
                               const char* where);
static void compile_srcs(bool is_lib, const char* target_name,
                         const char* options, const char** srcs,
                         const char** includes, const char** libs,
                         const char** lib_dirs, const char* where);

const char* lua_srcs[] = {
    "./lib/lua/lapi.c",     "./lib/lua/lauxlib.c",
    "./lib/lua/lbaselib.c", "./lib/lua/lcode.c",
    "./lib/lua/lcorolib.c", "./lib/lua/lctype.c",
    "./lib/lua/ldblib.c",   "./lib/lua/ldebug.c",
    "./lib/lua/ldo.c",      "./lib/lua/ldump.c",
    "./lib/lua/lfunc.c",    "./lib/lua/lgc.c",
    "./lib/lua/linit.c",    "./lib/lua/liolib.c",
    "./lib/lua/llex.c",     "./lib/lua/lmathlib.c",
    "./lib/lua/lmem.c",     "./lib/lua/loadlib.c",
    "./lib/lua/lobject.c",  "./lib/lua/lopcodes.c",
    "./lib/lua/loslib.c",   "./lib/lua/lparser.c",
    "./lib/lua/lstate.c",   "./lib/lua/lstring.c",
    "./lib/lua/lstrlib.c",  "./lib/lua/ltable.c",
    "./lib/lua/ltablib.c",  "./lib/lua/ltm.c",
    "./lib/lua/lua.c",      "./lib/lua/lundump.c",
    "./lib/lua/lutf8lib.c", "./lib/lua/lvm.c",
    "./lib/lua/lzio.c",     NULL,
};

const char* lua_includes[] = {
    "./lib/lua/src/",
    NULL,
};

const char* main_srcs[] = {
    "./src/lua_utils.c",
    "./src/parse_args.c",
    "./src/error/print_error.c",
    "./src/ffi/link_lua.c",
    "./src/ffi/os/aedif_os_abspath.c",
    "./src/ffi/os/aedif_os_copy.c",
    "./src/ffi/os/aedif_os_isdir.c",
    "./src/ffi/os/aedif_os_isfile.c",
    "./src/ffi/os/aedif_os_issym.c",
    "./src/ffi/os/aedif_os_mkdir.c",
    "./src/ffi/os/aedif_os_ostype.c",
    "./src/ffi/os/aedif_os_rmdir.c",
    "./src/ffi/os/aedif_os_rmfile.c",
    "./src/ffi/os/aedif_os_rename.c",
    "./src/main.c",
    NULL,
};

const char* main_includes[] = {
    "./lib/lua/", "./lib/drapeau", "./lib/dynString", "./src/",
    "./src/ffi/", "./src/ffi/os/", NULL,
};

#ifdef _WIN32
const char* main_libs[] = {
    "./build/lib/liblua.lib",
    NULL,
};
#else
const char* main_libs[] = {
    "lua",
    NULL,
};
const char* main_lib_dirs[] = {
    "./build/lib/",
    NULL,
};
#endif

int main(int argc, char** argv)
{
    drapeauStart("cb", "A building script written in C");
    bool* is_clean = drapeauSubcmd("c", "clean the built project");
    bool* clean_with_self = drapeauBool(
        "self", NO_SHORT, false, "clean the built project and remove itself too", "c");
    bool* is_build = drapeauSubcmd("b", "build the project");
    bool* is_install = drapeauSubcmd("i", "install the project");

    if (!drapeauParse(argc, argv))
    {
        fprintf(stderr, "%s\n", drapeauGetErr());
        drapeauClose();
        return 1;
    }
    drapeauClose();

    if (drapeauIsHelp())
    {
        drapeauPrintHelp();
        return 0;
    }

    if (!*is_clean && !*is_build && !*is_install)
    {
        drapeauPrintHelp();
        return 0;
    }

    if (*is_clean)
    {
        printf("Clean build\n");
#ifdef _WIN32
        system("rd /s /q .\\build");
#else
        system("rm -rf ./build");
#endif
        if (*clean_with_self)
        {
            printf("Clean the build executable itself\n");
            String* filename = mkString(argv[0]);
            // TODO: run this script on Windows to test that this code works
#ifdef _WIN32
            appendStrBack(filename, "del ");
            system("del .\\cb.obj");
            system(getStr(filename));
#else
            appendStrBack(filename, "rm -rf ");
            system(getStr(filename));
#endif
        }
        return 0;
    }

    // making directories
    if (*is_build || *is_install)
    {
#ifdef _WIN32
        CreateDirectory("./build", NULL);
        CreateDirectory("./build/obj", NULL);
        CreateDirectory("./build/obj/lua", NULL);
        CreateDirectory("./build/obj/aedif", NULL);
        CreateDirectory("./build/lib", NULL);
        CreateDirectory("./build/bin", NULL);

        compile_srcs(true, "liblua.lib", "/w", lua_srcs, lua_includes, NULL,
                     NULL, "./build/obj/lua");
        compile_srcs(false, "aedif.exe", C_OPTIONS, main_srcs, main_includes,
                     main_libs, NULL, "./build/obj/aedif");
#else
        mkdir("./build", 00755);
        mkdir("./build/obj", 00755);
        mkdir("./build/obj/lua", 00755);
        mkdir("./build/obj/aedif", 00755);
        mkdir("./build/lib", 00755);
        mkdir("./build/bin", 00755);

        compile_srcs(true, "liblua.a", "-std=gnu99 -Wno-gnu-label-as-value",
                     lua_srcs, lua_includes, NULL, NULL, "./build/obj/lua");
        compile_srcs(false, "aedif", C_OPTIONS, main_srcs, main_includes,
                     main_libs, main_lib_dirs, "./build/obj/aedif");
#endif
    }

    return 0;
}

static void compile_srcs(bool is_lib, const char* target_name,
                         const char* options, const char** srcs,
                         const char** includes, const char** libs,
                         const char** lib_dirs, const char* where)
{
    size_t src_len = 0;
    while (srcs[src_len])
    {
        ++src_len;
    }

    String* cmdline = mkString(NULL);
    String** objs = malloc(sizeof(String*) * (src_len + 1));
    objs[src_len] = NULL;

    appendStr(cmdline, C_COMPILER " ");
    appendStr(cmdline, options);
    appendChar(cmdline, ' ');

    for (; *includes; ++includes)
    {
#ifdef _WIN32
        appendStr(cmdline, "/I");
#else
        appendStr(cmdline, "-I");
#endif
        appendStr(cmdline, *includes);
        appendChar(cmdline, ' ');
    }

#ifdef _WIN32
    appendStr(cmdline, "/c ");
#else
    appendStr(cmdline, "-c ");
#endif
    size_t len = getLen(cmdline);
    for (size_t i = 0; i < src_len; ++i)
    {
        clearStringAfter(cmdline, len);
        appendStr(cmdline, srcs[i]);
#ifdef _WIN32
        objs[i] = changeExtension(srcs[i], "obj", where);
        appendStr(cmdline, " /Fo: ");
#else
        objs[i] = changeExtension(srcs[i], "o", where);
        appendStr(cmdline, " -o ");
#endif
        concatString(cmdline, objs[i]);
        printf(DYNS_FMT "\n", DYNS_ARG(cmdline));
        system(getStr(cmdline));
    }

    clearEntireString(cmdline);
    if (is_lib)
    {
        appendStr(cmdline, C_LINKER " ");
#ifdef _WIN32
        appendStr(cmdline, " /out:");
        appendStr(cmdline, "./build/lib/");
        appendStr(cmdline, target_name);
#else
        appendStr(cmdline, "rcu ");
        appendStr(cmdline, "./build/lib/");
        appendStr(cmdline, target_name);
#endif
        appendChar(cmdline, ' ');

        for (size_t i = 0; i < src_len; ++i)
        {
            concatFreeString(cmdline, objs[i]);
            appendChar(cmdline, ' ');
        }
        printf(DYNS_FMT "\n", DYNS_ARG(cmdline));
        system(getStr(cmdline));
    }
    else
    {
        appendStr(cmdline, C_COMPILER " ");
        for (size_t i = 0; objs[i]; ++i)
        {
            concatFreeString(cmdline, objs[i]);
            appendChar(cmdline, ' ');
        }
#ifdef _WIN32
        for (; *libs; ++libs)
        {
            appendStr(cmdline, *libs);
            appendChar(cmdline, ' ');
        }
        appendStr(cmdline, " /Fe:");
        appendStr(cmdline, "./build/bin/");
        appendStr(cmdline, target_name);
#else
        appendStr(cmdline, " -o ");
        appendStr(cmdline, "./build/bin/");
        appendStr(cmdline, target_name);
        appendChar(cmdline, ' ');
        for (; *libs; ++libs)
        {
            appendStr(cmdline, "-l");
            appendStr(cmdline, *libs);
            appendChar(cmdline, ' ');
        }
        for (; *lib_dirs; ++lib_dirs)
        {
            appendStr(cmdline, "-L");
            appendStr(cmdline, *lib_dirs);
            appendChar(cmdline, ' ');
        }
#endif
        printf(DYNS_FMT "\n", DYNS_ARG(cmdline));
        system(getStr(cmdline));
    }

    free(objs);
    freeString(cmdline);
}

static String* changeExtension(const char* filename, const char* extension,
                               const char* where)
{
    ssize_t location;
    String* output = mkString(filename);

    if ((location = findCharReverse(output, '.')) < 0)
    {
        freeString(output);
        return NULL;
    }

    clearStringAfter(output, location);
    appendChar(output, '.');
    appendStr(output, extension);

    if ((location = findCharReverse(output, '/')) < 0)
    {
        freeString(output);
        return NULL;
    }

    clearStringBefore(output, location);
    if (!where)
    {
        appendStrBack(output, "./build/obj");
    }
    else
    {
        appendStrBack(output, where);
    }

    return output;
}
