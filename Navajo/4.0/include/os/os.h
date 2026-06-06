/*
 * os.h --
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

#ifndef _OS_H_
#define _OS_H_

/*****************************************************************************/

extern ACK_OS currentOs;

/*****************************************************************************/

ACK_PRIVATE ACK_RESULT osInitialize(
    ACK_LPOS *ppOs, /* in */
    ACK_LPOS pOs,   /* in */
    LPSTR zName,    /* in */
    LPVOID pAppData /* in */
);

ACK_PRIVATE ACK_RESULT osFinalize(
    ACK_LPOS *ppOs /* in */
);

ACK_PRIVATE VOID osInitializeMutex(
    ACK_LPOS pOs,      /* in */
    ACK_LPMUTEX pMutex /* in */
);

ACK_PRIVATE VOID osFinalizeMutex(
    ACK_LPOS pOs,      /* in */
    ACK_LPMUTEX pMutex /* in */
);

ACK_PRIVATE VOID osLockMutex(
    ACK_LPOS pOs,      /* in */
    ACK_LPMUTEX pMutex /* in */
);

ACK_PRIVATE VOID osUnlockMutex(
    ACK_LPOS pOs,      /* in */
    ACK_LPMUTEX pMutex /* in */
);

ACK_PRIVATE INT osErrno(
    ACK_LPOS pOs /* in */
);

ACK_PRIVATE LPVOID osMalloc(
    ACK_LPOS pOs, /* in */
    SIZE_T size   /* in */
);

ACK_PRIVATE LPVOID osZalloc(
    ACK_LPOS pOs, /* in */
    SIZE_T size   /* in */
);

ACK_PRIVATE LPVOID osRealloc(
    ACK_LPOS pOs,  /* in */
    LPVOID pBlock, /* in */
    SIZE_T size,   /* in */
    BOOL zero      /* in */
);

ACK_PRIVATE SIZE_T osMsize(
    ACK_LPOS pOs, /* in */
    LPVOID pBlock /* in */
);

ACK_PRIVATE VOID osFree(
    ACK_LPOS pOs, /* in */
    LPVOID pBlock /* in */
);

ACK_PRIVATE LPSTR osGetcwd(
    ACK_LPOS pOs, /* in */
    LPSTR buf,    /* in */
    SIZE_T size   /* in */
);

ACK_PRIVATE LPDIR osOpendir(
    ACK_LPOS pOs,  /* in */
    LPCSTR dirname /* in */
);

ACK_PRIVATE LPDIRENT osReaddir(
    ACK_LPOS pOs, /* in */
    LPDIR dirp    /* in */
);

ACK_PRIVATE INT osClosedir(
    ACK_LPOS pOs, /* in */
    LPDIR dirp    /* in */
);

ACK_PRIVATE BOOL osIsdir(
    ACK_LPOS pOs,    /* in */
    LPCSTR dirname,  /* in */
    LPDIRENT direntp /* in */
);

ACK_PRIVATE UINT64 osAtoull(
    ACK_LPOS pOs, /* in */
    LPCSTR str    /* in */
);

ACK_PRIVATE LPSTR osItoa(
    ACK_LPOS pOs, /* in */
    INT val       /* in */
);

ACK_PRIVATE LPSTR osI64toa(
    ACK_LPOS pOs, /* in */
    INT64 val     /* in */
);

ACK_PRIVATE LPSTR osUi64toa(
    ACK_LPOS pOs, /* in */
    UINT64 val    /* in */
);

ACK_PRIVATE LPSTR osStrcat(
    ACK_LPOS pOs, /* in */
    LPSTR dest,   /* in */
    LPCSTR src    /* in */
);

ACK_PRIVATE LPSTR osStrdup(
    ACK_LPOS pOs, /* in */
    LPCSTR str    /* in */
);

ACK_PRIVATE INT osFseek(
    ACK_LPOS pOs, /* in */
    FILE *stream, /* in */
    INT64 offset, /* in */
    INT whence    /* in */
);

ACK_PRIVATE INT64 osFtell(
    ACK_LPOS pOs, /* in */
    FILE *stream  /* in */
);

ACK_PRIVATE ERRNO_T osFtruncate(
    ACK_LPOS pOs, /* in */
    FILE *stream, /* in */
    INT64 size    /* in */
);

/*****************************************************************************/

#endif /* _OS_H_ */

/* end of file */
