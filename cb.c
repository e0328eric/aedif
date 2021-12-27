#if !defined(__APPLE__) && !defined(__linux__)
#error "build script is tested only at macos"
#endif

#if !defined(__GNUC__)
#error "Compiler Support List: gcc"
#endif

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(__APPLE__) || defined(__linux__)
#include <sys/types.h>
#endif

#define DYN_STRING_IMPL
#define DRAPEAU_IMPL
#include "lib/drapeau.h"
#include "lib/dynString.h"

// change compiler if the user wants other one
#define C_COMPILER "gcc"
#define C_STD "-std=c11"
#define C_OPTIONS                                                              \
    "-O3 -Wall -Wextra -Wpedantic -Wconversion -Werror=return-type"
#define TARGET_NAME "aedif"
#define INSTALL_DIR "~/.local/bin/"

const char* srcs[] = {
    "./src/main.c",
    "./src/lua_utils.c",
    "./src/conversion.c",
    "./src/aedif_lua_module.c",
    "./src/predefined_vars.c",
    "./src/project_data.c",
    "./src/build_data.c",
    NULL,
};

#ifdef __APPLE__
const char* libs[] = {
    "-llua",
    NULL,
};
#endif
#ifdef __linux__
const char* libs[] = {
    "-llua",
    "-lm",
    "-ldl",
    NULL,
};
#endif

const char* lib_dirs[] = {
    "-Lbuild/lib",
    NULL,
};

const char* includes[] = {
    "-Ilib/lua-5.4.3/src/",
    "-Ilib",
    NULL,
};

static String* changeExtension(const char* filename, const char* extension);

int main(int argc, char** argv)
{
    // parse commandline
    drapeauStart("cb", "A build script for aedif project written in C");

    bool* is_clean = drapeauSubcmd("c", "Clean the build datas");
    bool* is_build = drapeauSubcmd("b", "Build aedif");
    bool* is_install = drapeauSubcmd("i", "Install aedif");

    bool* help_msg = drapeauBool("help", false, "help message", NULL);

    drapeauParse(argc, argv);

    const char* err;
    if ((err = drapeauPrintErr()) != NULL)
    {
        fprintf(stderr, "%s\n", err);
        drapeauClose();
        return 1;
    }

    drapeauClose();

    if (*help_msg)
    {
        drapeauPrintHelp(stdout);
        return 0;
    }

    // if is_clean, delete build directory
    if (*is_clean)
    {
        system("make clean -C ./lib/lua-5.4.3/");
        system("rm -r ./build");
    }
    else if (*is_build || *is_install)
    {
        // make build directory
        String** objs = malloc(sizeof(srcs) * sizeof(String*));

        system("mkdir -p ./build/obj/");
        system("mkdir -p ./build/lib/");
        system("mkdir -p ./build/bin/");

        system("make -C ./lib/lua-5.4.3/");
        system("mv ./lib/lua-5.4.3/src/liblua.a ./build/lib");

        String* cmdline = mkString(C_COMPILER " " C_STD " " C_OPTIONS " ");
        for (size_t i = 0; includes[i]; ++i)
        {
            appendStr(cmdline, includes[i]);
            appendChar(cmdline, ' ');
        }
        size_t location = getLen(cmdline);

        for (size_t i = 0; srcs[i]; ++i)
        {
            clearStringAfter(cmdline, location);
            appendStr(cmdline, "-c ");
            appendStr(cmdline, srcs[i]);
            appendStr(cmdline, " -o ");
            objs[i] = changeExtension(srcs[i], "o");
            concatString(cmdline, objs[i]);
            printf(DYNS_FMT "\n", DYNS_ARG(cmdline));
            system(getStr(cmdline));
        }
        objs[sizeof(srcs)] = NULL;

        clearEntireString(cmdline);
        appendStr(cmdline, C_COMPILER " -o " TARGET_NAME " ");
        for (size_t i = 0; objs[i]; ++i)
        {
            concatFreeString(cmdline, objs[i]);
            appendChar(cmdline, ' ');
        }
        for (size_t i = 0; lib_dirs[i]; ++i)
        {
            appendStr(cmdline, lib_dirs[i]);
            appendChar(cmdline, ' ');
        }
        for (size_t i = 0; libs[i]; ++i)
        {
            appendStr(cmdline, libs[i]);
            appendChar(cmdline, ' ');
        }
        printf(DYNS_FMT "\n", DYNS_ARG(cmdline));
        system(getStr(cmdline));

        if (*is_install)
        {
            printf("mv " TARGET_NAME " " INSTALL_DIR "\n");
            system("mv " TARGET_NAME " " INSTALL_DIR);
        }
        else
        {
            printf("mv " TARGET_NAME " ./build/bin/\n");
            system("mv " TARGET_NAME " ./build/bin/");
        }

        freeString(cmdline);
        free(objs);
    }

    return 0;
}

static String* changeExtension(const char* filename, const char* extension)
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

    if ((location = findCharNth(output, '/', 2)) < 0)
    {
        freeString(output);
        return NULL;
    }

    clearStringBefore(output, location);
    appendStrBack(output, "./build/obj");

    return output;
}
