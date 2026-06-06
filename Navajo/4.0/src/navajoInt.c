/*
 * navajoInt.c -- Private Engine API
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

#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hresult.h"

#include "navajoPort.h"
#include "navajo.h"
#include "navajoSqlite.h"
#include "navajoIntTypes.h"
#include "navajoCache.h"
#include "navajoInt.h"
#include "navajoSql.h"
#include "navajoUtil.h"

ACK_RESULT getChunkIdAndOffset( /* PRIVATE */
    UINT64 chunkSize,
    UINT64 offset,
    ACK_LPBLOBINFO pBlobInfo
    )
{
    if (pBlobInfo == NULL)
        return E_POINTER;

    if (chunkSize != 0) {
        pBlobInfo->chunkId = (offset / chunkSize);
        pBlobInfo->offset = (offset % chunkSize);
    } else {
        pBlobInfo->chunkId = 0;
        pBlobInfo->offset = offset;
    }

    return ACK_S_OK;
}

ACK_RESULT getChunkName( /* PRIVATE */
    LPCSTR fileName,
    UINT64 chunkId,
    LPSTR *pChunkName
    )
{
    ACK_RESULT kResult = ACK_S_OK;
    SIZE_T size = 0;
    LPSTR strChunkId = NULL;
    LPSTR chunkName = NULL;

    if ((fileName == NULL) || (pChunkName == NULL))
        return E_POINTER;

    if (chunkId != 0) {
        strChunkId = ACK_strdup(ACK_ui64toa(chunkId));

        if (strChunkId == NULL)
            return E_OUTOFMEMORY;

        size += strlen(strChunkId) + 1; /* '_' + "$chunkId" */
    }

    size += 1; /* NUL */
    chunkName = ACK_strdup2(getFileNameOnly(fileName), size);

    if (chunkName == NULL) {
        kResult = E_OUTOFMEMORY;
        goto cleanup;
    }

    if (strChunkId != NULL) {
        ACK_strcat(chunkName, "_");
        ACK_strcat(chunkName, strChunkId);
    }

    if (*pChunkName != NULL)
        ACK_free(*pChunkName);

    *pChunkName = chunkName;

    goto done;

cleanup:
    if (chunkName != NULL)
        ACK_free(chunkName);

done:
    if (strChunkId != NULL)
        ACK_free(strChunkId);

    return kResult;
}

