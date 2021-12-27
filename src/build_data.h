#ifndef AEDIF_BUILD_DATA_H_
#define AEDIF_BUILD_DATA_H_

#include <lauxlib.h>
#include <lua.h>

#include "project_data.h"

typedef enum BuildType
{
    BUILD_TYPE_BINARY = 0,
    BUILD_TYPE_STATIC,
    BUILD_TYPE_DYNAMIC,
} BuildType;

typedef struct BuildData
{
    const char* targetName;
    const char** srcs;
    size_t srcsSize;
    const char** libs;
    size_t libsSize;
    const char** libDirs;
    size_t libDirsSize;
    const char** includes;
    size_t includesSize;
    BuildType buildType;
} BuildData;

BuildData newBuildData(lua_State* L, const char** err_msg);
void freeInnerBuildData(BuildData* bdata);

#endif // AEDIF_BUILD_DATA_H_
