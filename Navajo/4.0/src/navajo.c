/*
 * navajo.c -- Public Engine API
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

#include "buildnum.h"
#include "hresult.h"

#include "navajoPort.h"
#include "navajo.h"
#include "navajoSqlite.h"
#include "navajoSql.h"
#include "navajoIntTypes.h"
#include "navajoCache.h"
#include "navajoInt.h"
#include "navajoUtil.h"
#include "os.h"

ACK_LPOS pCurrentOs = NULL;


ACK_RESULT ACK_Initialize( /* PUBLIC */
    VOID
    )
{
    return osInitialize(&pCurrentOs, &currentOs, NULL, NULL);
}

ACK_RESULT ACK_Finalize( /* PUBLIC */
    VOID
    )
{
    return osFinalize(&pCurrentOs);
}

LPCSTR ACK_GetVersion( /* PUBLIC */
    VOID
    )
{
    /*
     * BUGBUG: This is not thread-safe (static buffer).
     */

    static CHAR buffer[MAX_VERSION_SPACE + 1];

    snprintf(buffer, MAX_VERSION_SPACE,
        "%s v%s (%s/%s) using SQLite v%s [%s]%c",
        FILE_DESCRIPTION, FILE_VERSION,
        FILE_PLATFORM, FILE_CONFIGURATION,
        sqlite3_libversion(), sqlite3_sourceid(),
        /* NUL */ '\0');

    buffer[MAX_VERSION_SPACE] = '\0';

    return buffer;
}

LPVOID ACK_malloc( /* PUBLIC */
    SIZE_T size
    )
{
    return osMalloc(pCurrentOs, size);
}

LPVOID ACK_zalloc( /* PUBLIC */
    SIZE_T size
    )
{
    return osZalloc(pCurrentOs, size);
}

LPVOID ACK_realloc( /* PUBLIC */
    LPVOID pBlock,
    SIZE_T size,
    BOOL zero
    )
{
    return osRealloc(pCurrentOs, pBlock, size, zero);
}

SIZE_T ACK_msize( /* PUBLIC */
    LPVOID pBlock
    )
{
    return osMsize(pCurrentOs, pBlock);
}

VOID ACK_free( /* PUBLIC */
    LPVOID pBlock
    )
{
    osFree(pCurrentOs, pBlock);
}

ACK_RESULT ACK_CreateSession( /* PUBLIC */
    ACK_LPSESSION pSession,
    ACK_SESSIONTYPE type,
    SIZE_T cacheSize
    )
{
    ACK_RESULT kResult = ACK_S_OK;
    ACK_LPSESSIONINFO pSessionInfo = NULL;
    BOOL useCache = FALSE;

    if (pSession == NULL)
        return E_POINTER;

    if (*pSession != NULL)
        return E_MUSTBENULL;

    if (cacheSize > INT_MAX)
        return E_OVERFLOW;
    else if (cacheSize > MAX_CHUNK_SIZE)
        return ACK_E_MAXIMUM_KEY_SIZE_EXCEEDED;

#if defined(FEATURE_AUTO_INITIALIZE)
    kResult = ACK_Initialize();

    if (FAILED(kResult))
        return kResult;
#endif /* defined(FEATURE_AUTO_INITIALIZE) */

    pSessionInfo = (ACK_LPSESSIONINFO)ACK_zalloc(sizeof(ACK_SESSIONINFO));

    if (pSessionInfo == NULL)
        return E_OUTOFMEMORY;

    ACK_initializeMutex(&pSessionInfo->mutex);
    ACK_lockMutex(&pSessionInfo->mutex);

    /*
     * NOTE: Should we use caching mechanisms?
     */

    useCache = isCacheEnabled(NULL, type);

    if (useCache) {
        /*
         * NOTE: Set the cached key size for this session to the value provided
         *       by the caller unless that value is zero.  In that case, use
         *       the default cached key size.
         */

        if (cacheSize != 0)
            pSessionInfo->cacheKeySize = cacheSize;
        else
            pSessionInfo->cacheKeySize = DEFAULT_CACHE_KEY_SIZE;

        /*
         * NOTE: Create the temporary key buffer to potentially prevent another
         *       repeated malloc/free in the critical encrypt/decrypt code path.
         */

        pSessionInfo->keySize = pSessionInfo->cacheKeySize;
        pSessionInfo->pKey = (LPBYTE)ACK_zalloc(
            sizeof(BYTE) * pSessionInfo->keySize);

        if (pSessionInfo->pKey == NULL) {
            kResult = E_OUTOFMEMORY;
            goto cleanup;
        }
    }

    pSessionInfo->pSession = pSession;
    pSessionInfo->type = type;

    if (useCache) {
        /*
         * NOTE: For certain session types, speed is critical; therefore,
         *       enable the key cache and set it to the default size.
         */

        INT nCache = DEFAULT_CACHE_KEYS;
        INT index;

        pSessionInfo->pCache = (ACK_LPCACHEDKEY)ACK_zalloc(
            sizeof(ACK_CACHEDKEY) * nCache);

        if (pSessionInfo->pCache == NULL) {
            kResult = E_OUTOFMEMORY;
            goto cleanup;
        }

        pSessionInfo->nCache = nCache;

        for (index = 0; index < nCache; index++) {
            ACK_LPCACHEDKEY pCachedKey = &pSessionInfo->pCache[index];

            /*
             * NOTE: Prepare the cached key entry for use.
             */

            pCachedKey->keyId = pCachedKey->aKeyId;
        }
    }

    *pSession = (ACK_SESSION)pSessionInfo;

    goto done;

cleanup:
    freeSessionInfo(pSessionInfo);

done:
    if (SUCCEEDED(kResult) && (pSessionInfo != NULL))
        ACK_unlockMutex(&pSessionInfo->mutex);

    return kResult;
}

ACK_RESULT ACK_CloseSession( /* PUBLIC */
    ACK_LPSESSION pSession
    )
{
    ACK_RESULT kResult = ACK_S_OK;
    ACK_LPSESSIONINFO pSessionInfo = NULL;

    if (pSession == NULL)
        return E_POINTER;

    pSessionInfo = (ACK_LPSESSIONINFO)*pSession;

    if (pSessionInfo == NULL)
        return E_HANDLE;

    ACK_lockMutex(&pSessionInfo->mutex);

    if (!(pSessionInfo->type & ACKST_Managed) &&
            (pSessionInfo->pSession != pSession)) {
        kResult = ACK_E_MISMATCHED_SESSION;
        goto done;
    }

    if (pSessionInfo->db != NULL) {
        kResult = ACK_E_MOUNTED_DATABASE;
        goto done;
    }

    freeSessionInfo(pSessionInfo);

    *pSession = NULL;

#if defined(FEATURE_AUTO_FINALIZE)
    kResult = ACK_Finalize();

    if (FAILED(kResult))
        return kResult;
#endif /* defined(FEATURE_AUTO_FINALIZE) */

done:
    if (FAILED(kResult) && (pSessionInfo != NULL))
        ACK_unlockMutex(&pSessionInfo->mutex);

    return kResult;
}