ACK_RESULT getKeyInfo( /* PRIVATE */
    sqlite3 *db,
    LPCSTR keyId,
    ACK_LPINTKEYINFO *ppKeyInfo
    )
{
    ACK_RESULT kResult = ACK_S_OK;
    ACK_LPINTKEYINFO pKeyInfo = NULL;
    sqlite3_stmt *pStmt = NULL;
    int rc = SQLITE_OK;
    LPCSTR fileName = NULL;

    if (db == NULL)
        return E_HANDLE;

    if ((keyId == NULL) || (ppKeyInfo == NULL))
        return E_POINTER;

    if (*ppKeyInfo != NULL)
        return E_MUSTBENULL;

    pKeyInfo = (ACK_LPINTKEYINFO)ACK_zalloc(sizeof(ACK_INTKEYINFO));

    if (pKeyInfo == NULL)
        return E_OUTOFMEMORY;

    rc = prepareAndExecute(db, ACK_GETKEYINFO_SQL, keyId, FALSE, NULL, FALSE,
        NULL, FALSE, NULL, FALSE, &pStmt);

    ACK_CHECK_SQLITE_RC(prepareAndExecute, rc, SQLITE_ROW);

    if (rc != SQLITE_ROW) {
        kResult = HRESULT_FROM_SQLITE(rc);
        goto cleanup;
    }

    pKeyInfo->baseInfo.version = ACK_KEYINFO_VERSION;

    pKeyInfo->baseInfo.bytesTotal =
        sqlite3_column_int64(pStmt, ONEKEYINFO_COLUMN_TOTALBYTES);

    pKeyInfo->baseInfo.bytesUsed =
        sqlite3_column_int64(pStmt, ONEKEYINFO_COLUMN_USEDBYTES);

    pKeyInfo->baseInfo.isEncrypt =
        sqlite3_column_int(pStmt, ONEKEYINFO_COLUMN_ISENCRYPT);

    pKeyInfo->baseInfo.isArchive =
        sqlite3_column_int(pStmt, ONEKEYINFO_COLUMN_ISARCHIVE);

    pKeyInfo->baseInfo.isRevoked =
        sqlite3_column_int(pStmt, ONEKEYINFO_COLUMN_ISREVOKED);

    pKeyInfo->baseInfo.isErasable =
        sqlite3_column_int(pStmt, ONEKEYINFO_COLUMN_ISERASABLE);

    pKeyInfo->baseInfo.keyId = pKeyInfo->baseInfo.aKeyId;
    strncpy(pKeyInfo->baseInfo.keyId,
        (LPSTR)sqlite3_column_text(pStmt, ONEKEYINFO_COLUMN_ID),
        ACK_KEYINFO_MAXID);
    pKeyInfo->baseInfo.keyId[ACK_KEYINFO_MAXID - 1] = '\0';

    pKeyInfo->baseInfo.keySetId = pKeyInfo->baseInfo.aKeySetId;
    strncpy(pKeyInfo->baseInfo.keySetId,
        (LPSTR)sqlite3_column_text(pStmt, ONEKEYINFO_COLUMN_SETID),
        ACK_KEYINFO_MAXID);
    pKeyInfo->baseInfo.keySetId[ACK_KEYINFO_MAXID - 1] = '\0';

    pKeyInfo->baseInfo.keyGroupId = pKeyInfo->baseInfo.aKeyGroupId;
    strncpy(pKeyInfo->baseInfo.keyGroupId,
        (LPSTR)sqlite3_column_text(pStmt, ONEKEYINFO_COLUMN_GROUPID),
        ACK_KEYINFO_MAXID);
    pKeyInfo->baseInfo.keyGroupId[ACK_KEYINFO_MAXID - 1] = '\0';

    pKeyInfo->baseInfo.keyName = pKeyInfo->baseInfo.aKeyName;
    strncpy(pKeyInfo->baseInfo.keyName,
        (LPSTR)sqlite3_column_text(pStmt, ONEKEYINFO_COLUMN_NAME),
        ACK_KEYINFO_MAXNAME);
    pKeyInfo->baseInfo.keyName[ACK_KEYINFO_MAXNAME - 1] = '\0';

    pKeyInfo->blobInfo.chunkId =
        sqlite3_column_int64(pStmt, ONEKEYINFO_COLUMN_CHUNKID);

    pKeyInfo->chunkSize =
        sqlite3_column_int64(pStmt, ONEKEYINFO_COLUMN_CHUNKSIZE);

    pKeyInfo->blobInfo.offset =
        sqlite3_column_int64(pStmt, ONEKEYINFO_COLUMN_OFFSET);

    fileName = (LPSTR)sqlite3_column_text(pStmt, ONEKEYINFO_COLUMN_FILENAME);

    if (fileName != NULL) {
        pKeyInfo->fileName = ACK_strdup(fileName);

        if (pKeyInfo->fileName == NULL) {
            kResult = E_OUTOFMEMORY;
            goto cleanup;
        }

        /*
         * NOTE: Precompute the multi-chunk attach alias for the recorded
         *       chunk; it is retained for diagnostic purposes only and is
         *       not used directly by getKeyBytes(), which derives a fresh
         *       alias for every chunk in the request's plan.
         */

        kResult = getChunkName(pKeyInfo->fileName, pKeyInfo->blobInfo.chunkId,
            &pKeyInfo->dbName);

        if (FAILED(kResult))
            goto cleanup;
    }

    *ppKeyInfo = pKeyInfo;

    goto done;

cleanup:
    if (FAILED(kResult) && (pKeyInfo != NULL))
        freeKeyInfo(pKeyInfo);

done:
    if (pStmt != NULL) {
        rc = sqlite3_finalize(pStmt);
        ACK_ASSERT_SQLITE_RC(sqlite3_finalize, rc, SQLITE_OK);
    }

    return kResult;
}

