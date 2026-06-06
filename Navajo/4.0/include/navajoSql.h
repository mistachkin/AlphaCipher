/*
 * navajoSql.h -- Private SQL Helper API
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

#if !defined(_SQLITE3_H_) && !defined(SQLITE3_H)
#error "The header file \"sqlite3.h\" must be included prior to this file."
#endif

#if !defined(_NAVAJO_H_)
#error "The header file \"navajo.h\" must be included prior to this file."
#endif

#ifndef _NAVAJO_SQL_H_
#define _NAVAJO_SQL_H_

/*****************************************************************************/

#ifndef ACK_MOUNT_KEYSET_SQL
#define ACK_MOUNT_KEYSET_SQL    NULL
#endif

#ifndef ACK_UNMOUNT_KEYSET_SQL
#define ACK_UNMOUNT_KEYSET_SQL  NULL
#endif

#ifndef ACK_VERIFY_DATABASE_SQL
#define ACK_VERIFY_DATABASE_SQL "PRAGMA integrity_check;"
#endif

#ifndef ACK_BEGIN_TRAN_SQL
#define ACK_BEGIN_TRAN_SQL      "BEGIN EXCLUSIVE TRANSACTION;"
#endif

#ifndef ACK_COMMIT_TRAN_SQL
#define ACK_COMMIT_TRAN_SQL     "COMMIT TRANSACTION;"
#endif

#ifndef ACK_ROLLBACK_TRAN_SQL
#define ACK_ROLLBACK_TRAN_SQL   "ROLLBACK TRANSACTION;"
#endif

#ifndef ACK_ATTACH_DATABASE_SQL
#define ACK_ATTACH_DATABASE_SQL "ATTACH DATABASE ? AS ?;"
#endif

#ifndef ACK_DETACH_DATABASE_SQL
#define ACK_DETACH_DATABASE_SQL "DETACH DATABASE ?;"
#endif

#ifndef ACK_UPDATE_ZEROBLOB_SQL
#define ACK_UPDATE_ZEROBLOB_SQL "UPDATE Chunks SET Data = zeroblob(?) WHERE Id = "   \
                                "? AND (Data IS NULL OR LENGTH(Data) < CAST(? AS "   \
                                "INTEGER));"
#endif

#ifndef ACK_GETALLKEYS_SQL
#define ACK_GETALLKEYS_SQL      "SELECT Keys.Id, Keys.KeySetId, Keys.KeyGroupId, "   \
                                "Keys.Name, Keys.IsEncrypt, Keys.IsRevoked, "        \
                                "Keys.IsErasable, Keys.TotalBytes, "                 \
                                "KeyOffsets.UsedBytes FROM Keys LEFT OUTER JOIN "    \
                                "KeyOffsets ON Keys.Id == KeyOffsets.KeyId;"
#endif

/*
 * NOTE: Multi-chunk discovery is performed using offset arithmetic on the
 *       ChunkSize and TotalBytes columns recorded in the Keys table rather
 *       than via a separate query into the KeyChunks table.  The chunk file
 *       naming convention is established by getChunkName() in navajoInt.c.
 */

#ifndef ACK_GETKEYINFO_SQL
#define ACK_GETKEYINFO_SQL      "SELECT Keys.Id, Keys.KeySetId, Keys.KeyGroupId, "   \
                                "Keys.Name, Keys.IsEncrypt, Keys.IsArchive, "        \
                                "Keys.IsRevoked, Keys.IsErasable, Keys.TotalBytes, " \
                                "KeyOffsets.ChunkId, KeyOffsets.UsedBytes, "         \
                                "KeyOffsets.Offset, Keys.ChunkSize, Keys.FileName "  \
                                "FROM Keys LEFT OUTER JOIN KeyOffsets ON Keys.Id "   \
                                "== KeyOffsets.KeyId WHERE Keys.Id = ?;"
#endif

#ifndef ACK_GETALLPROPS_SQL
#define ACK_GETALLPROPS_SQL     "SELECT Id, Type, Name, Value FROM KeyProperties "   \
                                "WHERE KeyId = %Q ORDER BY KeyId, Name;"
