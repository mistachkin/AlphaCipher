/*
 * navajoGen.c -- Public Key Generation API
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

#include "hresult.h"

#include "navajoPort.h"
#include "navajo.h"
#include "navajoSqlite.h"
#include "navajoIntTypes.h"
#include "navajoCache.h"
#include "navajoInt.h"
#include "navajoGen.h"
#include "navajoSql.h"
#include "navajoUtil.h"

#if defined(FEATURE_IMPORT)
ACK_RESULT ACK_PrepareChunk( /* PUBLIC */
    ACK_SESSION session,
    INT64 version,
    LPVOID *ppReserved,
    UINT64 chunkId,
    UINT64 size
    )
{
    ACK_RESULT kResult = ACK_S_OK;
    ACK_RESULT kRollbackResult = ACK_S_OK;
    ACK_LPSESSIONINFO pSessionInfo = (ACK_LPSESSIONINFO)session;
    sqlite3 *db = NULL;
    sqlite3 *keyDb = NULL;
    int rc = SQLITE_OK;
    LPSTR strSize = NULL;
    LPSTR strChunkId = NULL;

    if (pSessionInfo == NULL)
        return E_HANDLE;

    if (version != ACK_CHUNK_VERSION)
        return ACK_E_UNSUPPORTED_VERSION;

    if (ppReserved != NULL)
        return E_MUSTBENULL;

    if (chunkId == 0)
        return ACK_E_INVALID_CHUNK_ID;

    if (size == 0)
        return E_INVALIDARG;
    else if (size > INT_MAX)
        return E_OVERFLOW;
    else if (size > MAX_CHUNK_SIZE)
        return ACK_E_MAXIMUM_KEY_SIZE_EXCEEDED;

    if (!(pSessionInfo->type & ACKST_PrepareKey))
        return ACK_E_WRONG_SESSION_TYPE;

    if (pSessionInfo->keyId == NULL)
        return ACK_E_NO_KEY_FOR_SESSION;

    if (pSessionInfo->keyFileName == NULL)
        return ACK_E_NO_KEY_DATABASE_FOR_SESSION;

    db = pSessionInfo->db;

    if (db == NULL)
        return ACK_E_NO_MOUNTED_DATABASE;

    rc = sqlite3_open_v2(pSessionInfo->keyFileName, &keyDb,
        SQLITE_OPEN_READWRITE, VFS_NAME);

    ACK_CHECK_SQLITE_RC(sqlite3_open_v2, rc, SQLITE_OK);

    if (rc != SQLITE_OK) {
        kResult = HRESULT_FROM_SQLITE(rc);
        goto cleanup;
    }

    kResult = beginTransaction(keyDb);

    if (SUCCEEDED(kResult))
        pSessionInfo->transaction = TRUE;
    else
        goto cleanup;

    strSize = ACK_strdup(ACK_ui64toa(size));

    if (strSize == NULL) {
        kResult = E_OUTOFMEMORY;
        goto cleanup;
    }

    strChunkId = ACK_strdup(ACK_ui64toa(chunkId));

    if (strChunkId == NULL) {
        kResult = E_OUTOFMEMORY;
        goto cleanup;
    }

    rc = prepareAndExecute(keyDb, ACK_UPDATE_ZEROBLOB_SQL, strSize, FALSE,
        strChunkId, FALSE, strSize, FALSE, NULL, FALSE, NULL);

    ACK_CHECK_SQLITE_RC(prepareAndExecute, rc, SQLITE_DONE);

    if (rc != SQLITE_DONE) {
        kResult = HRESULT_FROM_SQLITE(rc);
        goto cleanup;
    }

    if (pSessionInfo->transaction) {
        kResult = commitTransaction(keyDb);

        if (FAILED(kResult))
            goto cleanup;

        pSessionInfo->transaction = FALSE;
    }

    goto done;

cleanup:
    if ((keyDb != NULL) && pSessionInfo->transaction) {
        kRollbackResult = rollbackTransaction(keyDb);
        ACK_ASSERT(SUCCEEDED(kRollbackResult) && "rollbackTransaction");
        (VOID)kRollbackResult; /* NOTE: silence unused-but-set in non-debug. */

        pSessionInfo->transaction = FALSE;
    }

done:
    if (strChunkId != NULL)
        ACK_free(strChunkId);

    if (strSize != NULL)
        ACK_free(strSize);

    if (keyDb != NULL) {
        rc = sqlite3_close(keyDb);
        ACK_ASSERT_SQLITE_RC(sqlite3_close, rc, SQLITE_OK);
    }

    return kResult;
}

