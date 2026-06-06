/*
 * os.c --
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
#include "navajoUtil.h"
#include "os.h"
#include "os_null.h"

#if defined(ACK_OS_ANSI)
#include "os_ansi.h"
#endif

#if defined(ACK_OS_GCC)
#include "os_gcc.h"
#endif

#if defined(ACK_OS_MSVC)
#include "os_msvc.h"
#endif

#if defined(ACK_OS_POSIX)
#include "os_posix.h"
#endif

#if defined(ACK_OS_WIN32)
#include "os_win32.h"
#endif

#if defined(ACK_OS_WINCE)
#include "os_wince.h"
#endif

#if defined(ACK_OS_SQLITE3)
#include "os_sqlite3.h"
#endif

ACK_OS currentOs = {
    NULL, /* zName */
    NULL, /* pAppData */
    NULL, /* xInitialize */
    NULL, /* xFinalize */
    NULL, /* xInitializeMutex */
    NULL, /* xFinalizeMutex */
    NULL, /* xLockMutex */
    NULL, /* xUnlockMutex */
    NULL, /* xErrno */
    NULL, /* xMalloc */
    NULL, /* xZalloc */
    NULL, /* xRealloc */
    NULL, /* xMsize */
    NULL, /* xFree */
    NULL, /* xGetcwd */
    NULL, /* xOpendir */
    NULL, /* xReaddir */
    NULL, /* xClosedir */
    NULL, /* xIsdir */
    NULL, /* xAtoull */
    NULL, /* xItoa */
    NULL, /* xI64toa */
    NULL, /* xUi64toa */
    NULL, /* xStrdup */
    NULL, /* xFseek */
    NULL, /* xFtell */
    NULL  /* xFtruncate */
};

ACK_LPOS aOs[] = {
    &nullOs,

#if defined(ACK_OS_ANSI)
    &ansiOs,
#endif

#if defined(ACK_OS_POSIX)
    &posixOs,
#endif

#if defined(ACK_OS_GCC)
    &gccOs,
#endif

#if defined(ACK_OS_MSVC)
    &msvcOs,
#endif

#if defined(ACK_OS_WIN32)
    &win32Os,
#endif

#if defined(ACK_OS_WINCE)
    &winceOs,
#endif

#if defined(ACK_OS_SQLITE3)
    &sqlite3Os,
#endif

    NULL
};

#ifndef ACK_OS_COUNT
    #define ACK_OS_COUNT (sizeof(aOs) / sizeof(aOs[0]))
#endif

#ifndef ACK_OS_MEMBER_COUNT
    #define ACK_OS_MEMBER_COUNT (sizeof(ACK_OS) / sizeof(LPVOID))
#endif

ACK_RESULT osInitialize( /* PRIVATE */
    ACK_LPOS *ppOs,
    ACK_LPOS pOs,
    LPSTR zName,
    LPVOID pAppData
    )
{
    ACK_RESULT kResult = ACK_S_OK;
    LPVOID *ppvOs = NULL;
    INT index[2] = { 0, 0 };

    if ((ppOs == NULL) || (pOs == NULL))
        return E_POINTER;

    if ((zName != NULL) || (pAppData != NULL))
        return E_MUSTBENULL;

    if (*ppOs != NULL)
        return S_OK;

    ppvOs = (LPVOID *)pOs;

    for (index[0] = 0; index[0] < ACK_OS_COUNT; index[0]++) {
        LPVOID *paOs = (LPVOID *)aOs[index[0]];

        if (paOs == NULL)
            continue;

        for (index[1] = 0; index[1] < ACK_OS_MEMBER_COUNT; index[1]++) {
            if (paOs[index[1]] != NULL)
                ppvOs[index[1]] = paOs[index[1]];
        }
    }

    if (pOs->xInitialize != NULL) {
        kResult = pOs->xInitialize(pOs, pOs->pAppData);

        if (FAILED(kResult))
            goto done;
    }

    *ppOs = pOs;

done:
    return kResult;
}

ACK_RESULT osFinalize( /* PRIVATE */
    ACK_LPOS *ppOs
    )
{
    ACK_RESULT kResult = ACK_S_OK;
    ACK_LPOS pOs = NULL;
    LPVOID *ppvOs = NULL;
    INT index = 0;

    if (ppOs == NULL)
        return E_POINTER;

    pOs = *ppOs;

    if (pOs == NULL)
        return S_OK;

    if (pOs->xFinalize != NULL) {
        kResult = pOs->xFinalize(pOs, pOs->pAppData);

        if (FAILED(kResult))
            goto done;
    }

    ppvOs = (LPVOID *)pOs;

    for (index = 0; index < ACK_OS_MEMBER_COUNT; index++)
        ppvOs[index] = NULL;

    *ppOs = NULL;

done:
    return kResult;
}

VOID osInitializeMutex( /* PRIVATE */
    ACK_LPOS pOs,
    ACK_LPMUTEX pMutex
    )
{
    if ((pOs != NULL) && (pOs->xInitializeMutex != NULL))
        pOs->xInitializeMutex(pMutex);
}

VOID osFinalizeMutex( /* PRIVATE */
    ACK_LPOS pOs,
    ACK_LPMUTEX pMutex
    )
{
    if ((pOs != NULL) && (pOs->xFinalizeMutex != NULL))
        pOs->xFinalizeMutex(pMutex);
}

VOID osLockMutex( /* PRIVATE */
    ACK_LPOS pOs,
    ACK_LPMUTEX pMutex
    )
{
    if ((pOs != NULL) && (pOs->xLockMutex != NULL))
        pOs->xLockMutex(pMutex);
}

