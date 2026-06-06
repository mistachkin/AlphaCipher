/*
 * os_gcc.c --
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

#if defined(ACK_OS_GCC)

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_MALLOC_NP_H
#include <malloc_np.h>
#endif

#include "navajoPort.h"
#include "navajo.h"
#include "navajoSqlite.h"
#include "navajoIntTypes.h"
#include "navajoInt.h"
#include "os.h"
#include "os_gcc.h"

ACK_OS gccOs = {
    "gcc",    /* zName */
    NULL,     /* pAppData */
    NULL,     /* xInitialize */
    NULL,     /* xFinalize */
    NULL,     /* xInitializeMutex */
    NULL,     /* xFinalizeMutex */
    NULL,     /* xLockMutex */
    NULL,     /* xUnlockMutex */
    NULL,     /* xErrno */
    NULL,     /* xMalloc */
    NULL,     /* xZalloc */
    NULL,     /* xRealloc */
    gccMsize, /* xMsize */
    gccFree,  /* xFree */
    NULL,     /* xGetcwd */
    NULL,     /* xOpendir */
    NULL,     /* xReaddir */
    NULL,     /* xClosedir */
    NULL,     /* xIsdir */
    NULL,     /* xAtoull */
    NULL,     /* xItoa */
    NULL,     /* xI64toa */
    NULL,     /* xUi64toa */
    NULL,     /* xStrcat */
    NULL,     /* xStrdup */
    NULL,     /* xFseek */
    NULL,     /* xFtell */
    NULL      /* xFtruncate */
};

SIZE_T gccMsize( /* PRIVATE */
    LPVOID pBlock
    )
{
    if (pBlock != NULL)
#if defined(HAVE_MALLOC_USABLE_SIZE)
        return malloc_usable_size(pBlock);
#else
        return 0;
#endif
    else
        return (SIZE_T)-1;
}

VOID gccFree( /* PRIVATE */
    LPVOID pBlock
    )
{
    if (pBlock != NULL) {
#if defined(HAVE_MALLOC_USABLE_SIZE)
        SIZE_T size = malloc_usable_size(pBlock);

        if (size > 0)
            memset(pBlock, 0, size);
#endif

        free(pBlock);
    }
}

#endif /* defined(ACK_OS_GCC) */

/* end of file */