#endif

#ifndef ACK_GETKEYPROP_SQL
#define ACK_GETKEYPROP_SQL      "SELECT Id, Type, Name, Value FROM KeyProperties "   \
                                "WHERE KeyId = ? AND Name = ? ORDER BY KeyId, Name;"
#endif

#ifndef ACK_GETKEYBYTES_SQL
#define ACK_GETKEYBYTES_SQL     "SELECT KeyOffsets.KeyId, KeyOffsets.ChunkId, "      \
                                "KeyOffsets.UsedBytes, KeyOffsets.Offset FROM "      \
                                "KeyOffsets LEFT OUTER JOIN Chunks ON "              \
                                "KeyOffsets.ChunkId == Chunks.Id WHERE "             \
                                "KeyOffsets.KeyId = ?;"
#endif

#ifndef ACK_UPDATE_PROP_SQL
#define ACK_UPDATE_PROP_SQL     "INSERT OR REPLACE INTO KeyProperties (KeyId, "      \
                                "Type, Name, Value) VALUES (?, ?, ?, ?);"
#endif

#ifndef ACK_DELETE_PROP_SQL
#define ACK_DELETE_PROP_SQL     "DELETE FROM KeyProperties WHERE KeyId =? AND Name " \
                                "= ?;"
#endif

#ifndef ACK_UPDATE_OFFSET_SQL
#define ACK_UPDATE_OFFSET_SQL   "UPDATE KeyOffsets SET Offset = Offset + ?, "        \
                                "UsedBytes = UsedBytes + ? WHERE KeyId = ?"
#endif

/*
 * NOTE: The multi-chunk variant of the offset update.  Unlike the legacy
 *       statement above, this one SETS the (ChunkId, Offset) pair to a new
 *       post-read position computed by the engine, while still accumulating
 *       the used-bytes counter.
 */

#ifndef ACK_UPDATE_CHUNKOFFSET_SQL
#define ACK_UPDATE_CHUNKOFFSET_SQL "UPDATE KeyOffsets SET ChunkId = ?, Offset = ?, " \
                                   "UsedBytes = UsedBytes + ? WHERE KeyId = ?"
#endif

#if defined(FEATURE_RESET_CACHED_KEYS)
#ifndef ACK_RESET_OFFSET_SQL
#define ACK_RESET_OFFSET_SQL    "UPDATE KeyOffsets SET ChunkId = ?, Offset = ?, "    \
                                "UsedBytes = ? WHERE KeyId = ?"
#endif
#endif /* defined(FEATURE_RESET_CACHED_KEYS) */

#ifndef ACK_KEY_CHUNK_TABLE
#define ACK_KEY_CHUNK_TABLE     "Chunks"
#endif

#ifndef ACK_KEY_BLOB_COLUMN
#define ACK_KEY_BLOB_COLUMN     "Data"
#endif

/*****************************************************************************/

ACK_PRIVATE ACK_RESULT beginTransaction(
    sqlite3 *db /* in */
);

ACK_PRIVATE ACK_RESULT commitTransaction(
    sqlite3 *db /* in */
);

ACK_PRIVATE ACK_RESULT rollbackTransaction(
    sqlite3 *db /* in */
);

ACK_PRIVATE ACK_RESULT attachDatabase(
    sqlite3 *db,     /* in */
    LPCSTR fileName, /* in */
    LPCSTR name      /* in */
);

ACK_PRIVATE ACK_RESULT detachDatabase(
    sqlite3 *db, /* in */
    LPCSTR name  /* in */
);

ACK_PRIVATE int prepareAndExecute(
    sqlite3 *db,          /* in */
    LPCSTR zSql,          /* in */
    LPCSTR zData1,        /* in */
    BOOL force1,          /* in */
    LPCSTR zData2,        /* in */
    BOOL force2,          /* in */
    LPCSTR zData3,        /* in */
    BOOL force3,          /* in */
    LPCSTR zData4,        /* in */
    BOOL force4,          /* in */
    sqlite3_stmt **ppStmt /* in, out */
);

#endif /* _NAVAJO_SQL_H_ */

/* end of file */
