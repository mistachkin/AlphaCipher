/*
 * os_msvc.c --
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

#if defined(ACK_OS_MSVC)

#include <direct.h>
#include <io.h>
#include <malloc.h>

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
#include "os_msvc.h"

ACK_OS msvcOs = {
    "msvc",       /* zName */
    NULL,         /* pAppData */
    NULL,         /* xInitialize */
    NULL,         /* xFinalize */
    NULL,         /* xInitializeMutex */
    NULL,         /* xFinalizeMutex */
    NULL,         /* xLockMutex */
    NULL,         /* xUnlockMutex */
    NULL,         /* xErrno */
    NULL,         /* xMalloc */
    NULL,         /* xZalloc */
    NULL,         /* xRealloc */
    msvcMsize,    /* xMsize */
    msvcFree,     /* xFree */
    msvcGetcwd,   /* xGetcwd */
    msvcOpendir,  /* xOpendir */
    msvcReaddir,  /* xReaddir */
    msvcClosedir, /* xClosedir */
    msvcIsdir,    /* xIsdir */
    msvcAtoull,   /* xAtoull */
    msvcItoa,     /* xItoa */
    msvcI64toa,   /* xI64toa */
    msvcUi64toa,  /* xUi64toa */
    NULL,         /* xStrcat */
    NULL,         /* xStrdup */
    msvcFseek,    /* xFseek */
    msvcFtell,    /* xFtell */
    msvcFtruncate /* xFtruncate */
};

SIZE_T msvcMsize( /* PRIVATE */
    LPVOID pBlock
    )
{
    if (pBlock != NULL)
        return _msize(pBlock);
    else
        return (SIZE_T)-1;
}

VOID msvcFree( /* PRIVATE */
    LPVOID pBlock
    )
{
    if (pBlock != NULL) {
        SIZE_T size = _msize(pBlock);

        if (size > 0)
            memset(pBlock, 0, size);

        free(pBlock);
    }
}

LPSTR msvcGetcwd( /* PRIVATE */
    LPSTR buf,
    SIZE_T size
    )
{
    return getcwd(buf, size);
}

LPDIR msvcOpendir( /* PRIVATE */
    LPCSTR dirname
    )
{
    /*
     * NOTE: This function is flagged by the code analysis tool because it
     *       cannot "guarantee" that the "dirname" argument is actually
     *       null-terminated.
     */

    struct _finddata_t data;
    LPDIR dirp = (LPDIR)ACK_zalloc(sizeof(DIR));

    if (dirp == NULL)
        return NULL;

    /*
     * HACK: Avoid excessive allocations by re-purposing the name buffer of the
     *       find data structure to be used as the input parameter as well.
     */

    snprintf(data.name, sizeof(data.name) / sizeof(data.name[0]), "%s\\*",
        dirname);

    dirp->d_handle = _findfirst(data.name, &data);

    if (dirp->d_handle == BAD_INTPTR_T) {
        /* IGNORED */
        msvcClosedir(dirp);

        return NULL;
    }

    dirp->d_first.d_attributes = data.attrib;

    strncpy(dirp->d_first.d_name, data.name, NAME_MAX);
    dirp->d_first.d_name[NAME_MAX] = '\0';

    return dirp;
}

LPDIRENT msvcReaddir( /* PRIVATE */
    LPDIR dirp
    )
{
    struct _finddata_t data;
    INT result = 0;

    if (dirp == NULL)
        return NULL;

    if (dirp->d_first.d_ino == 0) {
        dirp->d_first.d_ino++; /* NOTE: This one is #1. */
        dirp->d_next.d_ino++;  /* NOTE: Next one is #2. */

        return &dirp->d_first;
    }

    result = _findnext(dirp->d_handle, &data);

    if (result == -1)
        return NULL;

    dirp->d_next.d_attributes = data.attrib;

    strncpy(dirp->d_next.d_name, data.name, NAME_MAX);
    dirp->d_next.d_name[NAME_MAX] = '\0';

    /*
     * NOTE: Increment the count for the number of results we have returned
     *       from this function.
     */

    dirp->d_next.d_ino++;

    return &dirp->d_next;
}

INT msvcClosedir( /* PRIVATE */
    LPDIR dirp
    )
{
    INT result = 0;

    if (dirp == NULL)
        return EINVAL;

    if ((dirp->d_handle != NULL_INTPTR_T) &&
            (dirp->d_handle != BAD_INTPTR_T)) {
        result = _findclose(dirp->d_handle);
    }

    ACK_free(dirp);

    return result;
}

BOOL msvcIsdir( /* PRIVATE */
    LPCSTR dirname,
    LPDIRENT direntp
    )
{
    if ((dirname == NULL) || (direntp == NULL))
        return FALSE;

    return direntp->d_attributes & FILE_ATTRIBUTE_DIRECTORY;
}

UINT64 msvcAtoull( /* PRIVATE */
    LPCSTR str
    )
{
    return (UINT64)_atoi64(str);
}

LPSTR msvcItoa( /* PRIVATE */
    INT val
    )
{
    /*
     * BUGBUG: This is not thread-safe (static buffer).
     */

    static CHAR buffer[MAX_INTEGER_SPACE + 1];

    return itoa(val, buffer, 10);
}

LPSTR msvcI64toa( /* PRIVATE */
    INT64 val
    )
{
    /*
     * BUGBUG: This is not thread-safe (static buffer).
     */

    static CHAR buffer[MAX_INTEGER_SPACE + 1];

    return _i64toa(val, buffer, 10);
}

LPSTR msvcUi64toa( /* PRIVATE */
    UINT64 val
    )
{
    /*
     * BUGBUG: This is not thread-safe (static buffer).
     */

    static CHAR buffer[MAX_INTEGER_SPACE + 1];

    return _ui64toa(val, buffer, 10);
}

INT msvcFseek( /* PRIVATE */
    FILE *stream,
    INT64 offset,
    INT whence
    )
{
    return _fseeki64(stream, offset, whence);
}

INT64 msvcFtell( /* PRIVATE */
    FILE *stream
    )
{
    return _ftelli64(stream);
}

ERRNO_T msvcFtruncate( /* PRIVATE */
    FILE *stream,
    INT64 size
    )
{
    return _chsize_s(fileno(stream), size);
}

#endif /* defined(ACK_OS_MSVC) */

/* end of file */