VOID osUnlockMutex( /* PRIVATE */
    ACK_LPOS pOs,
    ACK_LPMUTEX pMutex
    )
{
    if ((pOs != NULL) && (pOs->xUnlockMutex != NULL))
        pOs->xUnlockMutex(pMutex);
}

INT osErrno( /* PRIVATE */
    ACK_LPOS pOs
    )
{
    return ((pOs != NULL) && (pOs->xErrno != NULL)) ?
        pOs->xErrno(): 0;
}

LPVOID osMalloc( /* PRIVATE */
    ACK_LPOS pOs,
    SIZE_T size
    )
{
    return ((pOs != NULL) && (pOs->xMalloc != NULL)) ?
        pOs->xMalloc(size): NULL;
}

LPVOID osZalloc( /* PRIVATE */
    ACK_LPOS pOs,
    SIZE_T size
    )
{
    return ((pOs != NULL) && (pOs->xZalloc != NULL)) ?
        pOs->xZalloc(size): NULL;
}

LPVOID osRealloc( /* PRIVATE */
    ACK_LPOS pOs,
    LPVOID pBlock,
    SIZE_T size,
    BOOL zero
    )
{
    return ((pOs != NULL) && (pOs->xRealloc != NULL)) ?
        pOs->xRealloc(pBlock, size, zero): NULL;
}

SIZE_T osMsize( /* PRIVATE */
    ACK_LPOS pOs,
    LPVOID pBlock
    )
{
    return ((pOs != NULL) && (pOs->xMsize != NULL)) ?
        pOs->xMsize(pBlock): -1;
}

VOID osFree( /* PRIVATE */
    ACK_LPOS pOs,
    LPVOID pBlock
    )
{
    if ((pOs != NULL) && (pOs->xFree != NULL))
        pOs->xFree(pBlock);
}

LPSTR osGetcwd( /* PRIVATE */
    ACK_LPOS pOs,
    LPSTR buf,
    SIZE_T size
    )
{
    return ((pOs != NULL) && (pOs->xGetcwd != NULL)) ?
        pOs->xGetcwd(buf, size): NULL;
}

LPDIR osOpendir( /* PRIVATE */
    ACK_LPOS pOs,
    LPCSTR dirname
    )
{
    return ((pOs != NULL) && (pOs->xOpendir != NULL)) ?
        pOs->xOpendir(dirname): NULL;
}

LPDIRENT osReaddir( /* PRIVATE */
    ACK_LPOS pOs,
    LPDIR dirp
    )
{
    return ((pOs != NULL) && (pOs->xReaddir != NULL)) ?
        pOs->xReaddir(dirp): NULL;
}

INT osClosedir( /* PRIVATE */
    ACK_LPOS pOs,
    LPDIR dirp
    )
{
    return ((pOs != NULL) && (pOs->xClosedir != NULL)) ?
        pOs->xClosedir(dirp): -1;
}

BOOL osIsdir( /* PRIVATE */
    ACK_LPOS pOs,
    LPCSTR dirname,
    LPDIRENT direntp
    )
{
    return ((pOs != NULL) && (pOs->xIsdir != NULL)) ?
        pOs->xIsdir(dirname, direntp): FALSE;
}

UINT64 osAtoull( /* PRIVATE */
    ACK_LPOS pOs,
    LPCSTR str
    )
{
    return ((pOs != NULL) && (pOs->xAtoull != NULL)) ?
        pOs->xAtoull(str): 0;
}

LPSTR osItoa( /* PRIVATE */
    ACK_LPOS pOs,
    INT val
    )
{
    return ((pOs != NULL) && (pOs->xItoa != NULL)) ?
        pOs->xItoa(val): NULL;
}

LPSTR osI64toa( /* PRIVATE */
    ACK_LPOS pOs,
    INT64 val
    )
{
    return ((pOs != NULL) && (pOs->xI64toa != NULL)) ?
        pOs->xI64toa(val): NULL;
}

LPSTR osUi64toa( /* PRIVATE */
    ACK_LPOS pOs,
    UINT64 val
    )
{
    return ((pOs != NULL) && (pOs->xUi64toa != NULL)) ?
        pOs->xUi64toa(val): NULL;
}

LPSTR osStrcat( /* PRIVATE */
    ACK_LPOS pOs,
    LPSTR dest,
    LPCSTR src
    )
{
    return ((pOs != NULL) && (pOs->xStrcat != NULL)) ?
        pOs->xStrcat(dest, src): NULL;
}

LPSTR osStrdup( /* PRIVATE */
    ACK_LPOS pOs,
    LPCSTR str
    )
{
    return ((pOs != NULL) && (pOs->xStrdup != NULL)) ?
        pOs->xStrdup(str): NULL;
}

INT osFseek( /* PRIVATE */
    ACK_LPOS pOs,
    FILE *stream,
    INT64 offset,
    INT whence
    )
{
    return ((pOs != NULL) && (pOs->xFseek != NULL)) ?
        pOs->xFseek(stream, offset, whence): -1;
}

INT64 osFtell( /* PRIVATE */
    ACK_LPOS pOs,
    FILE *stream
    )
{
    return ((pOs != NULL) && (pOs->xFtell != NULL)) ?
        pOs->xFtell(stream): -1;
}

ERRNO_T osFtruncate( /* PRIVATE */
    ACK_LPOS pOs,
    FILE *stream,
    INT64 size
    )
{
    return ((pOs != NULL) && (pOs->xFtruncate != NULL)) ?
        pOs->xFtruncate(stream, size): -1;
}

/* end of file */
