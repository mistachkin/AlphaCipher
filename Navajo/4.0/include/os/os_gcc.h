/*
 * os_gcc.h --
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

#ifndef _OS_GCC_H_
#define _OS_GCC_H_

/*****************************************************************************/

extern ACK_OS gccOs;

/*****************************************************************************/

ACK_PRIVATE SIZE_T gccMsize(
    LPVOID pBlock /* in */
);

ACK_PRIVATE VOID gccFree(
    LPVOID pBlock /* in */
);

/*****************************************************************************/

#endif /* _OS_GCC_H_ */

/* end of file */
