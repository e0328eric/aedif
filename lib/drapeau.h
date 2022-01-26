//
// Copyright (C) 2021  Sungbae Jeong
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
////////////////////////////////////////////////////////////////////////////////
//
// Drapeau Command line parser library v0.1.0
//
// It is a command line parser inspired by go's flag module and tsodings flag.h
// ( tsodings flag.h source code : https://github.com/tsoding/flag.h )
//

#ifndef DRAPEAU_LIBRARY_H_
#define DRAPEAU_LIBRARY_H_

// TODO: Test this library in C++
// #ifdef __cplusplus
// extern "C"
// {
// #endif

#ifndef DRAPEAUDEF
#ifdef DRAPEAU_STATIC
#define DRAPEAUDEF static
#else
#define DRAPEAUDEF extern
#endif // DRAPEAU_STATIC
#endif // DRAPEAUDEF

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/* NOTE: Every flag, subcommand names and descriptions must have static
 * lifetimes
 */
// Function Signatures
DRAPEAUDEF void drapeauStart(const char* restrict name,
                             const char* restrict desc);
DRAPEAUDEF bool drapeauParse(int argc, char** argv, bool allow_empty_arg);
DRAPEAUDEF void drapeauClose(void);
DRAPEAUDEF const char* drapeauGetErr(void);
DRAPEAUDEF void drapeauPrintHelp(FILE* fp);

// clang-format off
#define DRAPEAU_TYPES(T)                                                       \
    T(Bool, bool, boolean, FLAG_TYPE_BOOL)                                     \
    T(I8, int8_t, i8, FLAG_TYPE_I8)                                            \
    T(I16, int16_t, i16, FLAG_TYPE_I16)                                        \
    T(I32, int32_t, i32, FLAG_TYPE_I32)                                        \
    T(I64, int64_t, i64, FLAG_TYPE_I64)                                        \
    T(U8, uint8_t, u8, FLAG_TYPE_U8)                                           \
    T(U16, uint16_t, u16, FLAG_TYPE_U16)                                       \
    T(U32, uint32_t, u32, FLAG_TYPE_U32)                                       \
    T(U64, uint64_t, u64, FLAG_TYPE_U64)                                       \
    T(Str, const char*, str, FLAG_TYPE_STRING)
// clang-format on

DRAPEAUDEF bool* drapeauSubcmd(const char* restrict subcmd_name,
                               const char* restrict desc);

#define T(_name, _type, _foo1, _foo2)                                          \
    DRAPEAUDEF _type* drapeau##_name(const char* restrict flag_name,           \
                                     _type dfault, const char* restrict desc,  \
                                     const char* restrict subcmd);
DRAPEAU_TYPES(T)
#undef T

// #ifdef __cplusplus
// }
// #endif
#endif // DRAPEAU_LIBRARY_H_

/************************/
/* START IMPLEMENTATION */
/************************/
#ifdef DRAPEAU_IMPL

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

// A Flag and a Subcmd struct definitions
typedef enum
{
    FLAG_TYPE_NIL = 0,
    FLAG_TYPE_BOOL,
    FLAG_TYPE_I8,
    FLAG_TYPE_I16,
    FLAG_TYPE_I32,
    FLAG_TYPE_I64,
    FLAG_TYPE_U8,
    FLAG_TYPE_U16,
    FLAG_TYPE_U32,
    FLAG_TYPE_U64,
    FLAG_TYPE_STRING,
} FlagType;

typedef union {
    bool boolean;
    int8_t i8;
    int16_t i16;
    int32_t i32;
    int64_t i64;
    uint8_t u8;
    uint16_t u16;
    uint32_t u32;
    uint64_t u64;
    const char* str;
} FlagKind;

typedef struct
{
    const char* name;
    FlagType type;
    FlagKind kind;
    FlagKind dfault;
    const char* desc;
} Flag;

#ifndef FLAG_CAPACITY
#define FLAG_CAPACITY 256
#endif // FLAG_CAPACITY

typedef struct Subcmd
{
    const char* name;
    const char* desc;
    bool is_activate;
    Flag flags[FLAG_CAPACITY];
    size_t flags_len;
} Subcmd;

#ifndef SUBCOMMAND_CAPACITY
#define SUBCOMMAND_CAPACITY 64
#endif // SUBCOMMAND_CAPACITY