ACK_RESULT ACK_IsSessionOk( /* PUBLIC */
    ACK_LPSESSION pSession
    )
{
    ACK_RESULT kResult = ACK_S_OK;
    ACK_LPSESSIONINFO pSessionInfo = NULL;
    sqlite3 *db = NULL;
    sqlite3_stmt *pStmt = NULL;
    int rc = SQLITE_OK;
    LPCSTR value = NULL;

    if (pSession == NULL)
        return E_POINTER;

    pSessionInfo = (ACK_LPSESSIONINFO)*pSession;

    if (pSessionInfo == NULL)
        return E_HANDLE;

    ACK_lockMutex(&pSessionInfo->mutex);

    if (!(pSessionInfo->type & ACKST_Managed) &&
            (pSessionInfo->pSession != pSession)) {
        kResult = ACK_E_MISMATCHED_SESSION;
        goto done;
    }

    db = pSessionInfo->db;

    if (db == NULL) {
        kResult = ACK_E_NO_MOUNTED_DATABASE;
        goto done;
    }

    rc = prepareAndExecute(db, ACK_VERIFY_DATABASE_SQL, NULL, FALSE, NULL,
        FALSE, NULL, FALSE, NULL, FALSE, &pStmt);

    ACK_CHECK_SQLITE_RC(prepareAndExecute, rc, SQLITE_ROW);

    if (rc != SQLITE_ROW) {
        kResult = HRESULT_FROM_SQLITE(rc);
        goto done;
    }

    /*
     * NOTE: Should be the literal string "ok" if everything has passed.
     */

    value = (LPCSTR)sqlite3_column_text(pStmt, VERIFYDATABASE_COLUMN_STATUS);

    if ((value == NULL) || strcmp(value, VERIFYDATABASE_STATUS_VALUE) != 0)
        kResult = ACK_E_SESSION_CHECK_FAILED;

done:
    if (pStmt != NULL) {
        rc = sqlite3_finalize(pStmt);
        ACK_ASSERT_SQLITE_RC(sqlite3_finalize, rc, SQLITE_OK);
    }

    ACK_unlockMutex(&pSessionInfo->mutex);

    return kResult;
}

ACK_RESULT ACK_MountKeySet( /* PUBLIC */
    ACK_SESSION session,
    LPSTR database
    )
{
    ACK_RESULT kResult = ACK_S_OK;
    ACK_LPSESSIONINFO pSessionInfo = (ACK_LPSESSIONINFO)session;
    LPSTR fileName = NULL;
    LPSTR zVfs = VFS_NAME;
    sqlite3 *db = NULL;
    LPSTR zSql = NULL;
    int rc = SQLITE_OK;

    if (pSessionInfo == NULL)
        return E_HANDLE;

    if (database == NULL)
        return E_POINTER;

    ACK_lockMutex(&pSessionInfo->mutex);

#if defined(__SYMBIAN32__)
    kResult = symbianDbInit(pSessionInfo, database, zVfs);

    if (FAILED(kResult))
        goto cleanup;
#endif /* defined(__SYMBIAN32__) */

#if defined(FEATURE_AUTO_REMOUNT)
    /*
     * NOTE: If there is an existing keyset mounted for this session, attempt
     *       to close it now.  If that attempt fails, we cannot continue.
     */

    if (pSessionInfo->db != NULL) {
        rc = sqlite3_close(pSessionInfo->db);

        ACK_CHECK_SQLITE_RC(sqlite3_close, rc, SQLITE_OK);

        if (rc == SQLITE_OK) {
            pSessionInfo->db = NULL;
        } else {
            kResult = HRESULT_FROM_SQLITE(rc);
            goto cleanup;
        }
    }
#endif /* defined(FEATURE_AUTO_REMOUNT) */

    /*
     * NOTE: Make sure there is no keyset currently mounted for this session.
     */

    if (pSessionInfo->db != NULL) {
        kResult = ACK_E_MOUNTED_DATABASE;
        goto cleanup;
    }

    fileName = ACK_strdup(database);

    if (fileName == NULL) {
        kResult = E_OUTOFMEMORY;
        goto cleanup;
    }

    /*
     * NOTE: Attempt to open the specified keyset database.
     */

    rc = sqlite3_open_v2(database, &db, SQLITE_OPEN_READWRITE, zVfs);

    ACK_CHECK_SQLITE_RC(sqlite3_open_v2, rc, SQLITE_OK);

    if (rc != SQLITE_OK) {
        kResult = HRESULT_FROM_SQLITE(rc);
        goto cleanup;
    }

    zSql = ACK_MOUNT_KEYSET_SQL;

    if (zSql != NULL) {
        rc = prepareAndExecute(db, zSql, NULL, FALSE, NULL, FALSE, NULL, FALSE,
            NULL, FALSE, NULL);

        if (rc == SQLITE_DONE)
            rc = SQLITE_OK;

        ACK_CHECK_SQLITE_RC(prepareAndExecute, rc, SQLITE_OK);

        if (rc != SQLITE_OK) {
            kResult = HRESULT_FROM_SQLITE(rc);
            goto cleanup;
        }
    }

    pSessionInfo->fileName = fileName;
    pSessionInfo->db = db;

    goto done;

cleanup:
    if (fileName != NULL)
        ACK_free(fileName);

done:
    ACK_unlockMutex(&pSessionInfo->mutex);

    return kResult;
}

ACK_RESULT ACK_UnmountKeySet( /* PUBLIC */
    ACK_SESSION session
    )
{
    ACK_RESULT kResult = ACK_S_OK;
    ACK_LPSESSIONINFO pSessionInfo = (ACK_LPSESSIONINFO)session;
    sqlite3 *db = NULL;
    LPSTR zSql = NULL;
    int rc = SQLITE_OK;

    if (pSessionInfo == NULL)
        return E_HANDLE;

    ACK_lockMutex(&pSessionInfo->mutex);

    db = pSessionInfo->db;

    if (db == NULL) {
        kResult = ACK_E_NO_MOUNTED_DATABASE;
        goto done;
    }

    freeAllCachedKeys(pSessionInfo);

    zSql = ACK_UNMOUNT_KEYSET_SQL;

    if (zSql != NULL) {
        rc = prepareAndExecute(db, zSql, NULL, FALSE, NULL, FALSE, NULL, FALSE,
            NULL, FALSE, NULL);

        if (rc == SQLITE_DONE)
            rc = SQLITE_OK;

        ACK_CHECK_SQLITE_RC(prepareAndExecute, rc, SQLITE_OK);

        if (rc != SQLITE_OK) {
            kResult = HRESULT_FROM_SQLITE(rc);
            goto done;
        }
    }

    rc = sqlite3_close(db);

    ACK_CHECK_SQLITE_RC(sqlite3_close, rc, SQLITE_OK);

    if (rc != SQLITE_OK) {
        kResult = HRESULT_FROM_SQLITE(rc);
        goto done;
    }

    pSessionInfo->db = NULL;
    pSessionInfo->transaction = FALSE;

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

done:
    ACK_unlockMutex(&pSessionInfo->mutex);

    return kResult;
}

