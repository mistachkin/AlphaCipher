/*
 * hresult.h --
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

#ifndef _HRESULT_H_
#define _HRESULT_H_

#ifndef _HRESULT_DEFINED
#define _HRESULT_DEFINED
/*
 * NOTE: HRESULT must be exactly 32 bits wide so the high (sign) bit
 *       carries failure status as on Windows.  On 64-bit Unix targets
 *       'long' is 64 bits, which would silently inflate error codes
 *       to positive values and break SUCCEEDED()/FAILED().
 */
#if defined(WIN32) || defined(_WIN32_WCE)
typedef long HRESULT;
#else
typedef int HRESULT;
#endif
#endif

#ifndef SUCCEEDED
#define SUCCEEDED(hr) ((HRESULT)(hr) >= 0)
#endif

#ifndef FAILED
#define FAILED(hr) ((HRESULT)(hr) < 0)
#endif

#ifndef FACILITY_ANSI_CRT
#define FACILITY_ANSI_CRT                     42
#endif

#ifndef FACILITY_SQLITE
#define FACILITY_SQLITE                     1967
#endif

#ifndef FACILITY_SHARED
#define FACILITY_SHARED                       99
#endif

#ifndef HRESULT_FROM_ERRNO
#define HRESULT_FROM_ERRNO(x) \
    ((HRESULT)(x) <= 0 ? ((HRESULT)(x)) : \
    ((HRESULT) (((x) & 0x0000FFFF) | \
        (FACILITY_ANSI_CRT << 16) | 0x80000000)))
#endif

#ifndef HRESULT_FROM_SQLITE
#define HRESULT_FROM_SQLITE(x) \
    ((HRESULT)(x) <= 0 ? ((HRESULT)(x)) : \
    ((HRESULT) (((x) & 0x0000FFFF) | \
        (FACILITY_SQLITE << 16) | 0x80000000)))
#endif

#ifndef S_OK
#define S_OK                       (0x00000000L)
#endif

#ifndef S_FALSE
#define S_FALSE                    (0x00000001L)
#endif

#ifndef E_PENDING
#define E_PENDING                  (0x8000000AL)
#endif

#ifndef E_NOTIMPL
#define E_NOTIMPL                  (0x80004001L)
#endif

#ifndef E_NOINTERFACE
#define E_NOINTERFACE              (0x80004002L)
#endif

#ifndef E_POINTER
#define E_POINTER                  (0x80004003L)
#endif

#ifndef E_ABORT
#define E_ABORT                    (0x80004004L)
#endif

#ifndef E_FAIL
#define E_FAIL                     (0x80004005L)
#endif

#ifndef E_UNEXPECTED
#define E_UNEXPECTED               (0x8000FFFFL)
#endif

#ifndef E_ACCESSDENIED
#define E_ACCESSDENIED             (0x80070005L)
#endif

#ifndef E_HANDLE
#define E_HANDLE                   (0x80070006L)
#endif

#ifndef E_OUTOFMEMORY
#define E_OUTOFMEMORY              (0x8007000EL)
#endif

#ifndef E_INVALIDARG
#define E_INVALIDARG               (0x80070057L)
#endif

#ifndef E_BASE
#define E_BASE                     (0xA0630000L)
#endif

#ifndef E_MUSTBENULL
#define E_MUSTBENULL               (E_BASE + 1L)
#endif

#ifndef E_MUSTBEZERO
#define E_MUSTBEZERO               (E_BASE + 2L)
#endif

#ifndef E_NOHANDLE
#define E_NOHANDLE                 (E_BASE + 3L)
#endif

#ifndef E_NOTFOUND
#define E_NOTFOUND                 (E_BASE + 4L)
#endif

#ifndef E_FOUND
#define E_FOUND                    (E_BASE + 5L)
#endif

#ifndef E_BADMAGIC
#define E_BADMAGIC                 (E_BASE + 6L)
#endif

#ifndef E_BADVERSION
#define E_BADVERSION               (E_BASE + 7L)
#endif

#ifndef E_BADOFFSET
#define E_BADOFFSET                (E_BASE + 8L)
#endif

#ifndef E_BADSIZE
#define E_BADSIZE                  (E_BASE + 9L)
#endif

#ifndef E_OVERFLOW
#define E_OVERFLOW                (E_BASE + 10L)
#endif

#ifndef E_BOUNDS
#define E_BOUNDS                  (E_BASE + 11L)
#endif

#ifndef E_STARTED
#define E_STARTED                 (E_BASE + 12L)
#endif

#ifndef E_STOPPED
#define E_STOPPED                 (E_BASE + 13L)
#endif

#ifndef E_LOCK
#define E_LOCK                    (E_BASE + 14L)
#endif

#ifndef E_TOOLARGE
#define E_TOOLARGE                (E_BASE + 15L)
#endif

#endif /* _HRESULT_H_ */

/* end of file */
