/*
 * os_ansi.c --
 *
 * Copyright (c) 2009-2026 by Joe Mistachkin.  All rights reserved.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * written by: Joe Mistachkin
 *
 * RCS: @(#) $Id: $
 */

#if defined(ACK_OS_ANSI)

#if defined(HAVE_ERRNO)
#include <errno.h>
#endif

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "navajoPort.h"
#include "navajo.h"
#include "navajoSqlite.h"
#include "navajoIntTypes.h"
#include "navajoInt.h"
#include "os.h"
#include "os_ansi.h"

#ifndef INT_FORMAT
#define INT_FORMAT "%d"
#endif

ACK_OS ansiOs = {
    "ansi",      /* zName */
    NULL,        /* pAppData */
    NULL,        /* xInitialize */
    NULL,        /* xFinalize */
    NULL,        /* xInitializeMutex */
    NULL,        /* xFinalizeMutex */
    NULL,        /* xLockMutex */
    NULL,        /* xUnlockMutex */
    ansiErrno,   /* xErrno */
    ansiMalloc,  /* xMalloc */
    ansiZalloc,  /* xZalloc */
    ansiRealloc, /* xRealloc */
    NULL,        /* xMsize */
    ansiFree,    /* xFree */
    NULL,        /* xGetcwd */
    NULL,        /* xOpendir */
    NULL,        /* xReaddir */
    NULL,        /* xClosedir */
    NULL,        /* xIsdir */
    ansiAtoull,  /* xAtoull */
    ansiItoa,    /* xItoa */
    ansiI64toa,  /* xI64toa */
    ansiUi64toa, /* xUi64toa */
    ansiStrcat,  /* xStrcat */
    ansiStrdup,  /* xStrdup */
    ansiFseek,   /* xFseek */
    ansiFtell,   /* xFtell */
    NULL         /* xFtruncate */
};

INT ansiErrno( /* PRIVATE */
    )
{
#if defined(HAVE_ERRNO)
    return errno;
#else
    return 0;
#endif
}

LPVOID ansiMalloc( /* PRIVATE */
    SIZE_T size
    )
{
    return malloc(size);
}

LPVOID ansiZalloc( /* PRIVATE */
    SIZE_T size
    )
{
    LPVOID pBlock = malloc(size);

    if (pBlock != NULL)
        memset(pBlock, 0, size);

    return pBlock;
}

LPVOID ansiRealloc( /* PRIVATE */
    LPVOID pBlock,
    SIZE_T size,
    BOOL zero
    )
{
    LPVOID pNewBlock = realloc(pBlock, size);

    if (zero && (pNewBlock != NULL))
        memset(pNewBlock, 0, size);

    return pNewBlock;
}

VOID ansiFree( /* PRIVATE */
    LPVOID pBlock
    )
{
    if (pBlock != NULL)
        free(pBlock);
}

UINT64 ansiAtoull( /* PRIVATE */
    LPCSTR str
    )
{
#if defined(HAVE_ATOLL)
    return (UINT64)atoll(str);
#else
    return 0;
#endif
}

LPSTR ansiItoa( /* PRIVATE */
    INT val
    )
{
    /*
     * BUGBUG: This is not thread-safe (static buffer).
     */

    static CHAR buffer[MAX_INTEGER_SPACE + 1];

    snprintf(buffer, MAX_INTEGER_SPACE, INT_FORMAT, val);
    return buffer;
}

LPSTR ansiI64toa( /* PRIVATE */
    INT64 val
    )
{
    /*
     * BUGBUG: This is not thread-safe (static buffer).
     */

    static CHAR buffer[MAX_INTEGER_SPACE + 1];

    snprintf(buffer, MAX_INTEGER_SPACE, INT64_FORMAT, val);
    return buffer;
}

LPSTR ansiUi64toa( /* PRIVATE */
    UINT64 val
    )
{
    /*
     * BUGBUG: This is not thread-safe (static buffer).
     */

    static CHAR buffer[MAX_INTEGER_SPACE + 1];

    snprintf(buffer, MAX_INTEGER_SPACE, UINT64_FORMAT, val);
    return buffer;
}

LPSTR ansiStrcat( /* PRIVATE */
    LPSTR dest,
    LPCSTR src
    )
{
    return strcat(dest, src);
}

LPSTR ansiStrdup( /* PRIVATE */
    LPCSTR str
    )
{
    if (str != NULL) {
        SIZE_T length = strlen(str);
        LPSTR newStr = (LPSTR)ACK_zalloc(
            sizeof(CHAR) * (length + 1));

        if (newStr != NULL) {
            memcpy(newStr, str, sizeof(CHAR) * length);
            newStr[length] = '\0'; /* NOTE: Null terminate. */

            return newStr;
        }
    }

    return NULL;
}

INT ansiFseek( /* PRIVATE */
    FILE *stream,
    INT64 offset,
    INT whence
    )
{
    return fseek(stream, (long)offset, whence);
}

INT64 ansiFtell( /* PRIVATE */
    FILE *stream
    )
{
    return ftell(stream);
}

#endif /* defined(ACK_OS_ANSI) */

/* end of file */