ACK_RESULT ACK_IsKeySetMounted( /* PUBLIC */
    ACK_SESSION session
    )
{
    ACK_RESULT kResult = ACK_S_OK;
    ACK_LPSESSIONINFO pSessionInfo = (ACK_LPSESSIONINFO)session;

    if (pSessionInfo == NULL)
        return E_HANDLE;

    ACK_lockMutex(&pSessionInfo->mutex);

    kResult = (pSessionInfo->db != NULL) ? S_OK : S_FALSE;

    ACK_unlockMutex(&pSessionInfo->mutex);

    return kResult;
}

ACK_RESULT ACK_SetSessionDirectory( /* PUBLIC */
    ACK_SESSION session,
    LPSTR directory
    )
{
    ACK_RESULT kResult = ACK_S_OK;
    ACK_LPSESSIONINFO pSessionInfo = (ACK_LPSESSIONINFO)session;
    LPSTR newDirectory = NULL;

    if (pSessionInfo == NULL)
        return E_HANDLE;

    ACK_lockMutex(&pSessionInfo->mutex);

    if (pSessionInfo->keyFileName != NULL) {
        kResult = ACK_E_KEY_FOR_SESSION;
        goto done;
    }

    if (directory == NULL) {
        if (pSessionInfo->directory != NULL) {
            ACK_free(pSessionInfo->directory);
            pSessionInfo->directory = NULL;
        }

        goto done;
    }

    newDirectory = ACK_strdup(directory);

    if (newDirectory == NULL) {
        kResult = E_OUTOFMEMORY;
        goto done;
    }

    if (pSessionInfo->directory != NULL)
        ACK_free(pSessionInfo->directory);

    pSessionInfo->directory = newDirectory;

done:
    ACK_unlockMutex(&pSessionInfo->mutex);

    return kResult;
}

ACK_RESULT ACK_SetSessionKey( /* PUBLIC */
    ACK_SESSION session,
    LPSTR keyId
    )
{
    ACK_RESULT kResult = ACK_S_OK;
    ACK_LPSESSIONINFO pSessionInfo = (ACK_LPSESSIONINFO)session;
    sqlite3 *db = NULL;
    ACK_LPINTKEYINFO pKeyInfo = NULL;
    LPSTR newKeyId = NULL;
    LPSTR newFileName = NULL;

    if (pSessionInfo == NULL)
        return E_HANDLE;

    ACK_lockMutex(&pSessionInfo->mutex);

    db = pSessionInfo->db;

    if (db == NULL) {
        kResult = ACK_E_NO_MOUNTED_DATABASE;
        goto cleanup;
    }

    if (keyId == NULL) {
        if (pSessionInfo->keyId != NULL) {
            ACK_free(pSessionInfo->keyId);
            pSessionInfo->keyId = NULL;
        }

        if (pSessionInfo->keyFileName != NULL) {
            ACK_free(pSessionInfo->keyFileName);
            pSessionInfo->keyFileName = NULL;
        }

        goto cleanup;
    }

    if ((pSessionInfo->keyId != NULL) &&
            strcmp(pSessionInfo->keyId, keyId) == 0) {
        goto cleanup;
    }

    kResult = getKeyInfo(db, keyId, &pKeyInfo);

    if (FAILED(kResult))
        goto cleanup;

    newKeyId = ACK_strdup(keyId);

    if (newKeyId == NULL) {
        kResult = E_OUTOFMEMORY;
        goto cleanup;
    }

    if (pSessionInfo->directory != NULL) {
        SIZE_T extra = 0;

        if (pKeyInfo->fileName == NULL) {
            kResult = ACK_E_NO_FILE_NAME_FOR_KEY;
            goto cleanup;
        }

        extra += strlen(pKeyInfo->fileName) + 1; /* '/' OR '\\' */
        newFileName = ACK_strdup2(pSessionInfo->directory, extra);

        if (newFileName == NULL) {
            kResult = E_OUTOFMEMORY;
            goto cleanup;
        }

#if defined(WIN32)
        ACK_strcat(newFileName, "\\");
#else
        ACK_strcat(newFileName, "/");
#endif

        ACK_strcat(newFileName, pKeyInfo->fileName);
    } else {
        newFileName = makeQualifiedFileName(pSessionInfo->fileName,
            pKeyInfo->fileName, NULL);

        if (newFileName == NULL) {
            kResult = E_FAIL;
            goto cleanup;
        }
    }

    /*
     * NOTE: Do not eagerly ATTACH the key's chunk database here.  ATTACH
     *       cannot occur inside a SQLite transaction, so the engine
     *       defers per-chunk ATTACH/DETACH to getKeyBytes() where it is
     *       performed around the read of the request's chunk plan.
     */

    if (pSessionInfo->keyId != NULL)
        ACK_free(pSessionInfo->keyId);

    pSessionInfo->keyId = newKeyId;

    if (pSessionInfo->keyFileName != NULL)
        ACK_free(pSessionInfo->keyFileName);

    pSessionInfo->keyFileName = newFileName;
    pSessionInfo->chunkId = pKeyInfo->blobInfo.chunkId;

    goto done;

cleanup:
    if (newKeyId != NULL)
        ACK_free(newKeyId);

    if (newFileName != NULL)
        ACK_free(newFileName);

done:
    if (pKeyInfo != NULL)
        freeKeyInfo(pKeyInfo);

    ACK_unlockMutex(&pSessionInfo->mutex);

    return kResult;
}

