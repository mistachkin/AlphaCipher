/*
 * stdint.h -- Minimal C99 <stdint.h> for Microsoft Visual C++ 2008.
 *
 * Copyright (c) 2026 by Joe Mistachkin.  All rights reserved.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * written by: Joe Mistachkin
 *
 * RCS: @(#) $Id: $
 *
 * --------------------------------------------------------------------------
 *
 * Background
 * ----------
 *
 * The Visual C++ 2008 toolchain (cl.exe v9.00, _MSC_VER == 1500) predates
 * the addition of a C99 <stdint.h> header to the Microsoft Windows SDK.
 * Visual C++ 2010 and later (_MSC_VER >= 1600) ship a real <stdint.h>
 * inside the platform SDK, so this header is needed only for the VS2008
 * build configuration.
 *
 * Recent SQLite amalgamations include <stdint.h> unconditionally; the
 * VS2008 build therefore fails to compile sqlite3.c without a shim.
 * This header provides the minimum set of typedefs, limit macros, and
 * integer-constant macros that the C99 standard mandates and that SQLite
 * (and well-behaved third-party C code generally) consumes.
 *
 * Usage
 * -----
 *
 * Add the parent directory of this file to AdditionalIncludeDirectories
 * for the VS2008 project configurations ONLY:
 *
 *     ..\..\..\Common\Shared\include\compat
 *
 * Do NOT place this directory on the include path for VS2010+ or for
 * non-MSVC builds (GCC, Clang, MinGW): those toolchains ship a real
 * <stdint.h> and this stub would otherwise shadow it.  As a safety net
 * the body of this header is wrapped in an _MSC_VER < 1600 guard so it
 * is a no-op when accidentally included by a newer toolchain.
 *
 * --------------------------------------------------------------------------
 */

#ifndef _COMPAT_STDINT_H_
#define _COMPAT_STDINT_H_

#if defined(_MSC_VER) && (_MSC_VER < 1600)

/*
 * NOTE: <stddef.h> defines ptrdiff_t / size_t (used below for SIZE_MAX
 *       and PTRDIFF_MIN/MAX).  It also defines intptr_t / uintptr_t on
 *       MSVC via _INTPTR_T_DEFINED / _UINTPTR_T_DEFINED.
 */

#include <stddef.h>

/* ---------------------------------------------------------------- */
/* 7.18.1.1  Exact-width integer types                              */
/* ---------------------------------------------------------------- */

#ifndef _INT8_T_DEFINED
#define _INT8_T_DEFINED
typedef signed   __int8  int8_t;
typedef unsigned __int8  uint8_t;
#endif

#ifndef _INT16_T_DEFINED
#define _INT16_T_DEFINED
typedef signed   __int16 int16_t;
typedef unsigned __int16 uint16_t;
#endif

#ifndef _INT32_T_DEFINED
#define _INT32_T_DEFINED
typedef signed   __int32 int32_t;
typedef unsigned __int32 uint32_t;
#endif

#ifndef _INT64_T_DEFINED
#define _INT64_T_DEFINED
typedef signed   __int64 int64_t;
typedef unsigned __int64 uint64_t;
#endif

/* ---------------------------------------------------------------- */
/* 7.18.1.2  Minimum-width integer types                            */
/*           (At least N bits wide.  Exact match is fine.)          */
/* ---------------------------------------------------------------- */

typedef int8_t   int_least8_t;
typedef uint8_t  uint_least8_t;
typedef int16_t  int_least16_t;
typedef uint16_t uint_least16_t;
typedef int32_t  int_least32_t;
typedef uint32_t uint_least32_t;
typedef int64_t  int_least64_t;
typedef uint64_t uint_least64_t;

/* ---------------------------------------------------------------- */
/* 7.18.1.3  Fastest minimum-width integer types                    */
/*           (Exact match satisfies the standard's minimum.)        */
/* ---------------------------------------------------------------- */

