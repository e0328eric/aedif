#ifndef AEDIF_ERR_PRINT_SYNTAX_H_
#define AEDIF_ERR_PRINT_SYNTAX_H_

#define AEDIF_INTERNAL_ERR_PREFIX                                              \
    "\x1b[1m\x1b[97;41m[aedif internal error]:\x1b[0m "
#define AEDIF_ERROR_PREFIX "\x1b[1m\x1b[91m[aedif error]:   \x1b[0m"
#define AEDIF_WARN_PREFIX "\x1b[1m\x1b[93m[aedif warning]: \x1b[0m"
#define AEDIF_NOTE_PREFIX "\x1b[1m\x1b[96m[aedif note]:    \x1b[0m"

#endif // AEDIF_ERR_PRINT_SYNTAX_H_
