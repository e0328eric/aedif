#ifndef AEDIF_PROJECT_DATA_H_
#define AEDIF_PROJECT_DATA_H_

#include <lua.h>

#include <dynString.h>

typedef enum LangType
{
    LANG_TYPE_C = 0,
    LANG_TYPE_CPP,
} LangType;

typedef enum StdType
{
    STD_TYPE_C_PLAIN = 0,
    STD_TYPE_C_99,
    STD_TYPE_C_11,
    STD_TYPE_C_14,
    STD_TYPE_C_17,
    STD_TYPE_C_23,
    STD_TYPE_CPP_PLAIN,
    STD_TYPE_CPP_11,
    STD_TYPE_CPP_14,
    STD_TYPE_CPP_17,
    STD_TYPE_CPP_20,
    STD_TYPE_CPP_23,
} StdType;

typedef enum OptLevel
{
    OPT_LEVEL_NO_OPTIMIZE = 0,
    OPT_LEVEL_1,
    OPT_LEVEL_2,
    OPT_LEVEL_3,
    OPT_LEVEL_SIZE,
} OptLevel;

typedef struct ProjectData
{
    LangType language;
    const String* compiler;
    StdType std;
    OptLevel optLevel;
    const String** warnings;
    size_t warningsSize;
    const String** errors;
    size_t errorsSize;
	const String** flags;
	size_t flagsSize;
} ProjectData;

ProjectData getProjectData(const char** output, lua_State* L);
void freeInnerProjectData(ProjectData* pdata);

#endif // AEDIF_PROJECT_DATA_H_
