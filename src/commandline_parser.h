#ifndef AEDIF_COMMAND_LINE_PARSER_H_
#define AEDIF_COMMAND_LINE_PARSER_H_

typedef enum ExecKind
{
    EXEC_KIND_BUILD = 0,
    EXEC_KIND_CLEAN,
    EXEC_KIND_INSTALL,
} ExecKind;

ExecKind cmdParser(int argc, const char** argv);

#endif // AEDIF_COMMAND_LINE_PARSER_H_
