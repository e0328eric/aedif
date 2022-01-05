#ifndef AEDIF_ERR_PRINT_SYNTAX_H_
#define AEDIF_ERR_PRINT_SYNTAX_H_

#define AEDIF_INTERNAL_ERR_PREFIX                                              \
    "\033[1m\033[97;41m[aedif internal error]:\033[0m "
#define AEDIF_ERROR_PREFIX "\033[1m\033[91m[aedif error]:   \033[0m"
#define AEDIF_WARN_PREFIX "\033[1m\033[93m[aedif warning]: \033[0m"
#define AEDIF_NOTE_PREFIX "\033[1m\033[96m[aedif note]:    \033[0m"
#define AEDIF_PADDING_PREFIX "                 "

#endif // AEDIF_ERR_PRINT_SYNTAX_H_
