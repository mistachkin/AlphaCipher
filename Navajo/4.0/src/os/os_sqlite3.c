/*
 * os_sqlite3.c --
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

#if defined(ACK_OS_SQLITE3)

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "navajoPort.h"
#include "navajo.h"
#include "navajoSqlite.h"
#include "navajoIntTypes.h"
#include "os_sqlite3.h"

ACK_OS sqlite3Os = {
    "sqlite3",      /* zName */
    NULL,           /* pAppData */
    NULL,           /* xInitialize */
    NULL,           /* xFinalize */
    NULL,           /* xInitializeMutex */
    NULL,           /* xFinalizeMutex */
    NULL,           /* xLockMutex */
    NULL,           /* xUnlockMutex */
    NULL,           /* xErrno */
    sqlite3Malloc,  /* xMalloc */
    sqlite3Zalloc,  /* xZalloc */
    sqlite3Realloc, /* xRealloc */
    sqlite3Msize,   /* xMsize */
    sqlite3Free,    /* xFree */
    NULL,           /* xGetcwd */
    NULL,           /* xOpendir */
    NULL,           /* xReaddir */
    NULL,           /* xClosedir */
    NULL,           /* xIsdir */
    NULL,           /* xAtoull */
    NULL,           /* xItoa */
    NULL,           /* xI64toa */
    NULL,           /* xUi64toa */
    NULL,           /* xStrcat */
    NULL,           /* xStrdup */
    NULL,           /* xFseek */
    NULL,           /* xFtell */
    NULL            /* xFtruncate */
};

LPVOID sqlite3Malloc( /* PRIVATE */
    SIZE_T size
    )
{
    return sqlite3_malloc(size);
}

LPVOID sqlite3Zalloc( /* PRIVATE */
    SIZE_T size
    )
{
    LPVOID pBlock = sqlite3_malloc(size);

    if (pBlock != NULL)
        memset(pBlock, 0, size);

    return pBlock;
}

LPVOID sqlite3Realloc( /* PRIVATE */
    LPVOID pBlock,
    SIZE_T size,
    BOOL zero
    )
{
    LPVOID pNewBlock = sqlite3_realloc(pBlock, size);

    if (zero && (pNewBlock != NULL))
        memset(pNewBlock, 0, size);

    return pNewBlock;
}

SIZE_T sqlite3Msize( /* PRIVATE */
    LPVOID pBlock
    )
{
    if (pBlock != NULL)
#if SQLITE_VERSION_NUMBER >= 3008007
        return (SIZE_T)sqlite3_msize(pBlock);
#else
        return 0;
#endif
    else
        return (SIZE_T)-1;
}

VOID sqlite3Free( /* PRIVATE */
    LPVOID pBlock
    )
{
    if (pBlock != NULL)
        sqlite3_free(pBlock);
}

#endif /* defined(ACK_OS_SQLITE3) */

/* end of file */
