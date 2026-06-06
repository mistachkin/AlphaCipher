/*
 * navajoCache.c -- Private Cache API
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

#if defined(FEATURE_RESET_CACHED_KEYS)
#include <stdio.h>
#include <stdlib.h>
#endif /* defined(FEATURE_RESET_CACHED_KEYS) */

#include <limits.h>
#include <stddef.h>
#include <string.h>

#include "navajoPort.h"
#include "navajo.h"
#include "navajoSqlite.h"

#if defined(FEATURE_RESET_CACHED_KEYS)
#include "navajoSql.h"
#endif /* defined(FEATURE_RESET_CACHED_KEYS) */

#include "navajoIntTypes.h"
#include "navajoCache.h"
#include "navajoInt.h"

#if defined(FEATURE_RESET_CACHED_KEYS)
#include "navajoUtil.h"
#endif /* defined(FEATURE_RESET_CACHED_KEYS) */

#include "hresult.h"

BOOL isCacheEnabled( /* PRIVATE */
    ACK_LPSESSIONINFO pSessionInfo,
    ACK_SESSIONTYPE type
    )
{
    return (((pSessionInfo != NULL) && (pSessionInfo->cacheKeySize > 0)) ||
        ((type & ACKST_Stream) && (type & (ACKST_Encrypt | ACKST_Decrypt))));
}

ACK_LPCACHEDKEY findCachedKey( /* PRIVATE */
    ACK_LPSESSIONINFO pSessionInfo,
    LPCSTR keyId,
    BOOL encrypt
    )
{
    INT index = 0;

    if ((pSessionInfo == NULL) || (keyId == NULL))
        return NULL;

    if (pSessionInfo->pCache == NULL)
        return NULL;

    for (index = 0; index < pSessionInfo->nCache; index++) {
        ACK_LPCACHEDKEY pCachedKey = &pSessionInfo->pCache[index];

        /*
         * NOTE: See if the current cached key entry matches the desired
         *       criteria.  We only match decrypt keys if the caller is
         *       requesting decryption.
         */

        if (pCachedKey->inUse && (pCachedKey->keyId != NULL) &&
                (strcmp(pCachedKey->keyId, keyId) == 0) &&
                (encrypt == pCachedKey->isEncrypt)) {
            /*
             * NOTE: We found a key with the matching Id and operation
             *       affinity; therefore, return it.
             */

            return pCachedKey;
        }
    }

    return NULL;
}

ACK_RESULT newCachedKey( /* PRIVATE */
    ACK_LPSESSIONINFO pSessionInfo,
    LPCSTR keyId,
    SIZE_T size,
    BOOL encrypt,
    LPUINT64 pOffset,
    ACK_LPCACHEDKEY *ppCachedKey
    )
{
    ACK_RESULT kResult = ACK_S_OK;
    INT index = 0;

    if (pSessionInfo == NULL)
        return E_HANDLE;

    if ((keyId == NULL) || (pOffset == NULL) || (ppCachedKey == NULL))
        return E_POINTER;

    if (*ppCachedKey != NULL)
        return E_MUSTBENULL;

    /*
     * NOTE: Make sure they are requesting a valid quantity of key bytes.
     */

    if (size == 0)
        return E_INVALIDARG;
    else if (size > INT_MAX)
        return E_OVERFLOW;
    else if (size > pSessionInfo->cacheKeySize)
        return ACK_E_CACHE_IS_TOO_SMALL;

    if (pSessionInfo->pCache == NULL)
        return ACK_E_NO_CACHE;

    /*
     * NOTE: Locate the next empty cached key entry in the session.
     */

    for (index = 0; index < pSessionInfo->nCache; index++) {
        ACK_LPCACHEDKEY pCachedKey = &pSessionInfo->pCache[index];

        if (!pCachedKey->inUse) {
            pCachedKey->size = pSessionInfo->cacheKeySize;

            /*
             * NOTE: pCachedKey->blobInfo.offset is treated as a global
             *       key offset (not an intra-chunk offset).  The chunk
             *       routing happens inside getKeyBytes(), which fills in
             *       the correct value via the out-parameter for encrypt
             *       and leaves the caller-supplied value untouched for
             *       decrypt.  The blobInfo.chunkId field is unused here.
             */

            pCachedKey->blobInfo.offset = ACK_GET_OFFSET2(encrypt,
                0 /* NOT USED */, pOffset);

            kResult = getKeyBytes(pSessionInfo, keyId, pCachedKey->size,
                encrypt, &pCachedKey->blobInfo.offset, &pCachedKey->pKey,
                &pCachedKey->size);

            if (FAILED(kResult)) {
                freeCachedKey(pSessionInfo, pCachedKey);
            } else {
                pCachedKey->keyId = pCachedKey->aKeyId;

                strncpy(pCachedKey->keyId, keyId, ACK_KEYINFO_MAXID);
                pCachedKey->keyId[ACK_KEYINFO_MAXID - 1] = '\0';

                pCachedKey->inUse = TRUE;
                pCachedKey->isEncrypt = encrypt;

                pCachedKey->workSize = pCachedKey->size;
                pCachedKey->workOffset = 0;

                *ppCachedKey = pCachedKey;

                if (encrypt)
                    *pOffset = pCachedKey->blobInfo.offset;
            }

            return kResult;
        }
    }

    return ACK_E_NO_STORAGE;
}

