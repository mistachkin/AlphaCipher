/*
 * os_null.c --
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

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "hresult.h"

#include "navajoPort.h"
#include "navajo.h"
#include "navajoSqlite.h"
#include "navajoIntTypes.h"
#include "navajoInt.h"
#include "os.h"
#include "os_null.h"

ACK_OS nullOs = {
    "null",              /* zName */
    NULL,                /* pAppData */
    nullInitialize,      /* xInitialize */
    nullFinalize,        /* xFinalize */
    nullInitializeMutex, /* xInitializeMutex */
    nullFinalizeMutex,   /* xFinalizeMutex */
    nullLockMutex,       /* xLockMutex */
    nullUnlockMutex,     /* xUnlockMutex */
    nullErrno,           /* xErrno */
    nullMalloc,          /* xMalloc */
    nullZalloc,          /* xZalloc */
    nullRealloc,         /* xRealloc */
    nullMsize,           /* xMsize */
    nullFree,            /* xFree */
    nullGetcwd,          /* xGetcwd */
    nullOpendir,         /* xOpendir */
    nullReaddir,         /* xReaddir */
    nullClosedir,        /* xClosedir */
    nullIsdir,           /* xIsdir */
    nullAtoull,          /* xAtoull */
    nullItoa,            /* xItoa */
    nullI64toa,          /* xI64toa */
    nullUi64toa,         /* xUi64toa */
    nullStrcat,          /* xStrcat */
    nullStrdup,          /* xStrdup */
    nullFseek,           /* xFseek */
    nullFtell,           /* xFtell */
    nullFtruncate        /* xFtruncate */
};

ACK_RESULT nullInitialize( /* PRIVATE */
    LPVOID pOs,
    LPVOID pAppData
    )
{
    return S_OK;
}

ACK_RESULT nullFinalize( /* PRIVATE */
    LPVOID pOs,
    LPVOID pAppData
    )
{
    return S_OK;
}

VOID nullInitializeMutex( /* PRIVATE */
    ACK_LPMUTEX pMutex
    )
{
    return;
}

VOID nullFinalizeMutex( /* PRIVATE */
    ACK_LPMUTEX pMutex
    )
{
    return;
}

VOID nullLockMutex( /* PRIVATE */
    ACK_LPMUTEX pMutex
    )
{
    return;
}

VOID nullUnlockMutex( /* PRIVATE */
    ACK_LPMUTEX pMutex
    )
{
    return;
}

INT nullErrno( /* PRIVATE */
    )
{
    return 0;
}

LPVOID nullMalloc( /* PRIVATE */
    SIZE_T size
    )
{
    return NULL;
}

LPVOID nullZalloc( /* PRIVATE */
    SIZE_T size
    )
{
    return NULL;
}

LPVOID nullRealloc( /* PRIVATE */
    LPVOID pBlock,
    SIZE_T size,
    BOOL zero
    )
{
    return NULL;
}

SIZE_T nullMsize( /* PRIVATE */
    LPVOID pBlock
    )
{
    return (SIZE_T)-1;
}

VOID nullFree( /* PRIVATE */
    LPVOID pBlock
    )
{
    return;
}

LPSTR nullGetcwd( /* PRIVATE */
    LPSTR buf,
    SIZE_T size
    )
{
    return NULL;
}

LPDIR nullOpendir( /* PRIVATE */
    LPCSTR dirname
    )
{
    return NULL;
}

LPDIRENT nullReaddir( /* PRIVATE */
    LPDIR dirp
    )
{
    return NULL;
}

INT nullClosedir( /* PRIVATE */
    LPDIR dirp
    )
{
    return -1;
}

BOOL nullIsdir( /* PRIVATE */
    LPCSTR dirname,
    LPDIRENT direntp
    )
{
    return FALSE;
}

UINT64 nullAtoull( /* PRIVATE */
    LPCSTR str
    )
{
    return 0;
}

LPSTR nullItoa( /* PRIVATE */
    INT val
    )
{
    return NULL;
}

LPSTR nullI64toa( /* PRIVATE */
    INT64 val
    )
{
    return NULL;
}

LPSTR nullUi64toa( /* PRIVATE */
    UINT64 val
    )
{
    return NULL;
}

LPSTR nullStrcat( /* PRIVATE */
    LPSTR dest,
    LPCSTR src
    )
{
    return NULL;
}

LPSTR nullStrdup( /* PRIVATE */
    LPCSTR str
    )
{
    return NULL;
}

INT nullFseek( /* PRIVATE */
    FILE *stream,
    INT64 offset,
    INT whence
    )
{
    return -1;
}

INT64 nullFtell( /* PRIVATE */
    FILE *stream
    )
{
    return -1;
}

ERRNO_T nullFtruncate( /* PRIVATE */
    FILE *stream,
    INT64 size
    )
{
    return -1;
}

/* end of file */
