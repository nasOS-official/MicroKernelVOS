/*===---- stdarg.h - Variable argument handling ----------------------------===
 *
 * Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
 * See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 */

/*
 * This header is designed to be included multiple times. If any of the __need_
 * macros are defined, then only that subset of interfaces are provided. This
 * can be useful for POSIX headers that need to not expose all of stdarg.h, but
 * need to use some of its interfaces. Otherwise this header provides all of
 * the expected interfaces.
 *
 * When clang modules are enabled, this header is a textual header to support
 * the multiple include behavior. As such, it doesn't directly declare anything
 * so that it doesn't add duplicate declarations to all of its includers'
 * modules.
 */
#if defined(__MVS__) && __has_include_next(<stdarg.h>)
#undef __need___va_list
#undef __need_va_list
#undef __need_va_arg
#undef __need___va_copy
#undef __need_va_copy
#include <__stdarg_header_macro.h>
#include_next <stdarg.h>

#else
#if !defined(__need___va_list) && !defined(__need_va_list) &&                  \
    !defined(__need_va_arg) && !defined(__need___va_copy) &&                   \
    !defined(__need_va_copy)
#define __need___va_list
#define __need_va_list
#define __need_va_arg
#define __need___va_copy
/* GCC always defines __va_copy, but does not define va_copy unless in c99 mode
 * or -ansi is not specified, since it was not part of C90.
 */
#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) ||              \
    (defined(__cplusplus) && __cplusplus >= 201103L) ||                        \
    !defined(__STRICT_ANSI__)
#define __need_va_copy
#endif
#ifndef __STDARG_H
#define __STDARG_H
#endif
#endif

#ifdef __need___va_list
#ifndef __GNUC_VA_LIST
#define __GNUC_VA_LIST
typedef __builtin_va_list __gnuc_va_list;
#endif
#undef __need___va_list
#endif /* defined(__need___va_list) */

#ifdef __need_va_list
#ifndef _VA_LIST
#define _VA_LIST
typedef __builtin_va_list va_list;
#endif
#undef __need_va_list
#endif /* defined(__need_va_list) */

#ifdef __need_va_arg
#ifndef va_arg

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
/* C23 uses a special builtin. */
#define va_start(...) __builtin_c23_va_start(__VA_ARGS__)
#else
/* Versions before C23 do require the second parameter. */
#define va_start(ap, param) __builtin_va_start(ap, param)
#endif
#define va_end(ap) __builtin_va_end(ap)
#define va_arg(ap, type) __builtin_va_arg(ap, type)

#endif
#undef __need_va_arg
#endif /* defined(__need_va_arg) */

#ifdef __need___va_copy
#ifndef __va_copy
#define __va_copy(d, s) __builtin_va_copy(d, s)
#endif
#undef __need___va_copy
#endif /* defined(__need___va_copy) */

#ifdef __need_va_copy
#ifndef va_copy
#define va_copy(dest, src) __builtin_va_copy(dest, src)
#endif
#undef __need_va_copy
#endif /* defined(__need_va_copy) */

#endif /* __MVS__ */
