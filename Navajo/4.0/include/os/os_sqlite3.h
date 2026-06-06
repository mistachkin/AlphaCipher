/*
 * os_sqlite3.h --
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

#ifndef _OS_SQLITE3_H_
#define _OS_SQLITE3_H_

/*****************************************************************************/

extern ACK_OS sqlite3Os;

/*****************************************************************************/

ACK_PRIVATE LPVOID sqlite3Malloc(
    SIZE_T size /* in */
);

ACK_PRIVATE LPVOID sqlite3Zalloc(
    SIZE_T size /* in */
);

ACK_PRIVATE LPVOID sqlite3Realloc(
    LPVOID pBlock, /* in */
    SIZE_T size,   /* in */
    BOOL zero      /* in */
);

ACK_PRIVATE SIZE_T sqlite3Msize(
    LPVOID pBlock /* in */
);

ACK_PRIVATE VOID sqlite3Free(
    LPVOID pBlock /* in */
);

/*****************************************************************************/

#endif /* _OS_SQLITE3_H_ */

/* end of file */
