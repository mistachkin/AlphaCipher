/*
 * os_posix.c --
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

#if defined(ACK_OS_POSIX)

#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <unistd.h>

#include "navajoPort.h"
#include "navajo.h"
#include "navajoSqlite.h"
#include "navajoIntTypes.h"
#include "navajoInt.h"
#include "navajoUtil.h"
#include "os.h"
#include "os_posix.h"

ACK_OS posixOs = {
    "posix",       /* zName */
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
    posixGetcwd,   /* xGetcwd */
    posixOpendir,  /* xOpendir */
    posixReaddir,  /* xReaddir */
    posixClosedir, /* xClosedir */
    posixIsdir,    /* xIsdir */
    NULL,          /* xAtoull */
    NULL,          /* xItoa */
    NULL,          /* xI64toa */
    NULL,          /* xUi64toa */
    NULL,          /* xStrcat */
    NULL,          /* xStrdup */
    NULL,          /* xFseek */
    NULL,          /* xFtell */
    posixFtruncate /* xFtruncate */
};

LPSTR posixGetcwd( /* PRIVATE */
    LPSTR buf,
    SIZE_T size
    )
{
    return getcwd(buf, size);
}

LPDIR posixOpendir( /* PRIVATE */
    LPCSTR dirname
    )
{
    return opendir(dirname);
}

LPDIRENT posixReaddir( /* PRIVATE */
    LPDIR dirp
    )
{
    return readdir(dirp);
}

INT posixClosedir( /* PRIVATE */
    LPDIR dirp
    )
{
    return closedir(dirp);
}

/*
 * NOTE: PATH_MAX is the POSIX upper bound on a complete pathname.  It is
 *       not guaranteed to be defined on every platform we target, so a
 *       reasonable fallback is provided.
 */
#ifndef ACK_POSIX_PATH_MAX
#ifdef PATH_MAX
#define ACK_POSIX_PATH_MAX (PATH_MAX)
#else
#define ACK_POSIX_PATH_MAX (4096)
#endif
#endif

BOOL posixIsdir( /* PRIVATE */
    LPCSTR dirname,
    LPDIRENT direntp
    )
{
    CHAR path[ACK_POSIX_PATH_MAX + 1];
    struct stat status;

    if ((dirname == NULL) || (direntp == NULL))
        return FALSE;

    snprintf(path, sizeof(path), "%s/%s", dirname, direntp->d_name);

    if (stat(path, &status) == -1)
        return FALSE;

    return status.st_mode & S_IFDIR;
}

ERRNO_T posixFtruncate( /* PRIVATE */
    FILE *stream,
    INT64 size
    )
{
    if (ftruncate(fileno(stream), (off_t)size) != 0)
        return ACK_errno();
    else
        return ENOERR;
}

#endif /* defined(ACK_OS_POSIX) */

/* end of file */
