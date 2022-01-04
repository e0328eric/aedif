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
#include "./lib/drapeau.h"
#include "./lib/dynString.h"

static String* changeExtension(const char* filename, const char* extension,
                               const char* where);
static void compile_srcs(bool is_lib, const char* target_name,
                         const char* options, const char** srcs,
                         const char** includes, const char** libs,
                         const char** lib_dirs, const char* where);

const char* lua_srcs[] = {
    "./lib/lua-5.4.3/src/lapi.c",     "./lib/lua-5.4.3/src/lauxlib.c",
    "./lib/lua-5.4.3/src/lbaselib.c", "./lib/lua-5.4.3/src/lcode.c",
    "./lib/lua-5.4.3/src/lcorolib.c", "./lib/lua-5.4.3/src/lctype.c",
    "./lib/lua-5.4.3/src/ldblib.c",   "./lib/lua-5.4.3/src/ldebug.c",
    "./lib/lua-5.4.3/src/ldo.c",      "./lib/lua-5.4.3/src/ldump.c",
    "./lib/lua-5.4.3/src/lfunc.c",    "./lib/lua-5.4.3/src/lgc.c",
    "./lib/lua-5.4.3/src/linit.c",    "./lib/lua-5.4.3/src/liolib.c",
    "./lib/lua-5.4.3/src/llex.c",     "./lib/lua-5.4.3/src/lmathlib.c",
    "./lib/lua-5.4.3/src/lmem.c",     "./lib/lua-5.4.3/src/loadlib.c",
    "./lib/lua-5.4.3/src/lobject.c",  "./lib/lua-5.4.3/src/lopcodes.c",
    "./lib/lua-5.4.3/src/loslib.c",   "./lib/lua-5.4.3/src/lparser.c",
    "./lib/lua-5.4.3/src/lstate.c",   "./lib/lua-5.4.3/src/lstring.c",
    "./lib/lua-5.4.3/src/lstrlib.c",  "./lib/lua-5.4.3/src/ltable.c",
    "./lib/lua-5.4.3/src/ltablib.c",  "./lib/lua-5.4.3/src/ltm.c",
    "./lib/lua-5.4.3/src/lua.c",      "./lib/lua-5.4.3/src/lundump.c",
    "./lib/lua-5.4.3/src/lutf8lib.c", "./lib/lua-5.4.3/src/lvm.c",
    "./lib/lua-5.4.3/src/lzio.c",     NULL,
};

const char* lua_includes[] = {
    "./lib/lua-5.4.3/src/",
    NULL,
};

const char* main_srcs[] = {
    "./src/lua_utils.c",
    "./src/parse_args.c",
    "./src/ffi/link_lua.c",
    "./src/ffi/os/aedif_os_abspath.c",
    "./src/ffi/os/aedif_os_copy.c",
    "./src/ffi/os/aedif_os_isdir.c",
    "./src/ffi/os/aedif_os_isfile.c",
    "./src/ffi/os/aedif_os_issym.c",
    "./src/ffi/os/aedif_os_mkdir.c",
    "./src/ffi/os/aedif_os_ostype.c",
    "./src/ffi/os/aedif_os_remove.c",
    "./src/ffi/os/aedif_os_rename.c",
    "./src/main.c",
    NULL,
};

const char* main_includes[] = {
    "./lib/lua-5.4.3/src/", "./lib/",        "./src/",
    "./src/ffi/",           "./src/ffi/os/", NULL,
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
        "self", false, "clean the built project and remove itself too", "c");
    bool* is_clean_help =
        drapeauBool("help", false, "Show the help message", "c");
    bool* is_build = drapeauSubcmd("b", "build the project");
    bool* is_install = drapeauSubcmd("i", "install the project");
    bool* is_help = drapeauBool("help", false, "Show the help message", NULL);

    drapeauParse(argc, argv);
    drapeauClose();

    if (*is_help || *is_clean_help)
    {
        drapeauPrintHelp(stdout);
        return 0;
    }

    if (!*is_clean && !*is_build && !*is_install)
    {
        *is_build = true;
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
            printf("Clean cb itself\n");
#ifdef _WIN32
            system("del .\\cb.obj");
            system("del .\\cb.exe");
#else
            system("rm -rf ./cb");
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
        mkdir("./build", 07755);
        mkdir("./build/obj", 07755);
        mkdir("./build/obj/lua", 07755);
        mkdir("./build/obj/aedif", 07755);
        mkdir("./build/lib", 07755);
        mkdir("./build/bin", 07755);

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
