/*
 * navajoCache.h -- Private Cache API
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

#if !defined(_NAVAJO_H_)
#error "The header file \"navajo.h\" must be included prior to this file."
#endif

#if !defined(_NAVAJO_INT_TYPES_H_)
#error "The header file \"navajoIntTypes.h\" must be included prior to this file."
#endif

#ifndef _NAVAJO_CACHE_H_
#define _NAVAJO_CACHE_H_

/*****************************************************************************/

/*
 * NOTE: These numbers (and possibly the underlying caching algorithm itself)
 *       will require careful hand-tuning for optimum embedded performance.
 *       However, no changes should be made without running the appropriate
 *       benchmarks before and after the change and comparing the results for
 *       all supported scenarios.
 */

#ifndef DEFAULT_CACHE_KEYS
    #if !defined(__SYMBIAN32__)
        #define DEFAULT_CACHE_KEYS           (2)
    #else
        #define DEFAULT_CACHE_KEYS           (1)
    #endif
#endif

#ifndef DEFAULT_CACHE_KEY_SIZE
    #if !defined(__SYMBIAN32__)
        #define DEFAULT_CACHE_KEY_SIZE (1048576)
    #else
        #define DEFAULT_CACHE_KEY_SIZE   (32768)
    #endif
#endif

/*****************************************************************************/

ACK_PRIVATE BOOL isCacheEnabled(
    ACK_LPSESSIONINFO pSessionInfo, /* in */
    ACK_SESSIONTYPE type            /* in */
);

ACK_PRIVATE ACK_LPCACHEDKEY findCachedKey(
    ACK_LPSESSIONINFO pSessionInfo, /* in */
    LPCSTR keyId,                   /* in */
    BOOL encrypt                    /* in */
);

ACK_PRIVATE ACK_RESULT newCachedKey(
    ACK_LPSESSIONINFO pSessionInfo, /* in */
    LPCSTR keyId,                   /* in */
    SIZE_T size,                    /* in */
    BOOL encrypt,                   /* in */
    LPUINT64 pOffset,               /* in, out */
    ACK_LPCACHEDKEY *ppCachedKey    /* in, out */
);

ACK_PRIVATE VOID freeCachedKey(
    ACK_LPSESSIONINFO pSessionInfo, /* in */
    ACK_LPCACHEDKEY pCachedKey      /* in */
);

ACK_PRIVATE VOID freeAllCachedKeys(
    ACK_LPSESSIONINFO pSessionInfo /* in */
);

ACK_PRIVATE ACK_RESULT getCachedKeyBytes( /* NOTE: Also see getKeyBytes. */
    ACK_LPSESSIONINFO pSessionInfo, /* in */
    LPCSTR keyId,                   /* in */
    SIZE_T size,                    /* in */
    BOOL encrypt,                   /* in */
    LPUINT64 pOffset,               /* in, out */
    LPBYTE *ppKey,                  /* in, out */
    LPSIZE_T pKeySize               /* in, out */
);

/*****************************************************************************/

#endif /* _NAVAJO_CACHE_H_ */

/* end of file */