typedef int8_t   int_fast8_t;
typedef uint8_t  uint_fast8_t;
typedef int16_t  int_fast16_t;
typedef uint16_t uint_fast16_t;
typedef int32_t  int_fast32_t;
typedef uint32_t uint_fast32_t;
typedef int64_t  int_fast64_t;
typedef uint64_t uint_fast64_t;

/* ---------------------------------------------------------------- */
/* 7.18.1.4  Pointer-sized integer types                            */
/*                                                                  */
/* On MSVC <stddef.h> already typedefs intptr_t and uintptr_t when  */
/* _INTPTR_T_DEFINED / _UINTPTR_T_DEFINED are not yet set.  We      */
/* respect those guards rather than redefining.                     */
/* ---------------------------------------------------------------- */

#ifndef _INTPTR_T_DEFINED
#define _INTPTR_T_DEFINED
#if defined(_WIN64) || defined(_M_X64) || defined(_M_IA64) || defined(_M_ARM64)
typedef signed   __int64 intptr_t;
#else
typedef signed   int     intptr_t;
#endif
#endif

#ifndef _UINTPTR_T_DEFINED
#define _UINTPTR_T_DEFINED
#if defined(_WIN64) || defined(_M_X64) || defined(_M_IA64) || defined(_M_ARM64)
typedef unsigned __int64 uintptr_t;
#else
typedef unsigned int     uintptr_t;
#endif
#endif

/* ---------------------------------------------------------------- */
/* 7.18.1.5  Greatest-width integer types                           */
/* ---------------------------------------------------------------- */

typedef int64_t  intmax_t;
typedef uint64_t uintmax_t;

/* ---------------------------------------------------------------- */
/* 7.18.2  Limits of integer types                                  */
/*                                                                  */
/* Per the standard these are visible only when                     */
/* __STDC_LIMIT_MACROS is defined before #include in C++ code.      */
/* SQLite and most C code want them unconditionally, so we emit     */
/* them in C and gate on the macro only for C++.                    */
/* ---------------------------------------------------------------- */

#if !defined(__cplusplus) || defined(__STDC_LIMIT_MACROS)

/* 7.18.2.1  Exact-width integer types */

#ifndef INT8_MIN
#define INT8_MIN   (-127i8 - 1)
#endif
#ifndef INT16_MIN
#define INT16_MIN  (-32767i16 - 1)
#endif
#ifndef INT32_MIN
#define INT32_MIN  (-2147483647i32 - 1)
#endif
#ifndef INT64_MIN
#define INT64_MIN  (-9223372036854775807i64 - 1)
#endif

#ifndef INT8_MAX
#define INT8_MAX   127i8
#endif
#ifndef INT16_MAX
#define INT16_MAX  32767i16
#endif
#ifndef INT32_MAX
#define INT32_MAX  2147483647i32
#endif
#ifndef INT64_MAX
#define INT64_MAX  9223372036854775807i64
#endif

#ifndef UINT8_MAX
#define UINT8_MAX  0xffui8
#endif
#ifndef UINT16_MAX
#define UINT16_MAX 0xffffui16
#endif
#ifndef UINT32_MAX
#define UINT32_MAX 0xffffffffui32
#endif
#ifndef UINT64_MAX
#define UINT64_MAX 0xffffffffffffffffui64
#endif

/* 7.18.2.2  Minimum-width integer types */

#ifndef INT_LEAST8_MIN
#define INT_LEAST8_MIN    INT8_MIN
#define INT_LEAST16_MIN   INT16_MIN
#define INT_LEAST32_MIN   INT32_MIN
#define INT_LEAST64_MIN   INT64_MIN
#define INT_LEAST8_MAX    INT8_MAX
#define INT_LEAST16_MAX   INT16_MAX
#define INT_LEAST32_MAX   INT32_MAX
#define INT_LEAST64_MAX   INT64_MAX
#define UINT_LEAST8_MAX   UINT8_MAX
#define UINT_LEAST16_MAX  UINT16_MAX
#define UINT_LEAST32_MAX  UINT32_MAX
#define UINT_LEAST64_MAX  UINT64_MAX
#endif

