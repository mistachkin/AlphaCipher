/*
 * os_ansi.h --
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

#ifndef _OS_ANSI_H_
#define _OS_ANSI_H_

/*****************************************************************************/

extern ACK_OS ansiOs;

/*****************************************************************************/

ACK_PRIVATE INT ansiErrno(
    VOID
);

ACK_PRIVATE LPVOID ansiMalloc(
    SIZE_T size /* in */
);

ACK_PRIVATE LPVOID ansiZalloc(
    SIZE_T size /* in */
);

ACK_PRIVATE LPVOID ansiRealloc(
    LPVOID pBlock, /* in */
    SIZE_T size,   /* in */
    BOOL zero      /* in */
);

ACK_PRIVATE VOID ansiFree(
    LPVOID pBlock /* in */
);

ACK_PRIVATE UINT64 ansiAtoull(
    LPCSTR str /* in */
);

ACK_PRIVATE LPSTR ansiItoa(
    INT val /* in */
);

ACK_PRIVATE LPSTR ansiI64toa(
    INT64 val /* in */
);

ACK_PRIVATE LPSTR ansiUi64toa(
    UINT64 val /* in */
);

ACK_PRIVATE LPSTR ansiStrcat(
    LPSTR dest, /* in */
    LPCSTR src  /* in */
);

ACK_PRIVATE LPSTR ansiStrdup(
    LPCSTR str /* in */
);

ACK_PRIVATE INT ansiFseek(
    FILE *stream, /* in */
    INT64 offset, /* in */
    INT whence    /* in */
);

ACK_PRIVATE INT64 ansiFtell(
    FILE *stream /* in */
);

/*****************************************************************************/

#endif /* _OS_ANSI_H_ */

/* end of file */
