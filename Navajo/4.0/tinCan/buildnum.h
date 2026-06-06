/*
 * buildnum.h -- Product name and build numbers.
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

#ifndef _BUILDNUM_H_
#define _BUILDNUM_H_

#define PRODUCT_COMPANY       "Joe Mistachkin"
#define PRODUCT_COPYRIGHT     "Copyright © 2008-2012 by " PRODUCT_COMPANY \
                              "  All rights reserved."

#define PRODUCT_NAME          "AlphaCipher Wave Library"

#define PRODUCT_VERSION       "2.0.4215.23344"
#define PRODUCT_VERSION_FIXED 2,0,4215,23344

#define FILE_NAME             "TinCan.dll"

#define FILE_VERSION          "2.0.4215.23344"
#define FILE_VERSION_FIXED    2,0,4215,23344

#if defined(_DEBUG)
#define FILE_CONFIGURATION    "debug"
#elif defined(NDEBUG)
#define FILE_CONFIGURATION    "release"
#else
#define FILE_CONFIGURATION    "unknown"
#endif

#define FILE_DESCRIPTION      PRODUCT_NAME " for Windows"

#endif /* _BUILDNUM_H_ */

/* end of file */