VOID freeSessionInfo( /* PRIVATE */
    ACK_LPSESSIONINFO pSessionInfo
    )
{
    if (pSessionInfo == NULL)
        return;

    freeAllCachedKeys(pSessionInfo);

    if (pSessionInfo->pKey != NULL) {
        ACK_free(pSessionInfo->pKey);
        pSessionInfo->pKey = NULL;
    }

    if (pSessionInfo->keyFileName != NULL) {
        ACK_free(pSessionInfo->keyFileName);
        pSessionInfo->keyFileName = NULL;
    }

    if (pSessionInfo->keyId != NULL) {
        ACK_free(pSessionInfo->keyId);
        pSessionInfo->keyId = NULL;
    }

    if (pSessionInfo->directory != NULL) {
        ACK_free(pSessionInfo->directory);
        pSessionInfo->directory = NULL;
    }

    if (pSessionInfo->fileName != NULL) {
        ACK_free(pSessionInfo->fileName);
        pSessionInfo->fileName = NULL;
    }

    ACK_unlockMutex(&pSessionInfo->mutex);
    ACK_finalizeMutex(&pSessionInfo->mutex);

    ACK_free(pSessionInfo);
}

VOID freeKeyInfo( /* PRIVATE */
    ACK_LPINTKEYINFO pKeyInfo
    )
{
    if (pKeyInfo == NULL)
        return;

    if (pKeyInfo->dbName != NULL) {
        ACK_free(pKeyInfo->dbName);
        pKeyInfo->dbName = NULL;
    }

    if (pKeyInfo->fileName != NULL) {
        ACK_free(pKeyInfo->fileName);
        pKeyInfo->fileName = NULL;
    }

    if (pKeyInfo->baseInfo.keyId != pKeyInfo->baseInfo.aKeyId) {
        ACK_free(pKeyInfo->baseInfo.keyId);
        pKeyInfo->baseInfo.keyId = pKeyInfo->baseInfo.aKeyId;
    }

    memset(pKeyInfo->baseInfo.aKeyId, 0, sizeof(pKeyInfo->baseInfo.aKeyId));

    if (pKeyInfo->baseInfo.keySetId != pKeyInfo->baseInfo.aKeySetId) {
        ACK_free(pKeyInfo->baseInfo.keySetId);
        pKeyInfo->baseInfo.keySetId = pKeyInfo->baseInfo.aKeySetId;
    }

    memset(pKeyInfo->baseInfo.aKeySetId, 0,
        sizeof(pKeyInfo->baseInfo.aKeySetId));

    if (pKeyInfo->baseInfo.keyGroupId != pKeyInfo->baseInfo.aKeyGroupId) {
        ACK_free(pKeyInfo->baseInfo.keyGroupId);
        pKeyInfo->baseInfo.keyGroupId = pKeyInfo->baseInfo.aKeyGroupId;
    }

    memset(pKeyInfo->baseInfo.aKeyGroupId, 0,
        sizeof(pKeyInfo->baseInfo.aKeyGroupId));

    if (pKeyInfo->baseInfo.keyName != pKeyInfo->baseInfo.aKeyName) {
        ACK_free(pKeyInfo->baseInfo.keyName);
        pKeyInfo->baseInfo.keyName = pKeyInfo->baseInfo.aKeyName;
    }

    memset(pKeyInfo->baseInfo.aKeyName, 0,
        sizeof(pKeyInfo->baseInfo.aKeyName));

    ACK_free(pKeyInfo);
}

ACK_RESULT buildChunkFilePath( /* PRIVATE */
    ACK_LPSESSIONINFO pSessionInfo,
    LPCSTR baseFileName,
    UINT64 chunkId,
    LPSTR *pFilePath
    )
{
    ACK_RESULT kResult = ACK_S_OK;
    LPSTR chunkName = NULL;
    LPSTR filePath = NULL;

    if (pSessionInfo == NULL)
        return E_HANDLE;

    if ((baseFileName == NULL) || (pFilePath == NULL))
        return E_POINTER;

    if (*pFilePath != NULL)
        return E_MUSTBENULL;

    /*
     * NOTE: First, derive the chunk-suffixed file name only (e.g. "key.db"
     *       for chunk 0 or "key.db_3" for chunk 3).
     */

    kResult = getChunkName(baseFileName, chunkId, &chunkName);

    if (FAILED(kResult))
        goto cleanup;

    /*
     * NOTE: When the session has an explicit key directory, use that as the
     *       qualifying directory; otherwise, qualify the chunk file name
     *       relative to the path of the mounted keyset database.
     */

    if (pSessionInfo->directory != NULL) {
        SIZE_T extra = 0;

        extra += strlen(chunkName) + 1; /* '/' OR '\\' */
        filePath = ACK_strdup2(pSessionInfo->directory, extra);

        if (filePath == NULL) {
            kResult = E_OUTOFMEMORY;
            goto cleanup;
        }

#if defined(WIN32)
        ACK_strcat(filePath, "\\");
#else
        ACK_strcat(filePath, "/");
#endif

        ACK_strcat(filePath, chunkName);
    } else {
        filePath = makeQualifiedFileName(pSessionInfo->fileName, chunkName,
            NULL);

        if (filePath == NULL) {
            kResult = E_FAIL;
            goto cleanup;
        }
    }

    *pFilePath = filePath;
    filePath = NULL;

cleanup:
    if (chunkName != NULL)
        ACK_free(chunkName);

    if (filePath != NULL)
        ACK_free(filePath);

    return kResult;
}

