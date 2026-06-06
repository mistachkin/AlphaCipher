/*
 * navajoSqlite.h -- Private SQLite Header
 *
 * Copyright (c) 2009-2026 by Joe Mistachkin.  All rights reserved.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * written by: Joe Mistachkin
 *
 * RCS: @(#) $Id: $
 */

#ifndef _NAVAJO_SQLITE_H_
#define _NAVAJO_SQLITE_H_

#if !defined(SQLITE_STATIC_LIB)
    /*
     * NOTE: We are not including SQLite as a static library.  Therefore, we
     *       must make sure to decorate the public SQLite API with DLL import
     *       attributes.
     */

    #ifndef SQLITE_API
        #if defined(WIN32)
            #if defined(_MSC_VER)
                #define SQLITE_API __declspec(dllimport)
            #elif defined(__GNUC__) && !defined(NO_VIZ)
                #define SQLITE_API __attribute__((dllimport))
            #else
                #define SQLITE_API
            #endif
        #else
            #define SQLITE_API
        #endif
    #endif
#endif

#include "sqlite3.h"

#endif /* _NAVAJO_SQLITE_H_ */

/* end of file */