ACK_RESULT ACK_GetAllKeys( /* PUBLIC */
    ACK_SESSION session,
    INT64 version,
    LPVOID *ppReserved,
    LPINT64 pCount,
    ACK_LPKEYINFO *ppKeyInfo
    )
{
    ACK_RESULT kResult = ACK_S_OK;
    ACK_LPSESSIONINFO pSessionInfo = (ACK_LPSESSIONINFO)session;
    sqlite3 *db = NULL;
    char **azResult = NULL;
    int nRow = 0;
    int nColumn = 0;
    char *zErrMsg = NULL;
    int rc = SQLITE_OK;
    ACK_LPKEYINFO pKeyInfo = NULL;
    int iRow = 0;
    int iColumn = 0;
    int index = 0;

    if (pSessionInfo == NULL)
        return E_HANDLE;

    if (version != ACK_KEYINFO_VERSION)
        return ACK_E_UNSUPPORTED_VERSION;

    if (ppReserved != NULL)
        return E_MUSTBENULL;

    if ((pCount == NULL) || (ppKeyInfo == NULL))
        return E_POINTER;

    if (*ppKeyInfo != NULL)
        return E_MUSTBENULL;

    ACK_lockMutex(&pSessionInfo->mutex);

    if (!(pSessionInfo->type & ACKST_ListKeys)) {
        kResult = ACK_E_WRONG_SESSION_TYPE;
        goto done;
    }

    db = pSessionInfo->db;

    if (db == NULL) {
        kResult = ACK_E_NO_MOUNTED_DATABASE;
        goto done;
    }

    rc = sqlite3_get_table(db, ACK_GETALLKEYS_SQL, &azResult, &nRow, &nColumn,
        &zErrMsg);

    ACK_CHECK_SQLITE_RC(sqlite3_get_table, rc, SQLITE_OK);

    if (rc != SQLITE_OK) {
        kResult = HRESULT_FROM_SQLITE(rc);
        goto done;
    }

    pKeyInfo = (ACK_LPKEYINFO)ACK_zalloc(ACK_SizeOfKeyInfo() * nRow);

    if (pKeyInfo == NULL) {
        kResult = E_OUTOFMEMORY;
        goto done;
    }

    for (iRow = 0; iRow < nRow; iRow++) {
        for (iColumn = 0; iColumn < nColumn; iColumn++) {
            index = ((iRow + 1) * nColumn) + iColumn;

            switch (iColumn) {
                case ALLKEYINFO_COLUMN_ID: {
                    if (azResult[index] != NULL) {
                        pKeyInfo[iRow].keyId = pKeyInfo[iRow].aKeyId;
                        strncpy(pKeyInfo[iRow].keyId, azResult[index],
                            ACK_KEYINFO_MAXID);
                        pKeyInfo[iRow].keyId[ACK_KEYINFO_MAXID - 1] = '\0';
                    }
                    break;
                }
                case ALLKEYINFO_COLUMN_SETID: {
                    if (azResult[index] != NULL) {
                        pKeyInfo[iRow].keySetId = pKeyInfo[iRow].aKeySetId;
                        strncpy(pKeyInfo[iRow].keySetId, azResult[index],
                            ACK_KEYINFO_MAXID);
                        pKeyInfo[iRow].keySetId[ACK_KEYINFO_MAXID - 1] = '\0';
                    }
                    break;
                }
                case ALLKEYINFO_COLUMN_GROUPID: {
                    if (azResult[index] != NULL) {
                        pKeyInfo[iRow].keyGroupId = pKeyInfo[iRow].aKeyGroupId;
                        strncpy(pKeyInfo[iRow].keyGroupId, azResult[index],
                            ACK_KEYINFO_MAXID);
                        pKeyInfo[iRow].keyGroupId[ACK_KEYINFO_MAXID - 1] = '\0';
                    }
                    break;
                }
                case ALLKEYINFO_COLUMN_NAME: {
                    if (azResult[index] != NULL) {
                        pKeyInfo[iRow].keyName = pKeyInfo[iRow].aKeyName;
                        strncpy(pKeyInfo[iRow].keyName, azResult[index],
                            ACK_KEYINFO_MAXNAME);
                        pKeyInfo[iRow].keyName[ACK_KEYINFO_MAXNAME - 1] = '\0';
                    }
                    break;
                }
                case ALLKEYINFO_COLUMN_ISENCRYPT: {
                    if (azResult[index] != NULL)
                        pKeyInfo[iRow].isEncrypt = atoi(azResult[index]);

                    break;
                }
                case ALLKEYINFO_COLUMN_ISREVOKED: {
                    if (azResult[index] != NULL)
                        pKeyInfo[iRow].isRevoked = atoi(azResult[index]);

                    break;
                }
                case ALLKEYINFO_COLUMN_ISERASABLE: {
                    if (azResult[index] != NULL)
                        pKeyInfo[iRow].isErasable = atoi(azResult[index]);

                    break;
                }
                case ALLKEYINFO_COLUMN_TOTALBYTES: {
                    if (azResult[index] != NULL)
                        pKeyInfo[iRow].bytesTotal = ACK_atoull(azResult[index]);

                    break;
                }
                case ALLKEYINFO_COLUMN_USEDBYTES: {
                    if (azResult[index] != NULL)
                        pKeyInfo[iRow].bytesUsed = ACK_atoull(azResult[index]);

                    break;
                }
                default: {
                    /*
                     * NOTE: Do nothing, we have no idea what this column
                     *       actually is.  Making this condition into an error
                     *       will break forward-compatibility (i.e. please do
                     *       not do it).
                     */

                    break;
                }
            }
        }

        pKeyInfo[iRow].version = ACK_KEYINFO_VERSION;
    }

    /*
     * NOTE: All-or-nothing semantics.  Commit changes now to the variables
     *       provided by the caller now that we know for certain that we have
     *       fully succeeded.
     */

    *pCount = nRow;
    *ppKeyInfo = pKeyInfo;

done:
    /*
     * NOTE: We know that SQLite has allocated space for the table because the
     *       query call succeeded; therefore, always free the table regardless
     *       of other criteria.
     */

    if (azResult != NULL)
        sqlite3_free_table(azResult);

    ACK_unlockMutex(&pSessionInfo->mutex);

    return kResult;
}

SIZE_T ACK_SizeOfKeyInfo(VOID) /* PUBLIC */
{
    return sizeof(ACK_KEYINFO);
}

