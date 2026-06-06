/*
 * os_posix.h --
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

#ifndef _OS_POSIX_H_
#define _OS_POSIX_H_

/*****************************************************************************/

extern ACK_OS posixOs;

/*****************************************************************************/

ACK_PRIVATE LPSTR posixGetcwd(
    LPSTR buf,  /* in */
    SIZE_T size /* in */
);

ACK_PRIVATE LPDIR posixOpendir(
    LPCSTR dirname /* in */
);

ACK_PRIVATE LPDIRENT posixReaddir(
    LPDIR dirp /* in */
);

ACK_PRIVATE INT posixClosedir(
    LPDIR dirp /* in */
);

ACK_PRIVATE BOOL posixIsdir(
    LPCSTR dirname,  /* in */
    LPDIRENT direntp /* in */
);

ACK_PRIVATE ERRNO_T posixFtruncate(
    FILE *stream, /* in */
    INT64 size    /* in */
);

/*****************************************************************************/

#endif /* _OS_POSIX_H_ */

/* end of file */
