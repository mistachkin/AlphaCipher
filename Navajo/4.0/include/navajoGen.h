/*
 * navajoGen.h -- Public Key Generation API
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

#if !defined(_NAVAJO_H_)
#error "The header file \"navajo.h\" must be included prior to this file."
#endif

#if !defined(_NAVAJO_INT_H_)
#error "The header file \"navajoInt.h\" must be included prior to this file."
#endif

#ifndef _NAVAJO_GEN_H_
#define _NAVAJO_GEN_H_

/*****************************************************************************/

#ifndef ACK_CHUNK_VERSION
#define ACK_CHUNK_VERSION                    (1)
#endif

/*****************************************************************************/

#if defined(FEATURE_IMPORT)
ACK_API ACK_RESULT ACK_PrepareChunk(
    ACK_SESSION session, /* in */
    INT64 version,       /* in */
    LPVOID *ppReserved,  /* in, out: RESERVED, must be NULL. */
    UINT64 chunkId,      /* in */
    UINT64 size          /* in */
);

ACK_API ACK_RESULT ACK_ImportChunk(
    ACK_SESSION session, /* in */
    INT64 version,       /* in */
    LPVOID *ppReserved,  /* in, out: RESERVED, must be NULL. */
    UINT64 chunkId,      /* in */
    UINT64 offset,       /* in */
    LPSTR fileName,      /* in */
    UINT64 size          /* in */
);
#endif /* defined(FEATURE_IMPORT) */

/*****************************************************************************/

#endif /* _NAVAJO_GEN_H_ */

/* end of file */