static const char* main_prog_name = NULL;
static const char* main_prog_desc = NULL;
static Subcmd* activated_subcmd = NULL;

static Subcmd subcommands[SUBCOMMAND_CAPACITY];
static size_t subcommands_len = 0;

static Flag main_flags[FLAG_CAPACITY];
static size_t main_flags_len = 0;

// Errorkinds
typedef enum DrapeauErrKind
{
    DRAPEAU_ERR_KIND_OK = 0,
    DRAPEAU_ERR_KIND_SUBCOMMAND_FIND,
    DRAPEAU_ERR_KIND_FLAG_FIND,
    DRAPEAU_ERR_KIND_INAVLID_NUMBER,
} DrapeauErrKind;

static DrapeauErrKind drapeau_err = 0;

// A container of subcommand names (with a hashmap)
// This hashmap made of fnv1a hash algorithm
#define DRAPEAU_HASHMAP_CAPACITY 1024

typedef struct HashBox
{
    const char* name;
    size_t where;
    struct HashBox* next;
} HashBox;

static HashBox hash_map[DRAPEAU_HASHMAP_CAPACITY];

/******************************/
/* Static Function Signatures */
/******************************/
static size_t drapeauHash(const char* letter);
static Flag* drapeauGetFlag(const char* subcmd);
static bool findSubcmdPosition(size_t* output, const char* subcmd_name);
static void freeNextHashBox(HashBox* hashbox);

/************************************/
/* Implementation of Main Functions */
/************************************/
void drapeauStart(const char* restrict name, const char* restrict desc)
{
    main_prog_name = name;
    main_prog_desc = desc;
    memset(hash_map, 0, sizeof(HashBox) * DRAPEAU_HASHMAP_CAPACITY);
}

void drapeauClose(void)
{
    for (size_t i = 0; i < DRAPEAU_HASHMAP_CAPACITY; ++i)
    {
        freeNextHashBox(&hash_map[i]);
    }
}

void drapeauPrintHelp(FILE* fp)
{
    size_t tmp;
    size_t name_len = 0;

    if (main_prog_name == NULL)
    {
        main_prog_name = "(*.*)";
    }

    if (main_prog_desc != NULL)
    {
        fprintf(fp, "%s\n\n", main_prog_desc);
    }

    if (activated_subcmd != NULL)
    {
        fprintf(fp, "Usage: %s %s [FLAGS]\n\n", main_prog_name,
                activated_subcmd->name);
        fprintf(fp, "Options:\n");

        for (size_t i = 0; i < activated_subcmd->flags_len; ++i)
        {
            tmp = strlen(activated_subcmd->flags[i].name);
            name_len = name_len > tmp ? name_len : tmp;
        }

        for (size_t i = 0; i < activated_subcmd->flags_len; ++i)
        {
            fprintf(fp, "    -%*s%s\n", -(int)name_len - 4,
                    activated_subcmd->flags[i].name,
                    activated_subcmd->flags[i].desc);
        }
    }
    else
    {
        fprintf(fp, "Usage: %s [SUBCOMMANDS] [FLAGS]\n\n", main_prog_name);
        fprintf(fp, "Options:\n");

        for (size_t i = 0; i < main_flags_len; ++i)
        {
            tmp = strlen(main_flags[i].name);
            name_len = name_len > tmp ? name_len : tmp;
        }

        for (size_t i = 0; i < main_flags_len; ++i)
        {
            fprintf(fp, "    -%*s%s\n", -(int)name_len - 4, main_flags[i].name,
                    main_flags[i].desc);
        }

        if (subcommands_len > 0)
        {
            fprintf(fp, "\nSubcommands:\n");

            for (size_t i = 0; i < subcommands_len; ++i)
            {
                tmp = strlen(subcommands[i].name);
                name_len = name_len > tmp ? name_len : tmp;
            }

            for (size_t i = 0; i < subcommands_len; ++i)
            {
                fprintf(fp, "    %*s%s\n", -(int)name_len - 4,
                        subcommands[i].name, subcommands[i].desc);
            }
        }
    }
}