ACK_RESULT ACK_FreeAllKeys( /* PUBLIC */
    ACK_SESSION session,
    LPVOID *ppReserved,
    LPINT64 pCount,
    ACK_LPKEYINFO *ppKeyInfo
    )
{
    ACK_LPSESSIONINFO pSessionInfo = (ACK_LPSESSIONINFO)session;
    ACK_LPKEYINFO pKeyInfo = NULL;
    INT64 index = 0;

    if (pSessionInfo == NULL)
        return E_HANDLE;

    if (ppReserved != NULL)
        return E_MUSTBENULL;

    if ((pCount == NULL) || (ppKeyInfo == NULL) || (*ppKeyInfo == NULL))
        return E_POINTER;

    ACK_lockMutex(&pSessionInfo->mutex); /* NOTE: Not strictly needed. */

    pKeyInfo = *ppKeyInfo;

    for (index = 0; index < *pCount; index++) {
        if (pKeyInfo[index].keyId != pKeyInfo[index].aKeyId) {
            ACK_free(pKeyInfo[index].keyId);
            pKeyInfo[index].keyId = pKeyInfo[index].aKeyId;
        }

        memset(pKeyInfo[index].aKeyId, 0, sizeof(pKeyInfo[index].aKeyId));

        if (pKeyInfo[index].keySetId != pKeyInfo[index].aKeySetId) {
            ACK_free(pKeyInfo[index].keySetId);
            pKeyInfo[index].keySetId = pKeyInfo[index].aKeySetId;
        }

        memset(pKeyInfo[index].aKeySetId, 0,
            sizeof(pKeyInfo[index].aKeySetId));

        if (pKeyInfo[index].keyGroupId != pKeyInfo[index].aKeyGroupId) {
            ACK_free(pKeyInfo[index].keyGroupId);
            pKeyInfo[index].keyGroupId = pKeyInfo[index].aKeyGroupId;
        }

        memset(pKeyInfo[index].aKeyGroupId, 0,
            sizeof(pKeyInfo[index].aKeyGroupId));

        if (pKeyInfo[index].keyName != pKeyInfo[index].aKeyName) {
            ACK_free(pKeyInfo[index].keyName);
            pKeyInfo[index].keyName = pKeyInfo[index].aKeyName;
        }

        memset(pKeyInfo[index].aKeyName, 0,
            sizeof(pKeyInfo[index].aKeyName));
    }

    ACK_free(*ppKeyInfo);

    *pCount = 0;
    *ppKeyInfo = NULL;

    ACK_unlockMutex(&pSessionInfo->mutex);

    return ACK_S_OK;
}

ACK_RESULT ACK_GetAllKeyProperties( /* PUBLIC */
    ACK_SESSION session,
    INT64 version,
    LPVOID *ppReserved,
    LPSTR keyId,
    LPINT64 pCount,
    ACK_LPKEYPROP *ppKeyProp
    )
{
    ACK_RESULT kResult = ACK_S_OK;
    ACK_LPSESSIONINFO pSessionInfo = (ACK_LPSESSIONINFO)session;
    sqlite3 *db = NULL;
    LPSTR zSql = NULL;
    char **azResult = NULL;
    int nRow = 0;
    int nColumn = 0;
    char *zErrMsg = NULL;
    int rc = SQLITE_OK;
    ACK_LPKEYPROP pKeyProp = NULL;
    int iRow = 0;
    int iColumn = 0;
    int index = 0;

    if (pSessionInfo == NULL)
        return E_HANDLE;

    if (version != ACK_KEYPROP_VERSION)
        return ACK_E_UNSUPPORTED_VERSION;

    if (ppReserved != NULL)
        return E_MUSTBENULL;

    if ((keyId == NULL) || (pCount == NULL) || (ppKeyProp == NULL))
        return E_POINTER;

    if (*ppKeyProp != NULL)
        return E_MUSTBENULL;

    ACK_lockMutex(&pSessionInfo->mutex);

    if (!(pSessionInfo->type & ACKST_ListKeyProperties)) {
        kResult = ACK_E_WRONG_SESSION_TYPE;
        goto done;
    }

    db = pSessionInfo->db;

    if (db == NULL) {
        kResult = ACK_E_NO_MOUNTED_DATABASE;
        goto done;
    }

    zSql = sqlite3_mprintf(ACK_GETALLPROPS_SQL, keyId);

    if (zSql == NULL) {
        kResult = E_OUTOFMEMORY;
        goto done;
    }

    rc = sqlite3_get_table(db, zSql, &azResult, &nRow, &nColumn, &zErrMsg);

    ACK_CHECK_SQLITE_RC(sqlite3_get_table, rc, SQLITE_OK);

    if (rc != SQLITE_OK) {
        kResult = HRESULT_FROM_SQLITE(rc);
        goto done;
    }

    pKeyProp = (ACK_LPKEYPROP)ACK_zalloc(ACK_SizeOfKeyProp() * nRow);

    if (pKeyProp == NULL) {
        kResult = E_OUTOFMEMORY;
        goto done;
    }

    for (iRow = 0; iRow < nRow; iRow++) {
        for (iColumn = 0; iColumn < nColumn; iColumn++) {
            index = ((iRow + 1) * nColumn) + iColumn;

            switch (iColumn) {
                case ALLKEYPROP_COLUMN_ID: {
                    if (azResult[index] != NULL)
                        pKeyProp[iRow].id = ACK_atoull(azResult[index]);

                    break;
                }
                case ALLKEYPROP_COLUMN_TYPE: {
                    if (azResult[index] != NULL) {
                        pKeyProp[iRow].type = pKeyProp[iRow].aType;
                        strncpy(pKeyProp[iRow].type, azResult[index],
                            ACK_KEYPROP_MAXTYPE);
                        pKeyProp[iRow].type[ACK_KEYPROP_MAXTYPE - 1] = '\0';
                    }
                    break;
                }
                case ALLKEYPROP_COLUMN_NAME: {
                    if (azResult[index] != NULL) {
                        pKeyProp[iRow].name = pKeyProp[iRow].aName;
                        strncpy(pKeyProp[iRow].name, azResult[index],
                            ACK_KEYPROP_MAXNAME);
                        pKeyProp[iRow].name[ACK_KEYPROP_MAXNAME - 1] = '\0';
                    }
                    break;
                }
                case ALLKEYPROP_COLUMN_VALUE: {
                    if (azResult[index] != NULL) {
                        pKeyProp[iRow].value = pKeyProp[iRow].aValue;
                        strncpy(pKeyProp[iRow].value, azResult[index],
                            ACK_KEYPROP_MAXVALUE);
                        pKeyProp[iRow].value[ACK_KEYPROP_MAXVALUE - 1] = '\0';
                    }
                    break;
                }
                default: {
                    /*
                     * NOTE: Do nothing, we have no idea what this column
                     *       actually is.  Making this condition into an error
                     *       will break forward-compatibility (i.e. please do
                     *       not do it).
                     */

                    break;
                }
            }
        }

        pKeyProp[iRow].version = ACK_KEYPROP_VERSION;
    }

    /*
     * NOTE: All-or-nothing semantics.  Commit changes now to the variables
     *       provided by the caller now that we know for certain that we have
     *       fully succeeded.
     */

    *pCount = nRow;
    *ppKeyProp = pKeyProp;

done:
    /*
     * NOTE: We know that SQLite has allocated space for the table because the
     *       query call succeeded; therefore, always free the table regardless
     *       of other criteria.
     */

    if (azResult != NULL)
        sqlite3_free_table(azResult);

    if (zSql != NULL)
        sqlite3_free(zSql);

    ACK_unlockMutex(&pSessionInfo->mutex);

    return kResult;
}

