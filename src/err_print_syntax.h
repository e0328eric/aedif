/*
 * Copyright (C) 2021  Sungbae Jeong
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/*
 * err_print_syntax.h Header
 *
 * The purpose of this header is to make an appropriate aedif error, warning or
 * note prefix align and formatting
 */

#ifndef AEDIF_ERR_PRINT_SYNTAX_H_
#define AEDIF_ERR_PRINT_SYNTAX_H_

#define AEDIF_INTERNAL_ERR_PREFIX                                              \
    "\x1b[1m\x1b[97;41m[aedif internal error]:\x1b[0m "
#define AEDIF_ERROR_PREFIX "\x1b[1m\x1b[91m[aedif error]:   \x1b[0m"
#define AEDIF_WARN_PREFIX "\x1b[1m\x1b[93m[aedif warning]: \x1b[0m"
#define AEDIF_NOTE_PREFIX "\x1b[1m\x1b[96m[aedif note]:    \x1b[0m"

#endif // AEDIF_ERR_PRINT_SYNTAX_H_
