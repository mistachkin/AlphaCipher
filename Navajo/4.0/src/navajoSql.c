/*
 * navajoSql.c -- Private SQL Helper API
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
#include "navajoSql.h"

ACK_RESULT beginTransaction( /* PRIVATE */
    sqlite3 *db
    )
{
    int rc = SQLITE_OK;

    if (db == NULL)
        return E_HANDLE;

    rc = sqlite3_exec(db, ACK_BEGIN_TRAN_SQL, NULL, NULL, NULL);

    ACK_CHECK_SQLITE_RC(sqlite3_exec, rc, SQLITE_OK);

    if (rc != SQLITE_OK)
        return HRESULT_FROM_SQLITE(rc);

    return ACK_S_OK;
}

ACK_RESULT commitTransaction( /* PRIVATE */
    sqlite3 *db
    )
{
    int rc = SQLITE_OK;

    if (db == NULL)
        return E_HANDLE;

    rc = sqlite3_exec(db, ACK_COMMIT_TRAN_SQL, NULL, NULL, NULL);

    ACK_CHECK_SQLITE_RC(sqlite3_exec, rc, SQLITE_OK);

    if (rc != SQLITE_OK)
        return HRESULT_FROM_SQLITE(rc);

    return ACK_S_OK;
}

ACK_RESULT rollbackTransaction( /* PRIVATE */
    sqlite3 *db
    )
{
    int rc = SQLITE_OK;

    if (db == NULL)
        return E_HANDLE;

    rc = sqlite3_exec(db, ACK_ROLLBACK_TRAN_SQL, NULL, NULL, NULL);

    ACK_CHECK_SQLITE_RC(sqlite3_exec, rc, SQLITE_OK);

    if (rc != SQLITE_OK)
        return HRESULT_FROM_SQLITE(rc);

    return ACK_S_OK;
}

int prepareAndExecute( /* PRIVATE */
    sqlite3 *db,
    LPCSTR zSql,
    LPCSTR zData1,
    BOOL force1,
    LPCSTR zData2,
    BOOL force2,
    LPCSTR zData3,
    BOOL force3,
    LPCSTR zData4,
    BOOL force4,
    sqlite3_stmt **ppStmt
    )
{
    sqlite3_stmt *pStmt = NULL;
    int rc = SQLITE_OK;
    int rcFinalize = SQLITE_OK;

    if (db == NULL)
        return E_HANDLE;

    rc = sqlite3_prepare_v2(db, zSql, -1, &pStmt, NULL);

    ACK_CHECK_SQLITE_RC(sqlite3_prepare_v2, rc, SQLITE_OK);

    if (rc != SQLITE_OK)
        return HRESULT_FROM_SQLITE(rc);

    if (force1 || (zData1 != NULL)) {
        rc = sqlite3_bind_text(pStmt, 1, zData1, -1, SQLITE_TRANSIENT);

        ACK_CHECK_SQLITE_RC(sqlite3_bind_text, rc, SQLITE_OK);

        if (rc != SQLITE_OK)
            goto cleanup;
    }

    if (force2 || (zData2 != NULL)) {
        rc = sqlite3_bind_text(pStmt, 2, zData2, -1, SQLITE_TRANSIENT);

        ACK_CHECK_SQLITE_RC(sqlite3_bind_text, rc, SQLITE_OK);

        if (rc != SQLITE_OK)
            goto cleanup;
    }

    if (force3 || (zData3 != NULL)) {
        rc = sqlite3_bind_text(pStmt, 3, zData3, -1, SQLITE_TRANSIENT);

        ACK_CHECK_SQLITE_RC(sqlite3_bind_text, rc, SQLITE_OK);

        if (rc != SQLITE_OK)
            goto cleanup;
    }

    if (force4 || (zData4 != NULL)) {
        rc = sqlite3_bind_text(pStmt, 4, zData4, -1, SQLITE_TRANSIENT);

        ACK_CHECK_SQLITE_RC(sqlite3_bind_text, rc, SQLITE_OK);

        if (rc != SQLITE_OK)
            goto cleanup;
    }

    /*
     * NOTE: We cannot use the SQLite return code verification macro (i.e.
     *       ACK_CHECK_SQLITE_RC) here because this function is allowed to
     *       return any valid SQLite return code to the caller, including
     *       SQLITE_OK, SQLITE_ROW, and SQLITE_DONE.  Therefore, the caller
     *       of this function must use the ACK_CHECK_SQLITE_RC macro to
     *       validate the return code has the value it expects.
     */

    rc = sqlite3_step(pStmt);

    if (ppStmt == NULL)
        goto cleanup;

    *ppStmt = pStmt;

    goto done;

cleanup:
    if (pStmt != NULL) {
        rcFinalize = sqlite3_finalize(pStmt);
        ACK_ASSERT_SQLITE_RC(sqlite3_finalize, rcFinalize, SQLITE_OK);
        (VOID)rcFinalize; /* NOTE: silence unused-but-set in non-debug. */
    }

done:
    return rc;
}

ACK_RESULT attachDatabase( /* PRIVATE */
    sqlite3 *db,
    LPCSTR fileName,
    LPCSTR name
    )
{
    int rc = SQLITE_OK;

    if (db == NULL)
        return E_HANDLE;

    if ((fileName == NULL) || (name == NULL))
        return E_POINTER;

    rc = prepareAndExecute(db, ACK_ATTACH_DATABASE_SQL, fileName, FALSE,
        name, FALSE, NULL, FALSE, NULL, FALSE, NULL);

    ACK_CHECK_SQLITE_RC(prepareAndExecute, rc, SQLITE_DONE);

    if (rc != SQLITE_DONE)
        return HRESULT_FROM_SQLITE(rc);

    return ACK_S_OK;
}

ACK_RESULT detachDatabase( /* PRIVATE */
    sqlite3 *db,
    LPCSTR name
    )
{
    int rc = SQLITE_OK;

    if (db == NULL)
        return E_HANDLE;

    if (name == NULL)
        return E_POINTER;

    rc = prepareAndExecute(db, ACK_DETACH_DATABASE_SQL, name, FALSE, NULL,
        FALSE, NULL, FALSE, NULL, FALSE, NULL);

    ACK_CHECK_SQLITE_RC(prepareAndExecute, rc, SQLITE_DONE);

    if (rc != SQLITE_DONE)
        return HRESULT_FROM_SQLITE(rc);

    return ACK_S_OK;
}

/* end of file */