ACK_RESULT ACK_GetKeyProperty( /* PUBLIC */
    ACK_SESSION session,
    INT64 version,
    LPVOID *ppReserved,
    LPSTR keyId,
    LPSTR name,
    ACK_LPKEYPROP *ppKeyProp
    )
{
    ACK_RESULT kResult = ACK_S_OK;
    ACK_LPSESSIONINFO pSessionInfo = (ACK_LPSESSIONINFO)session;
    sqlite3 *db = NULL;
    sqlite3_stmt *pStmt = NULL;
    int rc = SQLITE_OK;
    ACK_LPKEYPROP pKeyProp = NULL;
    LPSTR type = NULL;
    LPSTR value = NULL;

    if (pSessionInfo == NULL)
        return E_HANDLE;

    if (version != ACK_KEYPROP_VERSION)
        return ACK_E_UNSUPPORTED_VERSION;

    if (ppReserved != NULL)
        return E_MUSTBENULL;

    if ((keyId == NULL) || (name == NULL) || (ppKeyProp == NULL))
        return E_POINTER;

    if (*ppKeyProp != NULL)
        return E_MUSTBENULL;

    ACK_lockMutex(&pSessionInfo->mutex);

    if (!(pSessionInfo->type & ACKST_GetKeyProperty)) {
        kResult = ACK_E_WRONG_SESSION_TYPE;
        goto cleanup;
    }

    db = pSessionInfo->db;

    if (db == NULL) {
        kResult = ACK_E_NO_MOUNTED_DATABASE;
        goto cleanup;
    }

    pKeyProp = (ACK_LPKEYPROP)ACK_zalloc(ACK_SizeOfKeyProp());

    if (pKeyProp == NULL) {
        kResult = E_OUTOFMEMORY;
        goto cleanup;
    }

    rc = prepareAndExecute(db, ACK_GETKEYPROP_SQL, keyId, FALSE, name, FALSE,
        NULL, FALSE, NULL, FALSE, &pStmt);

    ACK_CHECK_SQLITE_RC(prepareAndExecute, rc, SQLITE_ROW);

    if (rc != SQLITE_ROW) {
        kResult = HRESULT_FROM_SQLITE(rc);
        goto cleanup;
    }

    pKeyProp->version = ACK_KEYPROP_VERSION;

    pKeyProp->id = sqlite3_column_int64(pStmt, ONEKEYPROP_COLUMN_ID);

    type = (LPSTR)sqlite3_column_text(pStmt, ONEKEYPROP_COLUMN_TYPE);

    if (type != NULL) {
        pKeyProp->type = pKeyProp->aType;
        strncpy(pKeyProp->type, type, ACK_KEYPROP_MAXTYPE);
        pKeyProp->type[ACK_KEYPROP_MAXTYPE - 1] = '\0';
    }

    pKeyProp->name = pKeyProp->aName;
    strncpy(pKeyProp->name,
        (LPSTR)sqlite3_column_text(pStmt, ONEKEYPROP_COLUMN_NAME),
        ACK_KEYPROP_MAXNAME);
    pKeyProp->name[ACK_KEYPROP_MAXNAME - 1] = '\0';

    value = (LPSTR)sqlite3_column_text(pStmt, ONEKEYPROP_COLUMN_VALUE);

    if (value != NULL) {
        pKeyProp->value = pKeyProp->aValue;
        strncpy(pKeyProp->value, value, ACK_KEYPROP_MAXVALUE);
        pKeyProp->value[ACK_KEYPROP_MAXVALUE - 1] = '\0';
    }

    *ppKeyProp = pKeyProp;

    goto done;

cleanup:
    if (FAILED(kResult) && (pKeyProp != NULL))
        ACK_free(pKeyProp);

done:
    if (pStmt != NULL) {
        rc = sqlite3_finalize(pStmt);
        ACK_ASSERT_SQLITE_RC(sqlite3_finalize, rc, SQLITE_OK);
    }

    ACK_unlockMutex(&pSessionInfo->mutex);

    return kResult;
}

ACK_RESULT ACK_SetKeyProperty( /* PUBLIC */
    ACK_SESSION session,
    INT64 version,
    LPVOID *ppReserved,
    LPSTR keyId,
    LPSTR type, /* NOTE: May be NULL. */
    LPSTR name,
    LPSTR value
    )
{
    ACK_RESULT kResult = ACK_S_OK;
    ACK_LPSESSIONINFO pSessionInfo = (ACK_LPSESSIONINFO)session;
    sqlite3 *db = NULL;
    int rc = SQLITE_OK;

    if (pSessionInfo == NULL)
        return E_HANDLE;

    if (version != ACK_KEYPROP_VERSION)
        return ACK_E_UNSUPPORTED_VERSION;

    if (ppReserved != NULL)
        return E_MUSTBENULL;

    if ((keyId == NULL) || (name == NULL) || (value == NULL))
        return E_POINTER;

    ACK_lockMutex(&pSessionInfo->mutex);

    if (!(pSessionInfo->type & ACKST_SetKeyProperty)) {
        kResult = ACK_E_WRONG_SESSION_TYPE;
        goto done;
    }

    db = pSessionInfo->db;

    if (db == NULL) {
        kResult = ACK_E_NO_MOUNTED_DATABASE;
        goto done;
    }

    rc = prepareAndExecute(db, ACK_UPDATE_PROP_SQL, keyId, FALSE, type, TRUE,
        name, FALSE, value, FALSE, NULL);

    ACK_CHECK_SQLITE_RC(prepareAndExecute, rc, SQLITE_DONE);

    if (rc != SQLITE_DONE) {
        kResult = HRESULT_FROM_SQLITE(rc);
        goto done;
    }

done:
    ACK_unlockMutex(&pSessionInfo->mutex);

    return kResult;
}