ACK_RESULT planChunkRange( /* PRIVATE */
    ACK_LPSESSIONINFO pSessionInfo,
    ACK_LPINTKEYINFO pKeyInfo,
    UINT64 chunkId,
    UINT64 chunkOffset,
    SIZE_T size,
    ACK_LPCHUNKPLAN pPlan
    )
{
    ACK_RESULT kResult = ACK_S_OK;
    UINT64 chunkSize = 0;
    UINT64 currentChunkId = chunkId;
    UINT64 currentChunkOffset = chunkOffset;
    SIZE_T remaining = size;
    UINT64 needed = 1;
    INT capacity = 0;
    INT index = 0;

    if ((pSessionInfo == NULL) || (pKeyInfo == NULL))
        return E_HANDLE;

    if (pPlan == NULL)
        return E_POINTER;

    if ((pPlan->entries != NULL) || (pPlan->count != 0))
        return E_MUSTBENULL;

    chunkSize = pKeyInfo->chunkSize;

    /*
     * NOTE: When the chunk size is zero, the entire key is treated as one
     *       monolithic chunk regardless of the recorded chunk Id (which is
     *       still used as the blob row identifier).  Produce a one-entry
     *       plan that names the unsuffixed base file.
     */

    if (chunkSize == 0) {
        capacity = 1;
    } else {
        UINT64 firstRoom = 0;

        if (currentChunkOffset > chunkSize)
            return ACK_E_CHUNK_OFFSET_OVERFLOW;

        firstRoom = chunkSize - currentChunkOffset;

        if ((UINT64)remaining > firstRoom) {
            UINT64 spillover = (UINT64)remaining - firstRoom;
            needed += (spillover / chunkSize);
            if ((spillover % chunkSize) != 0)
                needed += 1;
        }

        /*
         * NOTE: As a defensive safety margin, allocate room for one extra
         *       chunk past the strict minimum.  This avoids reallocating
         *       should the engine elect to pin the post-read chunk for
         *       offset maintenance.
         */

        needed += 1;

        if (needed > (UINT64)MAX_CHUNK_COUNT)
            return ACK_E_CHUNK_SIZE_OVERFLOW;

        capacity = (INT)needed;
    }

    pPlan->entries = (ACK_LPCHUNKPLAN_ENTRY)ACK_zalloc(
        sizeof(ACK_CHUNKPLAN_ENTRY) * capacity);

    if (pPlan->entries == NULL)
        return E_OUTOFMEMORY;

    pPlan->capacity = capacity;

    /*
     * NOTE: Walk the request, allocating per-chunk entries.  Each entry
     *       takes as many bytes as fit in the remainder of its chunk; the
     *       next entry resumes at offset zero of the following chunk.  In
     *       single-chunk mode the loop produces exactly one entry naming
     *       the unsuffixed base file but referencing the recorded blob
     *       rowid.
     */

    while (remaining > 0) {
        ACK_LPCHUNKPLAN_ENTRY pEntry = NULL;
        SIZE_T take = remaining;
        /*
         * NOTE: The historical naming convention for legacy single-chunk
         *       data uses getChunkName() with the recorded blob rowid
         *       (KeyOffsets.ChunkId), so a key whose Chunks row has Id=1
         *       lives on disk as <base>_1 even when ChunkSize == 0.  We
         *       follow the same convention here so the multi-chunk
         *       planner is naming-compatible with existing keysets.
         */

        UINT64 namingChunkId = currentChunkId;

        if (index >= capacity) {
            kResult = ACK_E_CHUNK_SIZE_OVERFLOW;
            goto cleanup;
        }

        pEntry = &pPlan->entries[index];

        if (chunkSize != 0) {
            UINT64 room = chunkSize - currentChunkOffset;

            if ((UINT64)take > room)
                take = (SIZE_T)room;
        }

        pEntry->chunkId = currentChunkId;
        pEntry->chunkOffset = currentChunkOffset;
        pEntry->length = take;
        pEntry->attached = FALSE;

        kResult = getChunkName(pKeyInfo->fileName, namingChunkId,
            &pEntry->dbName);

        if (FAILED(kResult))
            goto cleanup;

        kResult = buildChunkFilePath(pSessionInfo, pKeyInfo->fileName,
            namingChunkId, &pEntry->fileName);

        if (FAILED(kResult))
            goto cleanup;

        remaining -= take;
        index++;

        if (chunkSize == 0) {
            /*
             * NOTE: Single-chunk mode; the request never advances past
             *       the initial chunk.
             */

            currentChunkOffset += take;
            break;
        }

        currentChunkOffset += take;

        if (currentChunkOffset >= chunkSize) {
            currentChunkId++;
            currentChunkOffset = 0;
        }
    }

    pPlan->count = index;
    pPlan->finalChunkId = currentChunkId;
    pPlan->finalChunkOffset = currentChunkOffset;

    return ACK_S_OK;

cleanup:
    freeChunkPlan(pPlan);

    return kResult;
}

