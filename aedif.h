#ifndef AEDIF_H_
#define AEDIF_H_

#include <stdbool.h>

#ifndef COMPILER
#   if defined(_WIN32)
#       define COMPILER "cl"
#   elif defined(__APPLE__)
#       define COMPILER "clang"
#   else
#       define COMPILER "gcc"
#   endif
#endif // COMPILER

#ifndef LINKER
#   if defined(_WIN32)
#       define LINKER "lib"
#   else
#       define LINKER "ar"
#   endif
#endif // LINKER

void makeDir(const char* dir_name);
void cleanBuild(void);
void buildProject(bool is_lib, const char* target_name,
             const char* options, const char** srcs,
             const char** includes, const char** libs,
             const char** lib_dirs, const char* where);

#ifdef AEDIF_IMPLEMENTATION

#include <ctype.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
typedef long long ssize_t;
#else
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

///  dynString.h embed  //////////////////////////////////////////////////
typedef struct String String;

String* mkString(const char* str);
void freeString(String* pString);
void appendChar(String* pString, char chr);
void appendStr(String* pString, const char* str);
void appendFmtStr(struct String* pString, const char* format, ...);
void appendStrBack(String* pString, const char* str);
void concatString(String* dst, const String* src);
void concatFreeString(String* dst, String* src);
ssize_t findCharReverse(const String* pString, char chr);
ssize_t findCharReverseNth(const String* pString, char chr, size_t nth);
void clearStringAfter(String* pString, ssize_t pos);
void clearStringBefore(String* pString, ssize_t pos);
void clearEntireString(String* pString);
const char* getStr(const String* const pString);
size_t getLen(const String* const pString);
size_t getCapacity(const String* const pString);

#define DYNS_FMT "%s"
#define DYNS_ARG(str) (getStr(str))
//  END dynString.h    ///////////////////////////////////////////////////

static String* changeExtension(const char* filename, const char* extension,
                               const char* where);

void makeDir(const char* dir_name)
{
#ifdef _WIN32
    CreateDirectory(dir_name, NULL);
#else
    mkdir(dir_name, 00755);
#endif
}

void cleanBuild(void)
{
    printf("Clean build\n");
#ifdef _WIN32
    system("rd /s /q .\\build");
#else
    system("rm -rf ./build");
#endif
}

