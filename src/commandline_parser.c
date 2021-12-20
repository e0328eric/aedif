#include <string.h>

#include "commandline_parser.h"

#define USAGE_MSG                                                              \
    "C/C++ Building Tool\n"                                                    \
    "Usage:\n"                                                                 \
    "aedif\n"                                                                  \
    "aedif clean\n"                                                            \
    "aedif install\n"

ExecKind cmdParser(int argc, const char** argv)
{
    if (argc < 2)
    {
        return EXEC_KIND_BUILD;
    }

    if (strncmp(argv[1], "clean", sizeof("clean")) == 0)
    {
        return EXEC_KIND_CLEAN;
    }
    if (strncmp(argv[1], "install", sizeof("install")) == 0)
    {
        return EXEC_KIND_INSTALL;
    }

	return EXEC_KIND_BUILD;
}
