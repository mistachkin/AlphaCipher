/*
 * os_wince.c --
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

#if defined(ACK_OS_WINCE)

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "navajoPort.h"
#include "navajo.h"
#include "navajoSqlite.h"
#include "navajoIntTypes.h"
#include "navajoInt.h"
#include "os.h"
#include "os_null.h"
#include "os_wince.h"

ACK_OS winceOs = {
    "wince",       /* zName */
    NULL,          /* pAppData */
    NULL,          /* xInitialize */
    NULL,          /* xFinalize */
    NULL,          /* xInitializeMutex */
    NULL,          /* xFinalizeMutex */
    NULL,          /* xLockMutex */
    NULL,          /* xUnlockMutex */
    NULL,          /* xErrno */
    NULL,          /* xMalloc */
    NULL,          /* xZalloc */
    NULL,          /* xRealloc */
    NULL,          /* xMsize */
    NULL,          /* xFree */
    nullGetcwd,    /* xGetcwd */
    winceOpendir,  /* xOpendir */
    winceReaddir,  /* xReaddir */
    winceClosedir, /* xClosedir */
    winceIsdir,    /* xIsdir */
    winceAtoull,   /* xAtoull */
    NULL,          /* xItoa */
    NULL,          /* xI64toa */
    NULL,          /* xUi64toa */
    NULL,          /* xStrcat */
    NULL,          /* xStrdup */
    NULL,          /* xFseek */
    NULL,          /* xFtell */
    NULL           /* xFtruncate */
};

LPDIR winceOpendir( /* PRIVATE */
    LPCSTR dirname
    )
{
    WCHAR wdirname[MAX_PATH]= {0};
    WIN32_FIND_DATAW data = {0};
    LPDIR dirp = (LPDIR)ACK_zalloc(sizeof(DIR));

    if (dirp == NULL)
        return NULL;

    /*
     * HACK: Avoid excessive allocations by re-purposing the name buffer of the
     *       find data structure to be used as the input parameter as well.
     */

    if (!MultiByteToWideChar(CP_ACP, 0, dirname, -1, wdirname, MAX_PATH))
        return NULL;

    _snwprintf(data.cFileName,
        sizeof(data.cFileName) / sizeof(data.cFileName[0]),
        L"%s\\*", wdirname);

    dirp->d_handle = FindFirstFileW(data.cFileName, &data);

    if ((dirp->d_handle == NULL) ||
            (dirp->d_handle == INVALID_HANDLE_VALUE)) {
        winceClosedir(dirp);
        return NULL;
    }

    dirp->d_first.d_attributes = data.dwFileAttributes;

    if (!WideCharToMultiByte(CP_ACP, 0, data.cFileName, -1,
            dirp->d_first.d_name, NAME_MAX, NULL, NULL)) {
        winceClosedir(dirp);
        return NULL;
    }

    dirp->d_first.d_name[NAME_MAX] = '\0';

    return dirp;
}

LPDIRENT winceReaddir( /* PRIVATE */
    LPDIR dirp
    )
{
    WIN32_FIND_DATAW data;

    if (dirp == NULL)
        return NULL;

    if (dirp->d_first.d_ino == 0) {
        dirp->d_first.d_ino++; /* NOTE: This one is #1. */
        dirp->d_next.d_ino++;  /* NOTE: Next one is #2. */

        return &dirp->d_first;
    }

    if (!FindNextFileW(dirp->d_handle, &data))
        return NULL;

    dirp->d_next.d_attributes = data.dwFileAttributes;

    if (!WideCharToMultiByte(CP_ACP, 0, data.cFileName, -1,
            dirp->d_next.d_name, NAME_MAX, NULL, NULL)) {
        return NULL;
    }

    dirp->d_next.d_name[NAME_MAX] = '\0';

    /*
     * NOTE: Increment the count for the number of results we have returned
     *       from this function.
     */

    dirp->d_next.d_ino++;

    return &dirp->d_next;
}

INT winceClosedir( /* PRIVATE */
    LPDIR dirp
    )
{
    INT result = 0;

    if (dirp == NULL)
        return EINVAL;

    if ((dirp->d_handle != NULL) &&
            (dirp->d_handle != INVALID_HANDLE_VALUE)) {
        result = FindClose(dirp->d_handle) ? -1 : 0;
    }

    ACK_free(dirp);

    return result;
}

BOOL winceIsdir( /* PRIVATE */
    LPCSTR dirname,
    LPDIRENT direntp
    )
{
    if ((dirname == NULL) || (direntp == NULL))
        return FALSE;

    return direntp->d_attributes & FILE_ATTRIBUTE_DIRECTORY;
}

UINT64 winceAtoull( /* PRIVATE */
    LPCSTR str
    )
{
    return (UINT64)_atoi64(str);
}

#endif /* ACK_OS_WINCE */

/* end of file */
