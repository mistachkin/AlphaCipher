/*
 * os_null.h --
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

#ifndef _OS_NULL_H_
#define _OS_NULL_H_

/*****************************************************************************/

extern ACK_OS nullOs;

/*****************************************************************************/

ACK_PRIVATE ACK_RESULT nullInitialize(
    LPVOID pOs,     /* in */
    LPVOID pAppData /* in */
);

ACK_PRIVATE ACK_RESULT nullFinalize(
    LPVOID pOs,     /* in */
    LPVOID pAppData /* in */
);

ACK_PRIVATE VOID nullInitializeMutex(
    ACK_LPMUTEX pMutex /* in */
);

ACK_PRIVATE VOID nullFinalizeMutex(
    ACK_LPMUTEX pMutex /* in */
);

ACK_PRIVATE VOID nullLockMutex(
    ACK_LPMUTEX pMutex /* in */
);

ACK_PRIVATE VOID nullUnlockMutex(
    ACK_LPMUTEX pMutex /* in */
);

ACK_PRIVATE INT nullErrno(VOID);

ACK_PRIVATE LPVOID nullMalloc(
    SIZE_T size /* in */
);

ACK_PRIVATE LPVOID nullZalloc(
    SIZE_T size /* in */
);

ACK_PRIVATE LPVOID nullRealloc(
    LPVOID pBlock, /* in */
    SIZE_T size,   /* in */
    BOOL zero      /* in */
);

ACK_PRIVATE SIZE_T nullMsize(
    LPVOID pBlock /* in */
);

ACK_PRIVATE VOID nullFree(
    LPVOID pBlock /* in */
);

ACK_PRIVATE LPSTR nullGetcwd(
    LPSTR buf,  /* in */
    SIZE_T size /* in */
);

ACK_PRIVATE LPDIR nullOpendir(
    LPCSTR dirname /* in */
);

ACK_PRIVATE LPDIRENT nullReaddir(
    LPDIR dirp /* in */
);

ACK_PRIVATE INT nullClosedir(
    LPDIR dirp /* in */
);

ACK_PRIVATE BOOL nullIsdir(
    LPCSTR dirname,  /* in */
    LPDIRENT direntp /* in */
);

ACK_PRIVATE UINT64 nullAtoull(
    LPCSTR str /* in */
);

ACK_PRIVATE LPSTR nullItoa(
    INT val /* in */
);

ACK_PRIVATE LPSTR nullI64toa(
    INT64 val /* in */
);

ACK_PRIVATE LPSTR nullUi64toa(
    UINT64 val /* in */
);

ACK_PRIVATE LPSTR nullStrcat(
    LPSTR dest, /* in */
    LPCSTR src  /* in */
);

ACK_PRIVATE LPSTR nullStrdup(
    LPCSTR str /* in */
);

ACK_PRIVATE INT nullFseek(
    FILE *stream, /* in */
    INT64 offset, /* in */
    INT whence    /* in */
);

ACK_PRIVATE INT64 nullFtell(
    FILE *stream /* in */
);

ACK_PRIVATE ERRNO_T nullFtruncate(
    FILE *stream, /* in */
    INT64 size    /* in */
);

/*****************************************************************************/

#endif /* _OS_NULL_H_ */

/* end of file */