// TODO: drapeau only thinks a given string is a flag if it has only one '-'
// letter. For example, -v, -h and -version are considered as a flag, but
// --version, --help are not.
bool drapeauParse(int argc, char** argv, bool allow_empty_arg)
{
    Flag* flags;
    size_t flags_len;
    Flag* flag;
    int arg = 1;

    if (argc < 2)
    {
        if (!allow_empty_arg)
        {
            drapeauPrintHelp(stderr);
            return false;
        }
        else
        {
            return true;
        }
    }

    // check whether has a subcommand
    if (argv[arg][0] != '-')
    {
        size_t pos;
        if (!findSubcmdPosition(&pos, argv[arg++]))
        {
            drapeau_err = DRAPEAU_ERR_KIND_SUBCOMMAND_FIND;
            return false;
        }
        activated_subcmd = &subcommands[pos];
        activated_subcmd->is_activate = true;
        flags = activated_subcmd->flags;
        flags_len = activated_subcmd->flags_len;
    }
    else
    {
        flags = main_flags;
        flags_len = main_flags_len;
    }

    while (arg < argc)
    {
        if (strcmp(argv[arg], "--") == 0)
        {
            ++arg;
            continue;
        }

        if (argv[arg][0] != '-')
        {
            drapeau_err = DRAPEAU_ERR_KIND_FLAG_FIND;
            return false;
        }

        size_t j = 0;
        while (j < flags_len && strcmp(&argv[arg][1], flags[j].name) != 0)
        {
            ++j;
        }

        if (j >= flags_len)
        {
            drapeau_err = DRAPEAU_ERR_KIND_FLAG_FIND;
            return false;
        }

        flag = &flags[j];
        ++arg;

        switch (flag->type)
        {
        case FLAG_TYPE_BOOL:
            flag->kind.boolean = true;
            break;

        case FLAG_TYPE_I8:
            flag->kind.i8 = (int8_t)strtoull(argv[arg++], NULL, 0);
            if (errno == EINVAL || errno == ERANGE)
            {
                drapeau_err = DRAPEAU_ERR_KIND_INAVLID_NUMBER;
                return false;
            }
            break;

        case FLAG_TYPE_I16:
            flag->kind.i16 = (int16_t)strtoull(argv[arg++], NULL, 0);
            if (errno == EINVAL || errno == ERANGE)
            {
                drapeau_err = DRAPEAU_ERR_KIND_INAVLID_NUMBER;
                return false;
            }
            break;

        case FLAG_TYPE_I32:
            flag->kind.i32 = (int32_t)strtoull(argv[arg++], NULL, 0);
            if (errno == EINVAL || errno == ERANGE)
            {
                drapeau_err = DRAPEAU_ERR_KIND_INAVLID_NUMBER;
                return false;
            }
            break;

        case FLAG_TYPE_I64:
            flag->kind.i64 = (int64_t)strtoull(argv[arg++], NULL, 0);
            if (errno == EINVAL || errno == ERANGE)
            {
                drapeau_err = DRAPEAU_ERR_KIND_INAVLID_NUMBER;
                return false;
            }
            break;

        case FLAG_TYPE_U8:
            flag->kind.u8 = (uint8_t)strtoull(argv[arg++], NULL, 0);
            if (errno == EINVAL || errno == ERANGE)
            {
                drapeau_err = DRAPEAU_ERR_KIND_INAVLID_NUMBER;
                return false;
            }
            break;

        case FLAG_TYPE_U16:
            flag->kind.u16 = (uint16_t)strtoull(argv[arg++], NULL, 0);
            if (errno == EINVAL || errno == ERANGE)
            {
                drapeau_err = DRAPEAU_ERR_KIND_INAVLID_NUMBER;
                return false;
            }
            break;

        case FLAG_TYPE_U32:
            flag->kind.u32 = (uint32_t)strtoull(argv[arg++], NULL, 0);
            if (errno == EINVAL || errno == ERANGE)
            {
                drapeau_err = DRAPEAU_ERR_KIND_INAVLID_NUMBER;
                return false;
            }
            break;

        case FLAG_TYPE_U64:
            flag->kind.u64 = (uint64_t)strtoull(argv[arg++], NULL, 0);
            if (errno == EINVAL || errno == ERANGE)
            {
                drapeau_err = DRAPEAU_ERR_KIND_INAVLID_NUMBER;
                return false;
            }
            break;

        case FLAG_TYPE_STRING:
            flag->kind.str = argv[arg++];
            break;

        default:
            assert(false && "Unreatchable(drapeauParse)");
            return false;
        }
    }

    return true;
}

