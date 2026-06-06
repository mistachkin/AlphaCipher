/*
 * os_win32.c --
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

#if defined(ACK_OS_WIN32)

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "navajoPort.h"
#include "navajo.h"
#include "navajoSqlite.h"
#include "navajoIntTypes.h"
#include "navajoInt.h"
#include "os.h"
#include "os_win32.h"

ACK_OS win32Os = {
    "win32",              /* zName */
    NULL,                 /* pAppData */
    NULL,                 /* xInitialize */
    NULL,                 /* xFinalize */
    win32InitializeMutex, /* xInitializeMutex */
    win32FinalizeMutex,   /* xFinalizeMutex */
    win32LockMutex,       /* xLockMutex */
    win32UnlockMutex,     /* xUnlockMutex */
    NULL,                 /* xErrno */
    NULL,                 /* xMalloc */
    NULL,                 /* xZalloc */
    NULL,                 /* xRealloc */
    NULL,                 /* xMsize */
    NULL,                 /* xFree */
    NULL,                 /* xGetcwd */
    NULL,                 /* xOpendir */
    NULL,                 /* xReaddir */
    NULL,                 /* xClosedir */
    NULL,                 /* xIsdir */
    NULL,                 /* xAtoull */
    NULL,                 /* xItoa */
    NULL,                 /* xI64toa */
    NULL,                 /* xUi64toa */
    NULL,                 /* xStrcat */
    NULL,                 /* xStrdup */
    NULL,                 /* xFseek */
    NULL,                 /* xFtell */
    NULL                  /* xFtruncate */
};

VOID win32InitializeMutex( /* PRIVATE */
    ACK_LPMUTEX pMutex
    )
{
    InitializeCriticalSection(pMutex);
}

VOID win32FinalizeMutex( /* PRIVATE */
    ACK_LPMUTEX pMutex
    )
{
    DeleteCriticalSection(pMutex);
}

VOID win32LockMutex( /* PRIVATE */
    ACK_LPMUTEX pMutex
    )
{
    EnterCriticalSection(pMutex);
}

VOID win32UnlockMutex( /* PRIVATE */
    ACK_LPMUTEX pMutex
    )
{
    LeaveCriticalSection(pMutex);
}

#endif /* ACK_OS_WIN32 */

/* end of file */