ACK_RESULT ACK_UnsetKeyProperty( /* PUBLIC */
    ACK_SESSION session,
    INT64 version,
    LPVOID *ppReserved,
    LPSTR keyId,
    LPSTR name
    )
{
    ACK_RESULT kResult = ACK_S_OK;
    ACK_LPSESSIONINFO pSessionInfo = (ACK_LPSESSIONINFO)session;
    sqlite3 *db = NULL;
    int rc = SQLITE_OK;

    if (pSessionInfo == NULL)
        return E_HANDLE;

    if (version != ACK_KEYPROP_VERSION)
        return ACK_E_UNSUPPORTED_VERSION;

    if (ppReserved != NULL)
        return E_MUSTBENULL;

    if ((keyId == NULL) || (name == NULL))
        return E_POINTER;

    ACK_lockMutex(&pSessionInfo->mutex);

    if (!(pSessionInfo->type & ACKST_UnsetKeyProperty)) {
        kResult = ACK_E_WRONG_SESSION_TYPE;
        goto done;
    }

    db = pSessionInfo->db;

    if (db == NULL) {
        kResult = ACK_E_NO_MOUNTED_DATABASE;
        goto done;
    }

    rc = prepareAndExecute(db, ACK_DELETE_PROP_SQL, keyId, FALSE, name, FALSE,
        NULL, FALSE, NULL, FALSE, NULL);

    ACK_CHECK_SQLITE_RC(prepareAndExecute, rc, SQLITE_DONE);

    if (rc != SQLITE_DONE) {
        kResult = HRESULT_FROM_SQLITE(rc);
        goto done;
    }

done:
    ACK_unlockMutex(&pSessionInfo->mutex);

    return kResult;
}

SIZE_T ACK_SizeOfKeyProp(VOID) /* PUBLIC */
{
    return sizeof(ACK_KEYPROP);
}

ACK_RESULT ACK_FreeAllKeyProperties( /* PUBLIC */
    ACK_SESSION session,
    LPVOID pReserved,
    LPINT64 pCount,
    ACK_LPKEYPROP *ppKeyProp
    )
{
    ACK_LPSESSIONINFO pSessionInfo = (ACK_LPSESSIONINFO)session;
    ACK_LPKEYPROP pKeyProp = NULL;
    INT64 index = 0;

    if (pSessionInfo == NULL)
        return E_HANDLE;

    if (pReserved != NULL)
        return E_MUSTBENULL;

    if ((pCount == NULL) || (ppKeyProp == NULL) || (*ppKeyProp == NULL))
        return E_POINTER;

    ACK_lockMutex(&pSessionInfo->mutex); /* NOTE: Not strictly needed. */

    pKeyProp = *ppKeyProp;

    for (index = 0; index < *pCount; index++) {
        if (pKeyProp[index].type != pKeyProp[index].aType) {
            ACK_free(pKeyProp[index].type);
            pKeyProp[index].type = pKeyProp[index].aType;
        }

        memset(pKeyProp[index].aType, 0, sizeof(pKeyProp[index].aType));

        if (pKeyProp[index].name != pKeyProp[index].aName) {
            ACK_free(pKeyProp[index].name);
            pKeyProp[index].name = pKeyProp[index].aName;
        }

        memset(pKeyProp[index].aName, 0, sizeof(pKeyProp[index].aName));

        if (pKeyProp[index].value != pKeyProp[index].aValue) {
            ACK_free(pKeyProp[index].value);
            pKeyProp[index].value = pKeyProp[index].aValue;
        }

        memset(pKeyProp[index].aValue, 0, sizeof(pKeyProp[index].aValue));
    }

    ACK_free(*ppKeyProp);

    *pCount = 0;
    *ppKeyProp = NULL;

    ACK_unlockMutex(&pSessionInfo->mutex);

    return ACK_S_OK;
}

ACK_RESULT ACK_Encrypt( /* PUBLIC */
    ACK_SESSION session,
    LPVOID *ppReserved,
    UINT64 flags,
    LPUINT64 pOffset,
    LPBYTE pInput,
    SIZE_T inSize,
    LPBYTE *ppOutput,
    LPSIZE_T pOutSize
    )
{
    ACK_RESULT kResult = ACK_S_OK;
    ACK_LPSESSIONINFO pSessionInfo = (ACK_LPSESSIONINFO)session;
    sqlite3 *db = NULL;
    LPBYTE pOutput = NULL;
    BOOL allocated = FALSE;
    BOOL useCache = FALSE;
    UINT64 offset = 0;
    LPBYTE pKey = NULL;
    LPBYTE *ppKey = &pKey;
    SIZE_T keySize = 0;
    LPSIZE_T pKeySize = &keySize;

    UNUSED_ARGUMENT(flags);

    if (pSessionInfo == NULL)
        return E_HANDLE;

    if (ppReserved != NULL)
        return E_MUSTBENULL;

    if ((pOffset == NULL) || (pInput == NULL) || (ppOutput == NULL) ||
            (pOutSize == NULL))
        return E_POINTER;

    if (*pOffset != 0)
        return E_MUSTBEZERO;

    if (inSize > INT_MAX)
        return E_OVERFLOW;
    else if (inSize > MAX_CHUNK_SIZE)
        return ACK_E_MAXIMUM_KEY_SIZE_EXCEEDED;

    ACK_lockMutex(&pSessionInfo->mutex);

    if (!(pSessionInfo->type & ACKST_Encrypt)) {
        kResult = ACK_E_WRONG_SESSION_TYPE;
        goto cleanup;
    }

    if (!(pSessionInfo->type & (ACKST_Stream | ACKST_Block | ACKST_File))) {
        kResult = ACK_E_WRONG_SESSION_TYPE;
        goto cleanup;
    }

    if (pSessionInfo->keyId == NULL) {
        kResult = ACK_E_NO_KEY_FOR_SESSION;
        goto cleanup;
    }

    db = pSessionInfo->db;

    if (db == NULL) {
        kResult = ACK_E_NO_MOUNTED_DATABASE;
        goto cleanup;
    }

    if (pSessionInfo->transaction) {
        kResult = ACK_E_TRANSACTION_PENDING;
        goto cleanup;
    }

    /*
     * NOTE: If there is nothing to encrypt, simply bail out now.
     */

    if (inSize == 0)
        goto cleanup; /* SUCCESS */

    if ((*ppOutput == NULL) || (*pOutSize < inSize)) {
        /*
         * NOTE: The caller did not provide a valid buffer or it is too small.
         *       Attempt to allocate an output buffer now.
         */

        pOutput = (LPBYTE)ACK_zalloc(sizeof(BYTE) * inSize);

        if (pOutput == NULL) {
            kResult = E_OUTOFMEMORY;
            goto cleanup;
        }

        /*
         * NOTE: In case of failure, we need to know that we actually allocated
         *       the output buffer (i.e. it was not provided by the caller) and
         *       we must free it.
         */

        allocated = TRUE;
    } else {
        pOutput = *ppOutput;
    }

    /*
     * NOTE: Choose between the in-memory cache and a direct database
     *       fetch.  Transactional and chunk-attach management are
     *       performed inside getKeyBytes(); the wrapper does not start
     *       a transaction of its own.
     */

    useCache = isCacheEnabled(pSessionInfo, pSessionInfo->type);

    /*
     * NOTE: Can we use the temporary key storage for the session to avoid
     *       a malloc/free?
     */

    if ((pSessionInfo->pKey != NULL) && (pSessionInfo->keySize >= inSize)) {
        ppKey = &pSessionInfo->pKey;
        pKeySize = &pSessionInfo->keySize;
    }

    if (useCache) {
        kResult = getCachedKeyBytes(pSessionInfo, pSessionInfo->keyId, inSize,
            TRUE, &offset, ppKey, pKeySize);
    } else {
        kResult = getKeyBytes(pSessionInfo, pSessionInfo->keyId, inSize, TRUE,
            &offset, ppKey, pKeySize);
    }

    if (FAILED(kResult))
        goto cleanup;

    ACK_XOR(pOutput, pInput, *ppKey, inSize);

    /*
     * NOTE: All-or-nothing semantics.  Commit changes now to the variables
     *       provided by the caller now that we know for certain that we have
     *       fully succeeded.  If we did not allocate the output buffer then
     *       we used the one provided by the caller; in that case, we do not
     *       want to modify those output arguments.
     */

    if (allocated) {
        if (*ppOutput != NULL)
            ACK_free(*ppOutput);

        *ppOutput = pOutput;
    }

    *pOutSize = inSize;
    *pOffset = offset;

    goto done;

cleanup:
    if (allocated && (pOutput != NULL))
        ACK_free(pOutput);

    /*
     * NOTE: Transaction and ATTACH/DETACH state are managed entirely
     *       within getKeyBytes(); on failure it rolls back and detaches
     *       so no further cleanup is required here.
     */

done:
    if (pKey != NULL)
        ACK_free(pKey);

    ACK_unlockMutex(&pSessionInfo->mutex);

    return kResult;
}