VOID freeChunkPlan( /* PRIVATE */
    ACK_LPCHUNKPLAN pPlan
    )
{
    INT index = 0;

    if (pPlan == NULL)
        return;

    if (pPlan->entries != NULL) {
        for (index = 0; index < pPlan->capacity; index++) {
            ACK_LPCHUNKPLAN_ENTRY pEntry = &pPlan->entries[index];

            if (pEntry->dbName != NULL) {
                ACK_free(pEntry->dbName);
                pEntry->dbName = NULL;
            }

            if (pEntry->fileName != NULL) {
                ACK_free(pEntry->fileName);
                pEntry->fileName = NULL;
            }
        }

        ACK_free(pPlan->entries);
        pPlan->entries = NULL;
    }

    pPlan->count = 0;
    pPlan->capacity = 0;
    pPlan->finalChunkId = 0;
    pPlan->finalChunkOffset = 0;
}

ACK_RESULT attachChunkPlan( /* PRIVATE */
    sqlite3 *db,
    ACK_LPCHUNKPLAN pPlan
    )
{
    ACK_RESULT kResult = ACK_S_OK;
    INT index = 0;

    if (db == NULL)
        return E_HANDLE;

    if (pPlan == NULL)
        return E_POINTER;

    for (index = 0; index < pPlan->count; index++) {
        ACK_LPCHUNKPLAN_ENTRY pEntry = &pPlan->entries[index];

        if (pEntry->attached)
            continue;

        if ((pEntry->fileName == NULL) || (pEntry->dbName == NULL)) {
            kResult = E_POINTER;
            goto cleanup;
        }

        kResult = attachDatabase(db, pEntry->fileName, pEntry->dbName);

        if (FAILED(kResult))
            goto cleanup;

        pEntry->attached = TRUE;
    }

    return ACK_S_OK;

cleanup:
    /*
     * NOTE: An attach in the middle of the plan failed.  Detach any chunks
     *       that were successfully attached before propagating the failure.
     */

    (VOID)detachChunkPlan(db, pPlan);

    return kResult;
}

ACK_RESULT detachChunkPlan( /* PRIVATE */
    sqlite3 *db,
    ACK_LPCHUNKPLAN pPlan
    )
{
    ACK_RESULT kResult = ACK_S_OK;
    ACK_RESULT kFinal = ACK_S_OK;
    INT index = 0;

    if (db == NULL)
        return E_HANDLE;

    if (pPlan == NULL)
        return E_POINTER;

    /*
     * NOTE: Walk the plan in reverse so attach/detach pairs nest cleanly.
     *       Continue past individual detach failures so that as much
     *       cleanup as possible is performed; the first failure encountered
     *       is propagated to the caller.
     */

    for (index = pPlan->count - 1; index >= 0; index--) {
        ACK_LPCHUNKPLAN_ENTRY pEntry = &pPlan->entries[index];

        if (!pEntry->attached)
            continue;

        kResult = detachDatabase(db, pEntry->dbName);
        pEntry->attached = FALSE;

        if (FAILED(kResult) && !FAILED(kFinal))
            kFinal = kResult;
    }

    return kFinal;
}