/* 7.18.2.3  Fastest minimum-width integer types */

#ifndef INT_FAST8_MIN
#define INT_FAST8_MIN     INT8_MIN
#define INT_FAST16_MIN    INT16_MIN
#define INT_FAST32_MIN    INT32_MIN
#define INT_FAST64_MIN    INT64_MIN
#define INT_FAST8_MAX     INT8_MAX
#define INT_FAST16_MAX    INT16_MAX
#define INT_FAST32_MAX    INT32_MAX
#define INT_FAST64_MAX    INT64_MAX
#define UINT_FAST8_MAX    UINT8_MAX
#define UINT_FAST16_MAX   UINT16_MAX
#define UINT_FAST32_MAX   UINT32_MAX
#define UINT_FAST64_MAX   UINT64_MAX
#endif

/* 7.18.2.4  Integer types capable of holding object pointers */

#ifndef INTPTR_MIN
#if defined(_WIN64) || defined(_M_X64) || defined(_M_IA64) || defined(_M_ARM64)
#define INTPTR_MIN   INT64_MIN
#define INTPTR_MAX   INT64_MAX
#define UINTPTR_MAX  UINT64_MAX
#else
#define INTPTR_MIN   INT32_MIN
#define INTPTR_MAX   INT32_MAX
#define UINTPTR_MAX  UINT32_MAX
#endif
#endif

/* 7.18.2.5  Greatest-width integer types */

#ifndef INTMAX_MIN
#define INTMAX_MIN   INT64_MIN
#endif
#ifndef INTMAX_MAX
#define INTMAX_MAX   INT64_MAX
#endif
#ifndef UINTMAX_MAX
#define UINTMAX_MAX  UINT64_MAX
#endif

/* 7.18.3  Limits of other integer types */

#ifndef PTRDIFF_MIN
#if defined(_WIN64) || defined(_M_X64) || defined(_M_IA64) || defined(_M_ARM64)
#define PTRDIFF_MIN  INT64_MIN
#define PTRDIFF_MAX  INT64_MAX
#else
#define PTRDIFF_MIN  INT32_MIN
#define PTRDIFF_MAX  INT32_MAX
#endif
#endif

#ifndef SIZE_MAX
#if defined(_WIN64) || defined(_M_X64) || defined(_M_IA64) || defined(_M_ARM64)
#define SIZE_MAX     UINT64_MAX
#else
#define SIZE_MAX     UINT32_MAX
#endif
#endif

#ifndef SIG_ATOMIC_MIN
#define SIG_ATOMIC_MIN  INT32_MIN
#define SIG_ATOMIC_MAX  INT32_MAX
#endif

#ifndef WCHAR_MIN
#define WCHAR_MIN    0
#define WCHAR_MAX    0xffff
#endif

#ifndef WINT_MIN
#define WINT_MIN     0
#define WINT_MAX     0xffff
#endif

#endif /* !defined(__cplusplus) || defined(__STDC_LIMIT_MACROS) */

/* ---------------------------------------------------------------- */
/* 7.18.4  Macros for integer constants                             */
/*                                                                  */
/* Same C++ visibility rule as the limit macros above.              */
/* ---------------------------------------------------------------- */

#if !defined(__cplusplus) || defined(__STDC_CONSTANT_MACROS)

#ifndef INT8_C
#define INT8_C(val)   val##i8
#define INT16_C(val)  val##i16
#define INT32_C(val)  val##i32
#define INT64_C(val)  val##i64

#define UINT8_C(val)  val##ui8
#define UINT16_C(val) val##ui16
#define UINT32_C(val) val##ui32
#define UINT64_C(val) val##ui64

#define INTMAX_C(val)  INT64_C(val)
#define UINTMAX_C(val) UINT64_C(val)
#endif

#endif /* !defined(__cplusplus) || defined(__STDC_CONSTANT_MACROS) */

#endif /* defined(_MSC_VER) && (_MSC_VER < 1600) */

#endif /* _COMPAT_STDINT_H_ */

/* end of file */
