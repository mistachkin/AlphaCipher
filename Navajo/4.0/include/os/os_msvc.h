/*
 * os_msvc.h --
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

#ifndef _OS_MSVC_H_
#define _OS_MSVC_H_

/*****************************************************************************/

extern ACK_OS msvcOs;

/*****************************************************************************/

ACK_PRIVATE SIZE_T msvcMsize(
    LPVOID pBlock /* in */
);

ACK_PRIVATE VOID msvcFree(
    LPVOID pBlock /* in */
);

ACK_PRIVATE LPSTR msvcGetcwd(
    LPSTR buf,  /* in */
    SIZE_T size /* in */
);

ACK_PRIVATE LPDIR msvcOpendir(
    LPCSTR dirname /* in */
);

ACK_PRIVATE LPDIRENT msvcReaddir(
    LPDIR dirp /* in */
);

ACK_PRIVATE INT msvcClosedir(
    LPDIR dirp /* in */
);

ACK_PRIVATE BOOL msvcIsdir(
    LPCSTR dirname,  /* in */
    LPDIRENT direntp /* in */
);

ACK_PRIVATE UINT64 msvcAtoull(
    LPCSTR str /* in */
);

ACK_PRIVATE LPSTR msvcItoa(
    INT val /* in */
);

ACK_PRIVATE LPSTR msvcI64toa(
    INT64 val /* in */
);

ACK_PRIVATE LPSTR msvcUi64toa(
    UINT64 val /* in */
);

ACK_PRIVATE INT msvcFseek(
    FILE *stream, /* in */
    INT64 offset, /* in */
    INT whence    /* in */
);

ACK_PRIVATE INT64 msvcFtell(
    FILE *stream /* in */
);

ACK_PRIVATE ERRNO_T msvcFtruncate(
    FILE *stream, /* in */
    INT64 size    /* in */
);

/*****************************************************************************/

#endif /* _OS_MSVC_H_ */

/* end of file */