ACK_RESULT ACK_ImportChunk( /* PUBLIC */
    ACK_SESSION session,
    INT64 version,
    LPVOID *ppReserved,
    UINT64 chunkId,
    UINT64 offset,
    LPSTR fileName,
    UINT64 size
    )
{
    ACK_RESULT kResult = ACK_S_OK;
    ACK_RESULT kRollbackResult = ACK_S_OK;
    ACK_LPSESSIONINFO pSessionInfo = (ACK_LPSESSIONINFO)session;
    sqlite3 *db = NULL;
    FILE *stream = NULL;
    INT64 endOffset = 0;
    sqlite3 *keyDb = NULL;
    int rc = SQLITE_OK;
    LPSTR strSize = NULL;
    LPSTR strChunkId = NULL;
    sqlite3_blob *pBlob = NULL;
    UINT64 readSize = 0;
    LPBYTE buffer = NULL;
    SIZE_T numRead = 0;
    ERRNO_T errorNo = 0;

    if (pSessionInfo == NULL)
        return E_HANDLE;

    if (version != ACK_CHUNK_VERSION)
        return ACK_E_UNSUPPORTED_VERSION;

    if (ppReserved != NULL)
        return E_MUSTBENULL;

    if (fileName == NULL)
        return E_POINTER;

    if (chunkId == 0)
        return ACK_E_INVALID_CHUNK_ID;

    if (offset > INT_MAX)
        return ACK_E_KEY_OFFSET_OVERFLOW;

    if (size == 0)
        return E_INVALIDARG;
    else if (size > INT_MAX)
        return E_OVERFLOW;
    else if (size > MAX_CHUNK_SIZE)
        return ACK_E_MAXIMUM_KEY_SIZE_EXCEEDED;

    if ((offset + size) > INT_MAX)
        return E_OVERFLOW;

    if (!(pSessionInfo->type & ACKST_ImportKey))
        return ACK_E_WRONG_SESSION_TYPE;

    if (pSessionInfo->keyId == NULL)
        return ACK_E_NO_KEY_FOR_SESSION;

    if (pSessionInfo->keyFileName == NULL)
        return ACK_E_NO_KEY_DATABASE_FOR_SESSION;

    db = pSessionInfo->db;

    if (db == NULL)
        return ACK_E_NO_MOUNTED_DATABASE;

    stream = fopen(fileName, "r+b"); /* RDWR BINARY */

    if (stream == NULL)
        return HRESULT_FROM_ERRNO(ACK_errno());

    if (ACK_fseek(stream, 0, SEEK_END) != 0) {
        kResult = HRESULT_FROM_ERRNO(ACK_errno());
        goto cleanup;
    }

    endOffset = ACK_ftell(stream);

    if (endOffset < (INT64)size) {
        kResult = ACK_E_INSUFFICIENT_KEY_BYTES_REMAIN;
        goto cleanup;
    }

    rc = sqlite3_open_v2(pSessionInfo->keyFileName, &keyDb,
        SQLITE_OPEN_READWRITE, VFS_NAME);

    ACK_CHECK_SQLITE_RC(sqlite3_open_v2, rc, SQLITE_OK);

    if (rc != SQLITE_OK) {
        kResult = HRESULT_FROM_SQLITE(rc);
        goto cleanup;
    }

    kResult = beginTransaction(keyDb);

    if (SUCCEEDED(kResult))
        pSessionInfo->transaction = TRUE;
    else
        goto cleanup;

    strSize = ACK_strdup(ACK_ui64toa(size));

    if (strSize == NULL) {
        kResult = E_OUTOFMEMORY;
        goto cleanup;
    }

    strChunkId = ACK_strdup(ACK_ui64toa(chunkId));

    if (strChunkId == NULL) {
        kResult = E_OUTOFMEMORY;
        goto cleanup;
    }

    rc = prepareAndExecute(keyDb, ACK_UPDATE_ZEROBLOB_SQL, strSize, FALSE,
        strChunkId, FALSE, strSize, FALSE, NULL, FALSE, NULL);

    ACK_CHECK_SQLITE_RC(prepareAndExecute, rc, SQLITE_DONE);

    if (rc != SQLITE_DONE) {
        kResult = HRESULT_FROM_SQLITE(rc);
        goto cleanup;
    }

    rc = sqlite3_blob_open(keyDb, NULL, ACK_KEY_CHUNK_TABLE,
        ACK_KEY_BLOB_COLUMN, chunkId, 1 /* READ-WRITE */, &pBlob);

    ACK_CHECK_SQLITE_RC(sqlite3_blob_open, rc, SQLITE_OK);

    if (rc != SQLITE_OK) {
        kResult = HRESULT_FROM_SQLITE(rc);
        goto cleanup;
    }

    readSize = ACK_MAXIMUM_READ_SIZE;
    ACK_ASSERT(readSize <= INT_MAX);

    if (readSize > size)
        readSize = size;

    buffer = (LPBYTE)ACK_zalloc(sizeof(BYTE) * (SIZE_T)readSize);

    if (buffer == NULL) {
        kResult = E_OUTOFMEMORY;
        goto cleanup;
    }

    while (size > 0) {
        ACK_ASSERT(readSize > 0); ACK_ASSERT(readSize <= INT_MAX);

        if (ACK_fseek(stream, -((INT64)readSize), SEEK_END) != 0) {
            kResult = HRESULT_FROM_ERRNO(ACK_errno());
            goto cleanup;
        }

        numRead = fread(buffer, sizeof(BYTE), (SIZE_T)readSize, stream);

        if (numRead != readSize) {
            if (ferror(stream))
                kResult = HRESULT_FROM_ERRNO(ACK_errno());
            else if (feof(stream))
                kResult = ACK_E_INSUFFICIENT_KEY_BYTES_REMAIN;
            else
                kResult = ACK_E_IO_ERROR;

            goto cleanup;
        }

        endOffset -= numRead;
        ACK_ASSERT(endOffset >= 0);

        errorNo = ACK_ftruncate(stream, endOffset);

        if (errorNo != 0) {
            kResult = HRESULT_FROM_ERRNO(errorNo);
            goto cleanup;
        }

        rc = sqlite3_blob_write(pBlob, buffer, numRead, (int)offset);

        ACK_CHECK_SQLITE_RC(sqlite3_blob_write, rc, SQLITE_OK);

        if (rc != SQLITE_OK) {
            kResult = HRESULT_FROM_SQLITE(rc);
            goto cleanup;
        }

        offset += numRead;
        ACK_ASSERT(offset <= INT_MAX);

        size -= numRead;

        if (size < readSize)
            readSize = size;
    }

    rc = sqlite3_blob_close(pBlob);

    ACK_CHECK_SQLITE_RC(sqlite3_blob_close, rc, SQLITE_OK);

    if (rc == SQLITE_OK) {
        pBlob = NULL;
    } else {
        kResult = HRESULT_FROM_SQLITE(rc);
        goto cleanup;
    }

    if (pSessionInfo->transaction) {
        kResult = commitTransaction(keyDb);

        if (FAILED(kResult))
            goto cleanup;

        pSessionInfo->transaction = FALSE;
    }

    goto done;

cleanup:
    if (pBlob != NULL) {
        rc = sqlite3_blob_close(pBlob);
        ACK_ASSERT_SQLITE_RC(sqlite3_blob_close, rc, SQLITE_OK);
    }

    if ((keyDb != NULL) && pSessionInfo->transaction) {
        kRollbackResult = rollbackTransaction(keyDb);
        ACK_ASSERT(SUCCEEDED(kRollbackResult) && "rollbackTransaction");
        (VOID)kRollbackResult; /* NOTE: silence unused-but-set in non-debug. */

        pSessionInfo->transaction = FALSE;
    }

done:
    if (buffer != NULL)
        ACK_free(buffer);

    if (strChunkId != NULL)
        ACK_free(strChunkId);

    if (strSize != NULL)
        ACK_free(strSize);

    if (keyDb != NULL) {
        rc = sqlite3_close(keyDb);
        ACK_ASSERT_SQLITE_RC(sqlite3_close, rc, SQLITE_OK);
    }

    if (stream != NULL) {
        rc = fclose(stream);
        ACK_ASSERT((rc == 0) && "fclose");
        (VOID)rc; /* NOTE: silence unused-but-set in non-debug. */
    }

    return kResult;
}
#endif /* defined(FEATURE_IMPORT) */

/* end of file */