ACK_RESULT updateChunkOffset( /* PRIVATE */
    sqlite3 *db,
    LPCSTR keyId,
    UINT64 chunkId,
    UINT64 offset,
    SIZE_T size
    )
{
    ACK_RESULT kResult = ACK_S_OK;
    LPSTR strChunkId = NULL;
    LPSTR strOffset = NULL;
    LPSTR strSize = NULL;
    int rc = SQLITE_OK;

    if (db == NULL)
        return E_HANDLE;

    if (keyId == NULL)
        return E_POINTER;

    /*
     * NOTE: ACK_ui64toa returns a pointer to a static buffer; capture each
     *       distinct value with ACK_strdup so the next conversion does not
     *       overwrite a value still needed.
     */

    strChunkId = ACK_strdup(ACK_ui64toa(chunkId));

    if (strChunkId == NULL) {
        kResult = E_OUTOFMEMORY;
        goto cleanup;
    }

    strOffset = ACK_strdup(ACK_ui64toa(offset));

    if (strOffset == NULL) {
        kResult = E_OUTOFMEMORY;
        goto cleanup;
    }

    strSize = ACK_strdup(ACK_ui64toa((UINT64)size));

    if (strSize == NULL) {
        kResult = E_OUTOFMEMORY;
        goto cleanup;
    }

    rc = prepareAndExecute(db, ACK_UPDATE_CHUNKOFFSET_SQL, strChunkId, FALSE,
        strOffset, FALSE, strSize, FALSE, keyId, FALSE, NULL);

    ACK_CHECK_SQLITE_RC(prepareAndExecute, rc, SQLITE_DONE);

    if (rc != SQLITE_DONE) {
        kResult = HRESULT_FROM_SQLITE(rc);
        goto cleanup;
    }

cleanup:
    if (strSize != NULL)
        ACK_free(strSize);

    if (strOffset != NULL)
        ACK_free(strOffset);

    if (strChunkId != NULL)
        ACK_free(strChunkId);

    return kResult;
}

