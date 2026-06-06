/*
 * navajoPort.h --
 *
 * Copyright (c) 2008-2026 by Joe Mistachkin.  All rights reserved.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * written by: Joe Mistachkin
 *
 * RCS: @(#) $Id: $
 */

#if defined(_MSC_VER) && defined(WIN32) && !defined(_WIN32_WCE) && !defined(_INC_STDDEF)
#error "The header file <stddef.h> must be included prior to this file."
#endif

#ifndef _NAVAJO_PORT_H_
#define _NAVAJO_PORT_H_

/*****************************************************************************/

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
    /*
     * NOTE: Use the new C99 fixed-width integer types when they are available.
     */

    #include <stdint.h>
#endif /* defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L */

/*****************************************************************************/

#if defined(_MSC_VER) || defined(__MINGW32__)
    /*
     * NOTE: Create an alias for the name of this function in the POSIX
     *       standard.
     */

    #define snprintf _snprintf

    /*
     * NOTE: Use the Microsoft Visual C++ specific format specifier for 64-bit
     *       integers, both signed and unsigned.
     */

    #define INT64_FORMAT "%I64d"
    #define UINT64_FORMAT "%I64u"
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
    /*
     * NOTE: Prefer the C99 <inttypes.h> format-string macros because the
     *       underlying type of INT64/UINT64 (int64_t/uint64_t) is
     *       platform dependent: on LP64 systems such as 64-bit Linux it
     *       is 'long', whereas on LLP64 systems it is 'long long'.  Using
     *       PRId64/PRIu64 picks the matching specifier automatically.
     */

    #include <inttypes.h>

    #define INT64_FORMAT "%" PRId64
    #define UINT64_FORMAT "%" PRIu64
#else
    /*
     * NOTE: Pre-C99 fallback; assumes 'long long' is the underlying type.
     */

    #define INT64_FORMAT "%lld"
    #define UINT64_FORMAT "%llu"
#endif /* defined(_MSC_VER) || defined(__MINGW32__) */

/*****************************************************************************/

#if defined(WIN32) || defined(_WIN32_WCE)
    /*
     * NOTE: To maintain portability to this platform (Windows CE), we need to
     *       define various things from the Windows headers.
     */

    #include "windows.h"

    /*
     * NOTE: Define a mutex type for use internally and by the public API.  For
     *       now, just use critical sections on Windows.  Eventually, a higher
     *       performance alternative may be needed.
     */

    typedef CRITICAL_SECTION ACK_MUTEX, *ACK_LPMUTEX;
#else
    /*
     * HACK: Define a mutex type for use internally and by the public API.  For
     *       now, just use an integer as a dummy field for other platforms.
     *       Eventually, we need to figure out the magic to get pthreads to
     *       work properly on all of our other supported platforms.
     */

    typedef int ACK_MUTEX, *ACK_LPMUTEX;
#endif /* defined(WIN32) || defined(_WIN32_WCE) */

/*****************************************************************************/

/*
 * NOTE: Make the Windows CE build environment comply with the ANSI C
 *       standard.  The Windows CE C runtime omits abort() from
 *       <stdlib.h>, EINVAL and errno_t from <errno.h>, and intptr_t
 *       from <stddef.h>; the wce_stdlib.h compat header provides
 *       drop-in substitutes for all four so that both the SDK and the
 *       SQLite amalgamation will compile on WCE.  The same header is
 *       force-included into the SQLite WCE build via the
 *       SQLITE_WCE_STDLIB_FORCE_INCLUDE macro in sqlite3.vsprops, so
 *       there is a single source of truth for the substitute
 *       definitions.
 */
#if defined(_WIN32_WCE)
    #include "compat/wce_stdlib.h"
#else
    /*
     * NOTE: Include the ANSI C header file for the global variable
     *       errno.
     */

    #include <errno.h>
#endif /* defined(_WIN32_WCE) */

/*****************************************************************************/

/*
 * NOTE: All supported non-Windows platforms are assumed to be POSIX compliant.
 */