bool* drapeauSubcmd(const char* restrict subcmd_name, const char* restrict desc)
{
    assert(subcommands_len < SUBCOMMAND_CAPACITY);

    HashBox* hash_box;
    Subcmd* subcmd;
    size_t hash = drapeauHash(subcmd_name);

    if (hash_map[hash].next != NULL)
    {
        hash_box = hash_map[hash].next;
        while (hash_box->next != NULL)
        {
            hash_box = hash_box->next;
        }
    }
    else
    {
        hash_box = &hash_map[hash];
    }

    hash_box->name = subcmd_name;
    hash_box->where = subcommands_len;
    hash_box->next = malloc(sizeof(HashBox));
    hash_box->next->next = NULL;

    subcmd = &subcommands[subcommands_len++];

    subcmd->name = subcmd_name;
    subcmd->desc = desc;
    subcmd->is_activate = false;
    subcmd->flags_len = 0;

    return &subcmd->is_activate;
}

#define T(_name, _type, _arg, _flag_type)                                      \
    _type* drapeau##_name(const char* restrict flag_name, _type dfault,        \
                          const char* restrict desc,                           \
                          const char* restrict subcmd)                         \
    {                                                                          \
        Flag* flag = drapeauGetFlag(subcmd);                                   \
        if (flag == NULL)                                                      \
        {                                                                      \
            drapeau_err = DRAPEAU_ERR_KIND_SUBCOMMAND_FIND;                    \
            return NULL;                                                       \
        }                                                                      \
                                                                               \
        flag->name = flag_name;                                                \
        flag->type = _flag_type;                                               \
        flag->kind._arg = dfault;                                              \
        flag->dfault._arg = dfault;                                            \
        flag->desc = desc;                                                     \
                                                                               \
        return &flag->kind._arg;                                               \
    }

// implementation of drapeauBool kinds
DRAPEAU_TYPES(T)
#undef T

// TODO: implement better and clean error printing message
const char* drapeauGetErr(void)
{
    switch (drapeau_err)
    {
    case DRAPEAU_ERR_KIND_OK:
        return NULL;

    case DRAPEAU_ERR_KIND_SUBCOMMAND_FIND:
        return "Cannot find an appropriate subcommnads";

    case DRAPEAU_ERR_KIND_FLAG_FIND:
        return "Cannot find an appropriate flag";

    case DRAPEAU_ERR_KIND_INAVLID_NUMBER:
        return "Invalid number or overflowed number is given";

    default:
        assert(false && "Unreatchable (drapeauGetErr)");
        return "";
    }
}

/************************************/
/* Static Functions Implementations */
/************************************/
static Flag* drapeauGetFlag(const char* restrict subcmd)
{
    Flag* flag;

    if (subcmd != NULL)
    {
        size_t pos;
        if (!findSubcmdPosition(&pos, subcmd))
        {
            return NULL;
        }

        size_t* idx = &subcommands[pos].flags_len;
        assert(*idx < FLAG_CAPACITY);

        flag = &subcommands[pos].flags[(*idx)++];
    }
    else
    {
        assert(main_flags_len < FLAG_CAPACITY);
        flag = &main_flags[main_flags_len++];
    }

    return flag;
}

static size_t drapeauHash(const char* letter)
{
    uint32_t hash = 0x811c9dc5;
    const uint32_t prime = 16777619;
    while (*letter)
    {
        hash = ((uint32_t)*letter++ ^ hash) * prime;
    }
    return hash ^ (hash >> 10) << 10;
}

static bool findSubcmdPosition(size_t* output, const char* subcmd_name)
{
    size_t hash = drapeauHash(subcmd_name);
    HashBox* hashbox = &hash_map[hash];

    if (hashbox->name == NULL)
    {
        return false;
    }

    while (hashbox != NULL && strcmp(subcmd_name, hashbox->name) != 0)
    {
        hashbox = hashbox->next;
    }

    if (hashbox == NULL)
    {
        return false;
    }

    *output = hashbox->where;
    return true;
}

static void freeNextHashBox(HashBox* hashbox)
{
    if (hashbox == NULL)
    {
        return;
    }

    HashBox* next = hashbox->next;
    while (next != NULL)
    {
        hashbox = next;
        next = next->next;
        free(hashbox);
    }
}

#endif // DRAPEAU_IMPL