VOID freeCachedKey( /* PRIVATE */
    ACK_LPSESSIONINFO pSessionInfo,
    ACK_LPCACHEDKEY pCachedKey
    )
{
#if defined(FEATURE_RESET_CACHED_KEYS)
    ACK_RESULT kResult = ACK_S_OK;
    ACK_RESULT kRollbackResult = ACK_S_OK;
    sqlite3 *db = NULL;
    BOOL transaction = FALSE;
    ACK_LPINTKEYINFO pKeyInfo = NULL;
    UINT64 offset = 0;
    UINT64 recordedGlobal = 0;
    UINT64 newGlobal = 0;
    UINT64 newChunkId = 0;
    UINT64 newChunkOffset = 0;
    UINT64 newUsedBytes = 0;
    LPSTR strChunkId = NULL;
    LPSTR strChunkOffset = NULL;
    LPSTR strUsedBytes = NULL;
    int rc = SQLITE_OK;
#else
    /*
     * NOTE: The first argument to this function is not used unless the "reset
     *       cached key offset" feature is enabled.
     */

    UNUSED_ARGUMENT(pSessionInfo);
#endif /* defined(FEATURE_RESET_CACHED_KEYS) */

    if (pCachedKey == NULL)
        return;

#if defined(FEATURE_RESET_CACHED_KEYS)
    /*
     * NOTE: If caching key bytes is enabled for this session, attempt to
     *       recover the unused portion of the key bytes for this encryption
     *       key.
     */

    if (pSessionInfo == NULL)
        goto skip;

    if (!pCachedKey->inUse || !pCachedKey->isEncrypt)
        goto skip;

    if (!isCacheEnabled(pSessionInfo, pSessionInfo->type))
        goto skip;

    db = pSessionInfo->db;

    if (db == NULL)
        goto skip;

    /*
     * NOTE: If the caller did not explicitly start a transaction, do so now.
     *       In that case, the caller will be expected to commit the
     *       transaction before returning.
     */

    if (!pSessionInfo->transaction) {
        /*
         * NOTE: We are starting this transaction and now we must rollback upon
         *       failure.  We must do this now because we now need a consistent
         *       view of the key information for the duration of this function.
         */

        kResult = beginTransaction(db);

        if (!FAILED(kResult))
            pSessionInfo->transaction = transaction = TRUE;
        else
            goto skip;
    }

    kResult = getKeyInfo(db, pCachedKey->keyId, &pKeyInfo);

    if (FAILED(kResult))
        goto cleanup;

    /*
     * NOTE: Figure out if the current recorded encryption offset for this key
     *       matches what it was when we initially filled this cache entry.  If
     *       they match we can be 100% sure we were the last ones to allocate
     *       bytes from this encryption key; otherwise, something else has been
     *       using it and we can do nothing to recover the unused bytes from
     *       what we allocated.
     */

    /*
     * NOTE: Compute the recorded current global offset for this key.  In
     *       multi-chunk mode it is the sum of (recorded chunk * chunk
     *       size) + recorded intra-chunk offset; in single-chunk mode
     *       the recorded intra-chunk offset is itself the global value.
     */

    if (pKeyInfo->chunkSize != 0)
        recordedGlobal = (pKeyInfo->blobInfo.chunkId *
            pKeyInfo->chunkSize) + pKeyInfo->blobInfo.offset;
    else
        recordedGlobal = pKeyInfo->blobInfo.offset;

    /*
     * NOTE: Figure out if the current recorded encryption offset for this
     *       key matches what it was when we initially filled this cache
     *       entry.  If they match we can be 100% sure we were the last
     *       ones to allocate bytes from this encryption key; otherwise,
     *       something else has been using it and we can do nothing to
     *       recover the unused bytes from what we allocated.
     */

    offset = pCachedKey->blobInfo.offset + pCachedKey->workOffset +
        pCachedKey->workSize;

    if (offset != recordedGlobal)
        goto cleanup;

    /*
     * NOTE: We are going to reset the offset to account for the number
     *       of key bytes that have actually been used while it has been
     *       cached in memory.  At this point, the database is locked by
     *       this thread; therefore, we can be certain the recorded
     *       offset has not been changed out from under us.
     */

    newGlobal = pCachedKey->blobInfo.offset + pCachedKey->workOffset;

    if (pKeyInfo->chunkSize != 0) {
        newChunkId = newGlobal / pKeyInfo->chunkSize;
        newChunkOffset = newGlobal % pKeyInfo->chunkSize;
    } else {
        newChunkId = pKeyInfo->blobInfo.chunkId;
        newChunkOffset = newGlobal;
    }

    newUsedBytes = pKeyInfo->baseInfo.bytesUsed - pCachedKey->workSize;

    strChunkId = ACK_strdup(ACK_ui64toa(newChunkId));

    if (strChunkId == NULL) {
        kResult = E_OUTOFMEMORY;
        goto cleanup;
    }

    strChunkOffset = ACK_strdup(ACK_ui64toa(newChunkOffset));

    if (strChunkOffset == NULL) {
        kResult = E_OUTOFMEMORY;
        goto cleanup;
    }

    strUsedBytes = ACK_strdup(ACK_ui64toa(newUsedBytes));

    if (strUsedBytes == NULL) {
        kResult = E_OUTOFMEMORY;
        goto cleanup;
    }

    rc = prepareAndExecute(db, ACK_RESET_OFFSET_SQL, strChunkId, FALSE,
        strChunkOffset, FALSE, strUsedBytes, FALSE, pCachedKey->keyId,
        FALSE, NULL);

    ACK_CHECK_SQLITE_RC(prepareAndExecute, rc, SQLITE_DONE);

    if (rc != SQLITE_DONE) {
        kResult = HRESULT_FROM_SQLITE(rc);
        goto cleanup;
    }

    /*
     * NOTE: If we started the transaction locally and it is still active,
     *       commit it now.
     */

    if (transaction && pSessionInfo->transaction) {
        kResult = commitTransaction(db);

        if (FAILED(kResult))
            goto cleanup;

        pSessionInfo->transaction = FALSE;
    }

    goto done;

cleanup:
    if ((db != NULL) && transaction && pSessionInfo->transaction) {
        kRollbackResult = rollbackTransaction(db);
        ACK_ASSERT(SUCCEEDED(kRollbackResult) && "rollbackTransaction");
        (VOID)kRollbackResult; /* NOTE: silence unused-but-set in non-debug. */

        pSessionInfo->transaction = FALSE;
    }

done:
    if (strUsedBytes != NULL)
        ACK_free(strUsedBytes);

    if (strChunkOffset != NULL)
        ACK_free(strChunkOffset);

    if (strChunkId != NULL)
        ACK_free(strChunkId);

    if (pKeyInfo != NULL)
        freeKeyInfo(pKeyInfo);

skip:
#endif /* defined(FEATURE_RESET_CACHED_KEYS) */

    if (pCachedKey->pKey != NULL) {
        ACK_free(pCachedKey->pKey);
        pCachedKey->pKey = NULL;
    }

    if (pCachedKey->keyId != pCachedKey->aKeyId) {
        ACK_free(pCachedKey->keyId);
        pCachedKey->keyId = pCachedKey->aKeyId;
    }

    memset(pCachedKey->aKeyId, 0, sizeof(pCachedKey->aKeyId));

    pCachedKey->inUse = FALSE;
}

