/*
 * os_wince.h --
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

#ifndef _OS_WINCE_H_
#define _OS_WINCE_H_

/*****************************************************************************/

extern ACK_OS winceOs;

/*****************************************************************************/

ACK_PRIVATE LPDIR winceOpendir(
    LPCSTR dirname /* in */
);

ACK_PRIVATE LPDIRENT winceReaddir(
    LPDIR dirp /* in */
);

ACK_PRIVATE INT winceClosedir(
    LPDIR dirp /* in */
);

ACK_PRIVATE BOOL winceIsdir(
    LPCSTR dirname,  /* in */
    LPDIRENT direntp /* in */
);

ACK_PRIVATE UINT64 winceAtoull(
    LPCSTR str /* in */
);

/*****************************************************************************/

#endif /* _OS_WINCE_H_ */

/* end of file */
