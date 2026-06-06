/*
 * os_win32.h --
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

#ifndef _OS_WIN32_H_
#define _OS_WIN32_H_

/*****************************************************************************/

extern ACK_OS win32Os;

/*****************************************************************************/

ACK_PRIVATE VOID win32InitializeMutex(
    ACK_LPMUTEX pMutex /* in */
);

ACK_PRIVATE VOID win32FinalizeMutex(
    ACK_LPMUTEX pMutex /* in */
);

ACK_PRIVATE VOID win32LockMutex(
    ACK_LPMUTEX pMutex /* in */
);

ACK_PRIVATE VOID win32UnlockMutex(
    ACK_LPMUTEX pMutex /* in */
);

/*****************************************************************************/

#endif /* _OS_WIN32_H_ */

/* end of file */