void buildProject(bool is_lib, const char* target_name,
             const char* options, const char** srcs,
             const char** includes, const char** libs,
             const char** lib_dirs, const char* where)
{
    String* cmdline = mkString(NULL);
    char* obj_dir; // this value is not deallocated if where == NULL
    if (where == NULL)
    {
        appendFmtStr(cmdline, "./build/obj/%s", target_name);
        obj_dir = malloc(getLen(cmdline) + 1);
        memcpy(obj_dir, getStr(cmdline), getLen(cmdline));
        obj_dir[getLen(cmdline)] = '\0';
    }
    else
    {
        obj_dir = (char*)where;
    }

    makeDir("./build");
    makeDir("./build/obj");
    makeDir("./build/lib");
    makeDir("./build/bin");
    makeDir(obj_dir);

    size_t src_len = 0;
    while (srcs[src_len] != NULL)
    {
        ++src_len;
    }

    clearEntireString(cmdline);
    String** objs = malloc(sizeof(String*) * (src_len + 1));
    objs[src_len] = NULL;

    appendFmtStr(cmdline, COMPILER " %s ", options);

    if (includes)
    {
        for (; *includes; ++includes)
        {
#ifdef _WIN32
            appendFmtStr(cmdline, "/I%s ", *includes);
#else
            appendFmtStr(cmdline, "-I%s ", *includes);
#endif
        }
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
        objs[i] = changeExtension(srcs[i], "obj", obj_dir);
        appendStr(cmdline, " /Fo: ");
#else
        objs[i] = changeExtension(srcs[i], "o", obj_dir);
        appendStr(cmdline, " -o ");
#endif
        concatString(cmdline, objs[i]);
        printf(DYNS_FMT "\n", DYNS_ARG(cmdline));
        system(getStr(cmdline));
    }

    clearEntireString(cmdline);
    if (is_lib)
    {
        appendStr(cmdline, LINKER " ");
#ifdef _WIN32
        appendFmtStr(cmdline, " /out:.build/lib/%s.lib ", target_name);
#else
        appendFmtStr(cmdline, "rcu ./build/lib/%s.a ", target_name);
#endif

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
        appendStr(cmdline, COMPILER " ");
        for (size_t i = 0; objs[i]; ++i)
        {
            concatFreeString(cmdline, objs[i]);
            appendChar(cmdline, ' ');
        }
#ifdef _WIN32
        if (libs)
        {
            for (; *libs; ++libs)
            {
                appendStr(cmdline, *libs);
                appendChar(cmdline, ' ');
            }
        }
        appendFmtStr(cmdline, " /Fe:./build/bin/%s.exe ", target_name);
#else
        appendFmtStr(cmdline, " -o ./build/bin/%s ", target_name);
        if (libs)
        {
            for (; *libs; ++libs)
            {
                appendFmtStr(cmdline, "-l %s ", *libs);
            }
        }
        if (lib_dirs)
        {
            for (; *lib_dirs; ++lib_dirs)
            {
                appendFmtStr(cmdline, "-L %s ", *lib_dirs);
            }
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
    appendFmtStr(output, ".%s", extension);

    if ((location = findCharReverse(output, '/')) < 0)
    {
        freeString(output);
        return NULL;
    }

    clearStringBefore(output, location);
    appendStrBack(output, where);

    return output;
}

///  dynString.h embed  //////////////////////////////////////////////////
struct String
{
	size_t capacity;
	size_t len;
	char* inner;
};

struct String* mkString(const char* str)
{
	struct String* string = malloc(sizeof(struct String));

	if (!str)
	{
		string->capacity = 0;
		string->len = 0;
		string->inner = NULL;
	}
	else
	{
		size_t strLen = strlen(str);

		string->capacity = (strLen + 1) << 1;
		string->len = strLen;
		string->inner = malloc(string->capacity);
		memcpy(string->inner, str, strLen);
		string->inner[strLen] = '\0';
	}

	return string;
}

void freeString(String* pString)
{
	if (!pString)
		return;

	free(pString->inner);
	free(pString);
}

void appendChar(struct String* pString, char chr)
{
	if (pString->len + 1 >= pString->capacity)
	{
		pString->capacity = (pString->capacity + 1) << 1;
		pString->inner = realloc(pString->inner, pString->capacity);
	}
	pString->inner[pString->len++] = chr;
	pString->inner[pString->len] = '\0';
}

void appendStr(struct String* pString, const char* str)
{
	if (!str)
	{
		return;
	}

	size_t strLen = strlen(str);
	if (pString->len + strLen + 1 >= pString->capacity)
	{
		pString->capacity = (pString->capacity + strLen + 1) << 1;
		pString->inner = realloc(pString->inner, pString->capacity);
	}
	memcpy(pString->inner + pString->len, str, strLen);
	pString->inner[pString->len + strLen] = '\0';
	pString->len += strLen;
}

void appendFmtStr(struct String* pString, const char* format, ...)
{
	char* formattedString = NULL;
	ssize_t strLen;
	va_list args;
	va_start(args, format);
	strLen = vasprintf(&formattedString, format, args);
	va_end(args);

	if (strLen < 0)
	{
		return;
	}

	if (pString->len + (size_t)strLen + 1 >= pString->capacity)
	{
		pString->capacity = (pString->capacity + (size_t)strLen + 1) << 1;
		pString->inner = realloc(pString->inner, pString->capacity);
	}
	memcpy(pString->inner + pString->len, formattedString, (size_t)strLen);
	pString->inner[pString->len + (size_t)strLen] = '\0';
	pString->len += (size_t)strLen;

	// Since vasprintf allocates memories, we must free it
	free(formattedString);
}

void appendStrBack(struct String* pString, const char* str)
{
	size_t strLen = strlen(str);
	if (pString->len + strLen + 1 >= pString->capacity)
	{
		pString->capacity = (pString->capacity + strLen + 1) << 1;
		pString->inner = realloc(pString->inner, pString->capacity);
	}
	memmove(pString->inner + strLen, pString->inner, pString->len);
	memcpy(pString->inner, str, strLen);
	pString->inner[pString->len + strLen] = '\0';
	pString->len += strLen;
}

void concatString(struct String* dst, const struct String* src)
{
	if (!dst || !src)
		return;

	if (dst->len + src->len + 1 >= dst->capacity)
	{
		dst->capacity = (dst->capacity + src->len + 1) << 1;
		dst->inner = realloc(dst->inner, dst->capacity);
	}
	memcpy(dst->inner + dst->len, src->inner, src->len);
	dst->inner[dst->len + src->len] = '\0';
	dst->len += src->len;
}

void concatFreeString(struct String* dst, struct String* src)
{
	concatString(dst, src);
	freeString(src);
}

ssize_t findCharReverse(const struct String* pString, char chr)
{
	return findCharReverseNth(pString, chr, 1);
}

ssize_t findCharReverseNth(const String* pString, char chr, size_t nth)
{
	if (!pString)
		return -1;

	char* ptr = pString->inner - 1;
	while (nth--)
	{
		ptr = strrchr(++ptr, chr);
		if (!ptr)
			return -1;
	}

	return ptr - pString->inner;
}

void clearStringAfter(struct String* pString, ssize_t pos)
{
	if (!pString)
		return;

	size_t u_pos = pos < 0 ? pString->len + 1 + (size_t)pos : (size_t)pos;
	memset(pString->inner + u_pos, 0, sizeof(char) * (pString->len - u_pos));
	pString->len = u_pos;
}

void clearStringBefore(struct String* pString, ssize_t pos)
{
	if (!pString)
		return;

	size_t u_pos = pos < 0 ? pString->len + 1 + (size_t)pos : (size_t)pos;
	memmove(pString->inner, pString->inner + u_pos,
			sizeof(char) * (pString->len - u_pos));
	memset(pString->inner + pString->len - u_pos, 0, sizeof(char) * u_pos);
	pString->len -= u_pos;
}

void clearEntireString(struct String* pString)
{
	clearStringAfter(pString, 0);
}

const char* getStr(const String* const pString)
{
	if (!pString)
		return NULL;
	return pString->inner;
}
size_t getLen(const struct String* const pString)
{
	if (!pString)
		return 0;
	return pString->len;
}

size_t getCapacity(const struct String* const pString)
{
	if (!pString)
		return 0;
	return pString->capacity;
}
//  END dynString.h    ///////////////////////////////////////////////////

#endif // AEDIF_IMPLEMENTATION
#endif // AEDIF_H_