ACK_RESULT ACK_Decrypt( /* PUBLIC */
    ACK_SESSION session,
    LPVOID *ppReserved,
    UINT64 flags,
    UINT64 offset,
    LPBYTE pInput,
    SIZE_T inSize,
    LPBYTE *ppOutput,
    LPSIZE_T pOutSize
    )
{
    ACK_RESULT kResult = ACK_S_OK;
    ACK_LPSESSIONINFO pSessionInfo = (ACK_LPSESSIONINFO)session;
    sqlite3 *db = NULL;
    LPBYTE pOutput = NULL;
    BOOL allocated = FALSE;
    BOOL useCache = FALSE;
    LPBYTE pKey = NULL;
    LPBYTE *ppKey = &pKey;
    SIZE_T keySize = 0;
    LPSIZE_T pKeySize = &keySize;

    UNUSED_ARGUMENT(flags);

    if (pSessionInfo == NULL)
        return E_HANDLE;

    if (ppReserved != NULL)
        return E_MUSTBENULL;

    if ((pInput == NULL) || (ppOutput == NULL) || (pOutSize == NULL))
        return E_POINTER;

    if (inSize > INT_MAX)
        return E_OVERFLOW;
    else if (inSize > MAX_CHUNK_SIZE)
        return ACK_E_MAXIMUM_KEY_SIZE_EXCEEDED;

    ACK_lockMutex(&pSessionInfo->mutex);

    if (!(pSessionInfo->type & ACKST_Decrypt)) {
        kResult = ACK_E_WRONG_SESSION_TYPE;
        goto cleanup;
    }

    if (!(pSessionInfo->type & (ACKST_Stream | ACKST_Block | ACKST_File))) {
        kResult = ACK_E_WRONG_SESSION_TYPE;
        goto cleanup;
    }

    if (pSessionInfo->keyId == NULL) {
        kResult = ACK_E_NO_KEY_FOR_SESSION;
        goto cleanup;
    }

    db = pSessionInfo->db;

    if (db == NULL) {
        kResult = ACK_E_NO_MOUNTED_DATABASE;
        goto cleanup;
    }

    if (pSessionInfo->transaction) {
        kResult = ACK_E_TRANSACTION_PENDING;
        goto cleanup;
    }

    /*
     * NOTE: If there is nothing to decrypt, simply bail out now.
     */

    if (inSize == 0)
        goto cleanup; /* SUCCESS */

    if ((*ppOutput == NULL) || (*pOutSize < inSize)) {
        /*
         * NOTE: The caller did not provide a valid buffer or it is too small.
         *       Attempt to allocate an output buffer now.
         */

        pOutput = (LPBYTE)ACK_zalloc(sizeof(BYTE) * inSize);

        if (pOutput == NULL) {
            kResult = E_OUTOFMEMORY;
            goto cleanup;
        }

        /*
         * NOTE: In case of failure, we need to know that we actually allocated
         *       the output buffer (i.e. it was not provided by the caller) and
         *       we must free it.
         */

        allocated = TRUE;
    } else {
        pOutput = *ppOutput;
    }

    /*
     * NOTE: Choose between the in-memory cache and a direct database
     *       fetch.  Transactional and chunk-attach management are
     *       performed inside getKeyBytes(); the wrapper does not start
     *       a transaction of its own.
     */

    useCache = isCacheEnabled(pSessionInfo, pSessionInfo->type);

    /*
     * NOTE: Can we use the temporary key storage for the session to avoid
     *       a malloc/free?
     */

    if ((pSessionInfo->pKey != NULL) && (pSessionInfo->keySize >= inSize)) {
        ppKey = &pSessionInfo->pKey;
        pKeySize = &pSessionInfo->keySize;
    }

    if (useCache) {
        kResult = getCachedKeyBytes(pSessionInfo, pSessionInfo->keyId, inSize,
            FALSE, &offset, ppKey, pKeySize);
    } else {
        kResult = getKeyBytes(pSessionInfo, pSessionInfo->keyId, inSize, FALSE,
            &offset, ppKey, pKeySize);
    }

    if (FAILED(kResult))
        goto cleanup;

    ACK_XOR(pOutput, pInput, *ppKey, inSize);

    /*
     * NOTE: All-or-nothing semantics.  Commit changes now to the variables
     *       provided by the caller now that we know for certain that we have
     *       fully succeeded.  If we did not allocate the output buffer then
     *       we used the one provided by the caller; in that case, we do not
     *       want to modify those output arguments.
     */

    if (allocated) {
        if (*ppOutput != NULL)
            ACK_free(*ppOutput);

        *ppOutput = pOutput;
    }

    *pOutSize = inSize;

    goto done;

cleanup:
    if (allocated && (pOutput != NULL))
        ACK_free(pOutput);

    /*
     * NOTE: Transaction and ATTACH/DETACH state are managed entirely
     *       within getKeyBytes(); on failure it rolls back and detaches
     *       so no further cleanup is required here.
     */

done:
    if (pKey != NULL)
        ACK_free(pKey);

    ACK_unlockMutex(&pSessionInfo->mutex);

    return kResult;
}

/* end of file */