#if (!defined(WIN32) && !defined(_WIN32_WCE)) || defined(__MINGW32__)
    /*
     * NOTE: We need the "stdio.h" header for the "FILENAME_MAX" constant.
     */

    #include <stdio.h>

    /*
     * NOTE: We need the "MAX_PATH" constant for Windows portability support.
     *       This is provided for use by external (i.e. user) code only.
     */

    #ifndef MAX_PATH
        #ifdef FILENAME_MAX
            #define MAX_PATH (FILENAME_MAX)
        #else
            #define MAX_PATH (260)
        #endif
    #endif

    /*
     * NOTE: We need the "ftruncate" function, which is POSIX.  The header file
     *       "navajoInt.h" relies on the _POSIX_VERSION define being present to
     *       enable its use.
     */

    #include <sys/unistd.h>

    /*
     * NOTE: We need the "NAME_MAX" define from "limits.h" (if present).
     */

    #include <limits.h>

    /*
     * NOTE: We need to define "NAME_MAX" if it was not present in "limits.h".
     */

    #ifndef NAME_MAX
        #ifdef FILENAME_MAX
            #define NAME_MAX (FILENAME_MAX)
        #else
            #define NAME_MAX (260)
        #endif
    #endif

    /*
     * NOTE: We need the "struct dirent" and "DIR" POSIX types.
     */

    #if (defined(_POSIX_VERSION) && _POSIX_VERSION >= 200112L) || \
            defined(HAVE_DIRENT_H)
        /*
         * NOTE: The "dirent.h" header file is part of the POSIX standard and
         *       we need it now.
         */

        #include <dirent.h>

        typedef struct dirent DIRENT;
    #else
        typedef void *DIRENT;
        typedef void *DIR;
    #endif /* _POSIX_VERSION >= 200112L || defined(HAVE_DIRENT_H) */
#else
    /*
     * NOTE: We may need to provide the "ino_t" type on Windows.
     */

    #ifndef INO_T_DEFINED
        #define INO_T_DEFINED

        typedef unsigned short ino_t;
    #endif

    /*
     * NOTE: We need the "NAME_MAX" define from "limits.h" (if present).
     */

    #include <limits.h>

    /*
     * NOTE: We need to define "NAME_MAX" if it was not present in "limits.h".
     */

    #ifndef NAME_MAX
        #ifdef FILENAME_MAX
            #define NAME_MAX (FILENAME_MAX)
        #else
            #define NAME_MAX (260)
        #endif
    #endif

    /*
     * NOTE: We need to define "NULL_INTPTR_T" and "BAD_INTPTR_T" on Windows.
     */

    #ifndef NULL_INTPTR_T
        #define NULL_INTPTR_T ((intptr_t)(0))
    #endif

    #ifndef BAD_INTPTR_T
        #define BAD_INTPTR_T ((intptr_t)(-1))
    #endif

    #ifndef FILE_ATTRIBUTE_DIRECTORY
        #define FILE_ATTRIBUTE_DIRECTORY (0x00000010)
    #endif

    /*
     * NOTE: We need to provide the "DIRENT" structure and related types on
     *       Windows.
     */

    typedef struct DIRENT_tag {
        ino_t d_ino;               /* Not portable, do not use. */
        unsigned d_attributes;     /* Not portable, do not use. */
        char d_name[NAME_MAX + 1]; /* The name within the directory. */
    } DIRENT;

    /*
     * NOTE: We need to provide the "DIR" and "LPDIR" types on Windows.
     */

    typedef struct DIR_tag {
        intptr_t d_handle; /* The value returned by "_findfirst". */
        DIRENT d_first;    /* The DIRENT constructed based on "_findfirst". */
        DIRENT d_next;     /* The DIRENT constructed based on "_findnext". */
    } DIR;
#endif /* (!defined(WIN32) && !defined(_WIN32_WCE)) || defined(__MINGW32__) */

/*****************************************************************************/

/*
 * HACK: Make the S60 build environment play nice with the ANSI C standard and
 *       the conventions established on other platforms.
 */
#if defined(__SYMBIAN32__)
    /*
     * NOTE: We need the "stdio.h" header for the "FILENAME_MAX" constant.
     */

    #include <stdio.h>

    /*
     * NOTE: We need the "MAX_PATH" constant for Windows portability support.
     *       This is provided for use by external (i.e. user) code only.
     */

    #ifndef MAX_PATH
        #ifdef FILENAME_MAX
            #define MAX_PATH (FILENAME_MAX)
        #else
            #define MAX_PATH (260)
        #endif
    #endif

    /*
     * NOTE: We need this and it should be defined in <dirent.h>
     */

    #ifndef NAME_MAX
        #define NAME_MAX (MAXNAMLEN)
    #endif

    /*
     * NOTE: We need this and it should be defined in <errno.h>.
     */

    typedef int errno_t;

    /*
     * NOTE: The "__int32" type does not appear to be available.  Use the one
     *       provided by Symbian.  The "__int64" type is available, so leave
     *       it alone.
     */

    #include "e32def.h"

    typedef TInt32 INT32;
    typedef TUint32 UINT32;

    #define _INT32_DEFINED
    #define _UINT32_DEFINED
#endif /* defined(__SYMBIAN32__) */

/*****************************************************************************/

/*
 * Utility macros: STRINGIFY takes an argument and wraps it in "" (double
 * quotation marks), JOIN joins two arguments.
 */

#ifndef STRINGIFY
    #define STRINGIFY(x) STRINGIFY1(x)
    #define STRINGIFY1(x) #x
#endif

#ifndef JOIN
    #define JOIN(a,b) JOIN1(a,b)
    #define JOIN1(a,b) a##b
#endif

/*****************************************************************************/

#endif /* _NAVAJO_PORT_H_ */

/* end of file */