VOID freeAllCachedKeys( /* PRIVATE */
    ACK_LPSESSIONINFO pSessionInfo
    )
{
    INT index = 0;

    if (pSessionInfo == NULL)
        return;

    if (pSessionInfo->pCache == NULL) {
        pSessionInfo->nCache = 0;
        return;
    }

    for (index = 0; index < pSessionInfo->nCache; index++)
        freeCachedKey(pSessionInfo, &pSessionInfo->pCache[index]);

    pSessionInfo->nCache = 0;

    ACK_free(pSessionInfo->pCache);
    pSessionInfo->pCache = NULL;
}

ACK_RESULT getCachedKeyBytes( /* PRIVATE */
    ACK_LPSESSIONINFO pSessionInfo,
    LPCSTR keyId,
    SIZE_T size,
    BOOL encrypt,
    LPUINT64 pOffset,
    LPBYTE *ppKey,
    LPSIZE_T pKeySize
    )
{
    ACK_RESULT kResult = ACK_S_OK;
    ACK_LPCACHEDKEY pCachedKey = NULL;
    LPBYTE pKey = NULL;
    BOOL allocated = FALSE;
    UINT64 offset = 0;

    if (pSessionInfo == NULL)
        return E_HANDLE;

    if ((keyId == NULL) || (pOffset == NULL) || (ppKey == NULL) ||
            (pKeySize == NULL))
        return E_POINTER;

    /*
     * NOTE: Make sure they are requesting a valid quantity of key bytes.
     */

    if (size == 0)
        return E_INVALIDARG;
    else if (size > INT_MAX)
        return E_OVERFLOW;
    else if (size > pSessionInfo->cacheKeySize)
        goto fallback;

    /*
     * NOTE: Next, before doing anything else, make sure we can actually get a
     *       block of memory large enough to hold the entire requested size.
     */

    if ((*ppKey == NULL) || (*pKeySize < size)) {
        /*
         * NOTE: The caller did not provide a valid buffer or it is too small.
         *       Attempt to allocate an output buffer now.
         */

        pKey = (LPBYTE)ACK_zalloc(sizeof(BYTE) * size);

        if (pKey == NULL)
            return E_OUTOFMEMORY;

        /*
         * NOTE: In case of failure, we need to know that we actually allocated
         *       the output buffer (i.e. it was not provided by the caller) and
         *       we must free it.
         */

        allocated = TRUE;
    } else {
        pKey = *ppKey;
    }

    /*
     * NOTE: Attempt to locate the necessary key entry in the cache.
     */

    pCachedKey = findCachedKey(pSessionInfo, keyId, encrypt);

    /*
     * NOTE: Make sure the cached key has some bytes remaining.
     *
     * TODO: For now, forbid mixing cached and fresh key bytes.  If there are
     *       not enough cached key bytes to satisfy the request, throw them out
     *       and refill the cache from the database.  In the future, we will
     *       want to allow mixing cached and fresh key bytes even though it
     *       makes the caching implementation slightly more complex.
     */

    if ((pCachedKey != NULL) && (pCachedKey->workSize < size)) {
        /*
         * NOTE: No point in wasting encrypt key bytes that we have already
         *       allocated from the database; therefore, only clear the cache
         *       entry here if we are in decrypt mode UNLESS the "reset cached
         *       key offset" feature is enabled (i.e. in that case no key bytes
         *       are actually wasted).
         */

#if !defined(FEATURE_RESET_CACHED_KEYS)
        if (!encrypt)
#endif /* !defined(FEATURE_RESET_CACHED_KEYS) */
            freeCachedKey(pSessionInfo, pCachedKey);

        pCachedKey = NULL;
    }

    /*
     * NOTE: If we could not find the key in the cache (or it was invalid),
     *       create a new entry and use it.
     */

    if (pCachedKey == NULL) {
        offset = ACK_GET_OFFSET2(encrypt, 0 /* NOT USED */, pOffset);

        kResult = newCachedKey(pSessionInfo, keyId, size, encrypt, &offset,
            &pCachedKey);

        if (FAILED(kResult))
            goto fallback;

        /*
         * NOTE: Sanity check.  Make sure we have (or fetched) enough key
         *       bytes to satisfy the request.  This will be false if they
         *       requested more key bytes than will fit into the cache
         *       entry.
         */

        if ((pCachedKey != NULL) && (pCachedKey->workSize < size)) {
            /*
             * NOTE: No point in wasting brand new encrypt key bytes that we
             *       have just allocated from the database; therefore, only
             *       clear the cache entry here if we are in decrypt mode
             *       UNLESS the "reset cached key offset" feature is enabled
             *       (i.e. in that case no key bytes are actually wasted).
             */

#if !defined(FEATURE_RESET_CACHED_KEYS)
            if (!encrypt)
#endif /* !defined(FEATURE_RESET_CACHED_KEYS) */
                freeCachedKey(pSessionInfo, pCachedKey);

            pCachedKey = NULL;

            goto fallback;
        }
    }

    /*
     * NOTE: Make sure we have a valid cached key entry now.  If not, we will
     *       have to fallback to using the database directly.
     */

    if (pCachedKey != NULL) {
        /*
         * NOTE: Are we handling an encryption or decryption request?
         */

        if (encrypt) {
            /*
             * NOTE: Calculate the "real" key offset for use by the caller
             *       into our local variable.  This value will be committed
             *       to the variable provided by the caller when we fully
             *       succeed (below).
             */

            offset = pCachedKey->blobInfo.offset + pCachedKey->workOffset;

            /*
             * NOTE: Fill our locally allocated key buffer with key bytes
             *       from the cache now.  This cannot fail unless the cache
             *       has been corrupted by an external mechanism.
             */

            ACK_ASSERT(pCachedKey->workOffset < pCachedKey->size);
            memcpy(pKey, pCachedKey->pKey + pCachedKey->workOffset, size);

            /*
             * NOTE: Adjust the number of key bytes remaining in the cached
             *       key.  If it reaches zero, clear the entry; otherwise,
             *       update the working offset for the next request.
             */

            pCachedKey->workSize -= size;
            ACK_ASSERT(pCachedKey->workSize >= 0);

            if (pCachedKey->workSize == 0)
                freeCachedKey(NULL, pCachedKey);
            else
                pCachedKey->workOffset += size;

            /*
             * NOTE: All-or-nothing semantics.  At this point, we have
             *       succeeded.  Commit changes now to the variables
             *       provided by the caller now that we know for certain
             *       that we have fully succeeded.
             */

            if (allocated) {
                if (*ppKey != NULL)
                    ACK_free(*ppKey);

                *ppKey = pKey;
                *pKeySize = size;
            }

            *pOffset = offset;

            goto done;
        } else {
            /*
             * NOTE: Grab the offset provided by the caller now as we need it
             *       several times below.
             */

            offset = *pOffset;

            if (offset >= pCachedKey->blobInfo.offset) {
                /*
                 * NOTE: Calculate the "real" key offset for use by the caller
                 *       into our local variable.  This value will be committed
                 *       to the variable provided by the caller when we fully
                 *       succeed (below).
                 */

                offset -= pCachedKey->blobInfo.offset;

                if ((offset + size) < pCachedKey->workSize) {
                    /*
                     * NOTE: Fill our locally allocated key buffer with key
                     *       bytes from the cache now.  This cannot fail unless
                     *       the cache has been corrupted by an external
                     *       mechanism.
                     */

                    memcpy(pKey, pCachedKey->pKey + offset, size);

                    /*
                     * NOTE: All-or-nothing semantics.  At this point, we have
                     *       succeeded.  Commit changes now to the variables
                     *       provided by the caller now that we know for
                     *       certain that we have fully succeeded.
                     */

                    if (allocated) {
                        if (*ppKey != NULL)
                            ACK_free(*ppKey);

                        *ppKey = pKey;
                        *pKeySize = size;
                    }

                    goto done;
                }
            }
        }
    }

fallback:
    if (allocated && (pKey != NULL))
        ACK_free(pKey);

    /*
     * NOTE: If we get to this point for any reason, we must assume that the
     *       request could not be satified using the cache; therefore, get the
     *       key bytes directly from the database.
     */

    kResult = getKeyBytes(pSessionInfo, keyId, size, encrypt, pOffset, ppKey,
        pKeySize);

done:
    return kResult;
}

/* end of file */