ACK_RESULT getKeyBytes( /* PRIVATE */
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
    ACK_RESULT kRollbackResult = ACK_S_OK;
    ACK_RESULT kDetachResult = ACK_S_OK;
    sqlite3 *db = NULL;
    ACK_LPINTKEYINFO pKeyInfo = NULL;
    LPBYTE pKey = NULL;
    BOOL allocated = FALSE;
    UINT64 bytesTotal = 0;
    UINT64 startChunkId = 0;
    UINT64 startChunkOffset = 0;
    UINT64 globalStartOffset = 0;
    ACK_CHUNKPLAN plan;
    BOOL transaction = FALSE;
    SIZE_T cursor = 0;
    INT index = 0;
    sqlite3_blob *pBlob = NULL;
    int rc = SQLITE_OK;

    memset(&plan, 0, sizeof(ACK_CHUNKPLAN));

    if (pSessionInfo == NULL)
        return E_HANDLE;

    if ((keyId == NULL) || (pOffset == NULL) || (ppKey == NULL) ||
            (pKeySize == NULL)) {
        return E_POINTER;
    }

    db = pSessionInfo->db;

    if (db == NULL)
        return ACK_E_NO_MOUNTED_DATABASE;

    /*
     * NOTE: ATTACH and DETACH cannot occur within a SQLite transaction, so
     *       reject calls that arrive with one already active.  This also
     *       protects against re-entrant use of the database handle.
     */

    if (pSessionInfo->transaction)
        return ACK_E_TRANSACTION_PENDING;

    /*
     * NOTE: Make sure they are requesting a valid quantity of key bytes.
     *       Validate before allocating the output buffer to avoid leaks.
     */

    if (size == 0)
        return E_INVALIDARG;
    else if (size > INT_MAX)
        return E_OVERFLOW;

    if ((*ppKey == NULL) || (*pKeySize < size)) {
        /*
         * NOTE: The caller did not provide a valid buffer or it is too
         *       small.  Attempt to allocate an output buffer now.
         */

        pKey = (LPBYTE)ACK_zalloc(sizeof(BYTE) * size);

        if (pKey == NULL)
            return E_OUTOFMEMORY;

        /*
         * NOTE: In case of failure, we need to know that we actually
         *       allocated the output buffer (i.e. it was not provided by
         *       the caller) and we must free it.
         */

        allocated = TRUE;
    } else {
        pKey = *ppKey;
    }

    /*
     * NOTE: Execute a query to fetch all the metadata about the requested
     *       key.
     */

    kResult = getKeyInfo(db, keyId, &pKeyInfo);

    if (FAILED(kResult))
        goto cleanup;

    /*
     * NOTE: Make sure the key is an encryption key if the session type
     *       requires it.
     */

    if (encrypt && !pKeyInfo->baseInfo.isEncrypt) {
        kResult = ACK_E_KEY_IS_DECRYPT_ONLY;
        goto cleanup;
    }

    /*
     * NOTE: Make sure the key is not revoked.
     */

    if (pKeyInfo->baseInfo.isRevoked) {
        kResult = ACK_E_KEY_IS_REVOKED;
        goto cleanup;
    }

    /*
     * NOTE: Sanity check.  Make sure the total size of the key is larger
     *       than the requested number of bytes.  If this fails there is no
     *       reason to proceed any further.
     */

    bytesTotal = pKeyInfo->baseInfo.bytesTotal;

    if (bytesTotal < size) {
        kResult = ACK_E_KEY_IS_TOO_SMALL;
        goto cleanup;
    }

    if (encrypt) {
        UINT64 bytesUsed = pKeyInfo->baseInfo.bytesUsed + size;

        if (bytesUsed > bytesTotal) {
            kResult = ACK_E_INSUFFICIENT_KEY_BYTES_REMAIN;
            goto cleanup;
        }

        /*
         * NOTE: Begin from the recorded chunk and intra-chunk offset for
         *       this key.
         */

        startChunkId = pKeyInfo->blobInfo.chunkId;
        startChunkOffset = pKeyInfo->blobInfo.offset;
    } else {
        /*
         * NOTE: When decrypting, the caller supplies the global offset
         *       (matching the value previously returned by ACK_Encrypt).
         *       Translate it to (chunkId, chunkOffset) using the recorded
         *       chunk size.  When the chunk size is zero, the recorded
         *       chunk Id is preserved and the caller-supplied offset is
         *       used verbatim as the intra-chunk offset.
         */

        if ((*pOffset + size) > bytesTotal) {
            kResult = ACK_E_OFFSET_PLUS_SIZE_EXCEEDS_TOTAL;
            goto cleanup;
        }

        if (pKeyInfo->chunkSize != 0) {
            ACK_BLOBINFO derived;

            memset(&derived, 0, sizeof(ACK_BLOBINFO));

            kResult = getChunkIdAndOffset(pKeyInfo->chunkSize, *pOffset,
                &derived);

            if (FAILED(kResult))
                goto cleanup;

            startChunkId = derived.chunkId;
            startChunkOffset = derived.offset;
        } else {
            startChunkId = pKeyInfo->blobInfo.chunkId;
            startChunkOffset = *pOffset;
        }
    }

    /*
     * NOTE: Capture the request's starting global offset before producing
     *       the per-chunk plan; this is what the caller will record as the
     *       cipher-text offset (encrypt path).
     */

    if (pKeyInfo->chunkSize != 0)
        globalStartOffset = (startChunkId * pKeyInfo->chunkSize) +
            startChunkOffset;
    else
        globalStartOffset = startChunkOffset;

    /*
     * NOTE: Build the per-chunk plan for the request.  When the request
     *       spans a chunk boundary the plan contains multiple entries;
     *       otherwise it degenerates to a single entry.
     */

    kResult = planChunkRange(pSessionInfo, pKeyInfo, startChunkId,
        startChunkOffset, size, &plan);

    if (FAILED(kResult))
        goto cleanup;

    /*
     * NOTE: ATTACH every chunk database referenced by the plan.  This must
     *       happen outside any active SQLite transaction.
     */

    kResult = attachChunkPlan(db, &plan);

    if (FAILED(kResult))
        goto cleanup;

    /*
     * NOTE: For the encryption path an exclusive transaction is required
     *       so that the offset advancement is atomic with respect to other
     *       writers.  The decryption path is read-only and needs none.
     */

    if (encrypt) {
        kResult = beginTransaction(db);

        if (FAILED(kResult))
            goto cleanup;

        transaction = TRUE;
        pSessionInfo->transaction = TRUE;
    }

    /*
     * NOTE: Read each chunk in turn into the output buffer at the running
     *       cursor.
     */

    for (index = 0; index < plan.count; index++) {
        ACK_LPCHUNKPLAN_ENTRY pEntry = &plan.entries[index];

        if (pEntry->chunkOffset > INT_MAX) {
            kResult = ACK_E_KEY_OFFSET_OVERFLOW;
            goto cleanup;
        }

        if (pEntry->length > INT_MAX) {
            kResult = E_OVERFLOW;
            goto cleanup;
        }

        rc = sqlite3_blob_open(db, pEntry->dbName, ACK_KEY_CHUNK_TABLE,
            ACK_KEY_BLOB_COLUMN, (sqlite3_int64)pEntry->chunkId,
            0 /* READ-ONLY */, &pBlob);

        ACK_CHECK_SQLITE_RC(sqlite3_blob_open, rc, SQLITE_OK);

        if (rc != SQLITE_OK) {
            kResult = HRESULT_FROM_SQLITE(rc);
            goto cleanup;
        }

        rc = sqlite3_blob_read(pBlob, pKey + cursor, (int)pEntry->length,
            (int)pEntry->chunkOffset);

        ACK_CHECK_SQLITE_RC(sqlite3_blob_read, rc, SQLITE_OK);

        if (rc != SQLITE_OK) {
            kResult = HRESULT_FROM_SQLITE(rc);
            goto cleanup;
        }

        rc = sqlite3_blob_close(pBlob);
        pBlob = NULL;

        ACK_CHECK_SQLITE_RC(sqlite3_blob_close, rc, SQLITE_OK);

        if (rc != SQLITE_OK) {
            kResult = HRESULT_FROM_SQLITE(rc);
            goto cleanup;
        }

        cursor += pEntry->length;
    }

    /*
     * NOTE: For the encryption path, advance the recorded (ChunkId, Offset)
     *       to the post-read position and accumulate the used-bytes
     *       counter.
     */

    if (encrypt) {
        kResult = updateChunkOffset(db, keyId, plan.finalChunkId,
            plan.finalChunkOffset, size);

        if (FAILED(kResult))
            goto cleanup;

        kResult = commitTransaction(db);

        if (FAILED(kResult))
            goto cleanup;

        transaction = FALSE;
        pSessionInfo->transaction = FALSE;
    }

#if defined(FEATURE_LOW_MEMORY)
    sqlite3_release_memory(-1);
#endif

    /*
     * NOTE: Detach the chunk databases now that the transactional work is
     *       complete.  Detach must happen outside any active transaction.
     */

    kDetachResult = detachChunkPlan(db, &plan);

    if (FAILED(kDetachResult)) {
        kResult = kDetachResult;
        goto cleanup;
    }

    if (encrypt)
        *pOffset = globalStartOffset;

    if (allocated) {
        if (*ppKey != NULL)
            ACK_free(*ppKey);

        *ppKey = pKey;
        *pKeySize = size;
    }

    goto done;

cleanup:
    if (pBlob != NULL) {
        rc = sqlite3_blob_close(pBlob);
        ACK_ASSERT_SQLITE_RC(sqlite3_blob_close, rc, SQLITE_OK);
    }

    if (transaction) {
        kRollbackResult = rollbackTransaction(db);
        ACK_ASSERT(SUCCEEDED(kRollbackResult) && "rollbackTransaction");
        (VOID)kRollbackResult; /* NOTE: silence unused-but-set in non-debug. */

        pSessionInfo->transaction = FALSE;
    }

    (VOID)detachChunkPlan(db, &plan);

    if (allocated && (pKey != NULL))
        ACK_free(pKey);

done:
    freeChunkPlan(&plan);

    if (pKeyInfo != NULL)
        freeKeyInfo(pKeyInfo);

    return kResult;
}

/* end of file */
