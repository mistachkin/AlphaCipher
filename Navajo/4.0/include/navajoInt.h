/*
 * navajoInt.h -- Private Engine API
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

#if defined(_MSC_VER) && defined(WIN32) && !defined(_WIN32_WCE) && !defined(_INC_STDIO)
#error "The header file <stdio.h> must be included prior to this file."
#endif

#if defined(_MSC_VER) && defined(WIN32) && !defined(_WIN32_WCE) && !defined(_INC_STDLIB)
#error "The header file <stdlib.h> must be included prior to this file."
#endif

#if !defined(_SQLITE3_H_) && !defined(SQLITE3_H)
#error "The header file \"sqlite3.h\" must be included prior to this file."
#endif

#if !defined(_NAVAJO_PORT_H_)
#error "The header file \"navajoPort.h\" must be included prior to this file."
#endif

#if !defined(_NAVAJO_H_)
#error "The header file \"navajo.h\" must be included prior to this file."
#endif

#if !defined(_NAVAJO_INT_TYPES_H_)
#error "The header file \"navajoIntTypes.h\" must be included prior to this file."
#endif

#ifndef _NAVAJO_INT_H_
#define _NAVAJO_INT_H_

/*****************************************************************************/

#ifndef UNUSED_ARGUMENT
#define UNUSED_ARGUMENT(arg) if (arg)
#endif

/*****************************************************************************/

#ifndef MAX_INTEGER_SPACE
#define MAX_INTEGER_SPACE                     24
#endif

#ifndef MAX_VERSION_SPACE
#define MAX_VERSION_SPACE                    200
#endif

#ifndef MAX_GUID_SPACE
#define MAX_GUID_SPACE                        38
#endif

/*****************************************************************************/

/*
 * NOTE: This value represents the maximum "safe" blob size supported by the
 *       stock SQLite library.  The actual stated limit (SQLITE_MAX_LENGTH in
 *       the SQLite source code) is 1000000000 bytes; however, all attempts to
 *       open a handle to a blob of that size seem to fail with SQLITE_TOOBIG.
 *       It may be possible to use a value between the one here and the actual
 *       stated limit.
 */

#ifndef MAX_CHUNK_SIZE
#define MAX_CHUNK_SIZE               (900000000)
#endif

#ifndef MAX_CHUNK_COUNT
#define MAX_CHUNK_COUNT                    (500)
#endif

#ifndef MAX_KEY_SIZE
#define MAX_KEY_SIZE  (((UINT64)(MAX_CHUNK_SIZE))*((UINT64)(MAX_CHUNK_COUNT)))
#endif

/*****************************************************************************/

#if !defined(_WIN32_WCE)
    /*
     * NOTE: Unfortunately, Windows CE appears to totally lack support for the
     *       global "errno" variable from the ANSI C standard; however, all
     *       other platforms appear to support it so far.
     */

    #define HAVE_ERRNO
#endif /* !defined(_WIN32_WCE) */

/*****************************************************************************/

#if defined(__SYMBIAN32__)
    /*
     * NOTE: Make sure that the build environment knows we are importing the
             SQLite functions from an external library.
     */

    #undef SQLITE_API
    #define SQLITE_API IMPORT_C

    /*
     * NOTE: What is the SQLite VFS name for this operating system?  A value of
     *       NULL here means "use the default".
     */

    #ifndef VFS_NAME
        #define VFS_NAME "unix-none"
    #endif

    /*
     * NOTE: What is the name of the environment variable where the temporary
     *       directory should be stored?
     */

    #ifndef ENV_TMP_DIR
        #define ENV_TMP_DIR "TMPDIR"
    #endif

    /*
     * NOTE: Where should temporary files be stored?
     */

    #ifndef TMP_DIR
        #define TMP_DIR "E:\\"
    #endif
#else
    /*
     * NOTE: What is the SQLite VFS name for this operating system?  A value of
     *       NULL here means "use the default".
     */

    #ifndef VFS_NAME
        #define VFS_NAME NULL
    #endif
#endif /* defined(__SYMBIAN32__) */

/******************* COLUMNS FOR "ACK_VERIFY_DATABASE_SQL" *******************/

#ifndef VERIFYDATABASE_COLUMN_STATUS
#define VERIFYDATABASE_COLUMN_STATUS         (0)
#endif

#ifndef VERIFYDATABASE_COLUMNS
#define VERIFYDATABASE_COLUMNS               (1)
#endif

#ifndef VERIFYDATABASE_STATUS_VALUE
#define VERIFYDATABASE_STATUS_VALUE         "ok"
#endif

/********************* COLUMNS FOR "ACK_GETALLKEYS_SQL" *********************/

#ifndef ALLKEYINFO_COLUMN_ID
#define ALLKEYINFO_COLUMN_ID                 (0)
#endif

#ifndef ALLKEYINFO_COLUMN_SETID
#define ALLKEYINFO_COLUMN_SETID              (1)
#endif

#ifndef ALLKEYINFO_COLUMN_GROUPID
#define ALLKEYINFO_COLUMN_GROUPID            (2)
#endif

#ifndef ALLKEYINFO_COLUMN_NAME
#define ALLKEYINFO_COLUMN_NAME               (3)
#endif

#ifndef ALLKEYINFO_COLUMN_ISENCRYPT
#define ALLKEYINFO_COLUMN_ISENCRYPT          (4)
#endif

#ifndef ALLKEYINFO_COLUMN_ISREVOKED
#define ALLKEYINFO_COLUMN_ISREVOKED          (5)
#endif

#ifndef ALLKEYINFO_COLUMN_ISERASABLE
#define ALLKEYINFO_COLUMN_ISERASABLE         (6)
#endif

#ifndef ALLKEYINFO_COLUMN_TOTALBYTES
#define ALLKEYINFO_COLUMN_TOTALBYTES         (7)
#endif

#ifndef ALLKEYINFO_COLUMN_USEDBYTES
#define ALLKEYINFO_COLUMN_USEDBYTES          (8)
#endif

#ifndef ALLKEYINFO_COLUMNS
#define ALLKEYINFO_COLUMNS                   (9)
#endif

/********************* COLUMNS FOR "ACK_GETALLPROPS_SQL" *********************/

#ifndef ALLKEYPROP_COLUMN_ID
#define ALLKEYPROP_COLUMN_ID                 (0)
#endif

#ifndef ALLKEYPROP_COLUMN_TYPE
#define ALLKEYPROP_COLUMN_TYPE               (1)
#endif

#ifndef ALLKEYPROP_COLUMN_NAME
#define ALLKEYPROP_COLUMN_NAME               (2)
#endif

#ifndef ALLKEYPROP_COLUMN_VALUE
#define ALLKEYPROP_COLUMN_VALUE              (3)
#endif

#ifndef ALLKEYPROP_COLUMNS
#define ALLKEYPROP_COLUMNS                   (4)
#endif

/********************* COLUMNS FOR "ACK_GETKEYINFO_SQL" *********************/

#ifndef ONEKEYINFO_COLUMN_ID
#define ONEKEYINFO_COLUMN_ID                 (0)
#endif

#ifndef ONEKEYINFO_COLUMN_SETID
#define ONEKEYINFO_COLUMN_SETID              (1)
#endif

#ifndef ONEKEYINFO_COLUMN_GROUPID
#define ONEKEYINFO_COLUMN_GROUPID            (2)
#endif

#ifndef ONEKEYINFO_COLUMN_NAME
#define ONEKEYINFO_COLUMN_NAME               (3)
#endif

#ifndef ONEKEYINFO_COLUMN_ISENCRYPT
#define ONEKEYINFO_COLUMN_ISENCRYPT          (4)
#endif

#ifndef ONEKEYINFO_COLUMN_ISARCHIVE
#define ONEKEYINFO_COLUMN_ISARCHIVE          (5)
#endif

#ifndef ONEKEYINFO_COLUMN_ISREVOKED
#define ONEKEYINFO_COLUMN_ISREVOKED          (6)
#endif

#ifndef ONEKEYINFO_COLUMN_ISERASABLE
#define ONEKEYINFO_COLUMN_ISERASABLE         (7)
#endif

#ifndef ONEKEYINFO_COLUMN_TOTALBYTES
#define ONEKEYINFO_COLUMN_TOTALBYTES         (8)
#endif

#ifndef ONEKEYINFO_COLUMN_CHUNKID
#define ONEKEYINFO_COLUMN_CHUNKID            (9)
#endif

#ifndef ONEKEYINFO_COLUMN_USEDBYTES
#define ONEKEYINFO_COLUMN_USEDBYTES         (10)
#endif

#ifndef ONEKEYINFO_COLUMN_OFFSET
#define ONEKEYINFO_COLUMN_OFFSET            (11)
#endif

#ifndef ONEKEYINFO_COLUMN_CHUNKSIZE
#define ONEKEYINFO_COLUMN_CHUNKSIZE         (12)
#endif

#ifndef ONEKEYINFO_COLUMN_FILENAME
#define ONEKEYINFO_COLUMN_FILENAME          (13)
#endif

#ifndef ONEKEYINFO_COLUMNS
#define ONEKEYINFO_COLUMNS                  (14)
#endif

/********************* COLUMNS FOR "ACK_GETALLPROPS_SQL" *********************/

#ifndef ONEKEYPROP_COLUMN_ID
#define ONEKEYPROP_COLUMN_ID                 (0)
#endif

#ifndef ONEKEYPROP_COLUMN_TYPE
#define ONEKEYPROP_COLUMN_TYPE               (1)
#endif

#ifndef ONEKEYPROP_COLUMN_NAME
#define ONEKEYPROP_COLUMN_NAME               (2)
#endif

#ifndef ONEKEYPROP_COLUMN_VALUE
#define ONEKEYPROP_COLUMN_VALUE              (3)
#endif

#ifndef ONEKEYPROP_COLUMNS
#define ONEKEYPROP_COLUMNS                   (4)
#endif

/*****************************************************************************/

#define ACK_MAXIMUM_READ_SIZE         (52428800)

/*****************************************************************************/

#if defined(_DEBUG)
    #ifndef ACK_CHECK_SQLITE_RC
        #define ACK_CHECK_SQLITE_RC(a,b,c)                                 \
            do {                                                           \
                if ((b) != (c)) {                                          \
                    fprintf(stdout, "SQLite function \"%s\" returned %d, " \
                        "expected %d, file \"%s\", line %d (CHECK)\n",     \
                        #a, (b), (c), __FILE__, __LINE__);                 \
                }                                                          \
            } while (0)
    #endif

    #ifndef ACK_ASSERT_SQLITE_RC
        #define ACK_ASSERT_SQLITE_RC(a,b,c)                                \
            do {                                                           \
                if ((b) != (c)) {                                          \
                    fprintf(stdout, "SQLite function \"%s\" returned %d, " \
                        "expected %d, file \"%s\", line %d (ASSERT)\n",    \
                        #a, (b), (c), __FILE__, __LINE__);                 \
                    getchar(); /* NOTE: Pause. */                          \
                    abort();                                               \
                }                                                          \
            } while (0)
    #endif
#else
    #ifndef ACK_CHECK_SQLITE_RC
        #define ACK_CHECK_SQLITE_RC(a,b,c)  ((void)0)
    #endif

    #ifndef ACK_ASSERT_SQLITE_RC
        #define ACK_ASSERT_SQLITE_RC(a,b,c)  ((void)0)
    #endif
#endif /* defined(_DEBUG) */

/*****************************************************************************/

#if defined(_DEBUG)
    #ifndef ACK_ASSERT
        #define ACK_ASSERT(a)                                                 \
            do {                                                              \
                if (!(a)) {                                                   \
                    fprintf(stdout, "ACK_ASSERT(%s), file \"%s\", line %d\n", \
                        #a, __FILE__, __LINE__);                              \
                    getchar(); /* NOTE: Pause. */                             \
                    abort();                                                  \
                }                                                             \
            } while (0)
    #endif
#else
    #ifndef ACK_ASSERT
        #define ACK_ASSERT(a)  ((void)0)
    #endif
#endif /* defined(_DEBUG) */

/*****************************************************************************/

#ifndef ACK_MAX
    #define ACK_MAX(a,b)  (((a) >= (b)) ? (a) : (b))
#endif

#ifndef ACK_MIN
    #define ACK_MIN(a,b)  (((a) <= (b)) ? (a) : (b))
#endif

/*****************************************************************************/

#if !defined(FEATURE_AUTO_ALIGNMENT)
    #ifndef ACK_XOR
        #define ACK_XOR(a,b,c,d)                          \
            do {                                          \
                UINT64 index = 0;                         \
                for (; index < (d); index++) {            \
                    (a)[index] = (b)[index] ^ (c)[index]; \
                }                                         \
            } while (0)
    #endif
#else
    #ifndef ACK_XOR
        #define ACK_XOR(a,b,c,d)                                     \
            do {                                                     \
                UINT64 index = 0;                                    \
                for (; index < ((d) / sizeof(UINT)); index++) {      \
                    ((LPUINT)(a))[index] =                           \
                        ((LPUINT)(b))[index] ^ ((LPUINT)(c))[index]; \
                }                                                    \
                index *= sizeof(UINT);                               \
                for (; index < (d); index++) {                       \
                    (a)[index] = (b)[index] ^ (c)[index];            \
                }                                                    \
            } while (0)
    #endif
#endif

/*****************************************************************************/

#ifndef ACK_GET_OFFSET
    #define ACK_GET_OFFSET(a,b,c)   ((a) ? ((b).offset) : (*(c)))
#endif

/*****************************************************************************/

#ifndef ACK_GET_OFFSET2
    #define ACK_GET_OFFSET2(a,b,c)  ((a) ? 0 : (*(c)))
#endif

/*****************************************************************************/

extern ACK_LPOS pCurrentOs; /* NOTE: Current OS interface pointer. */

/*****************************************************************************/

ACK_PRIVATE VOID freeSessionInfo( /* NOTE: Assumes session mutex is held. */
    ACK_LPSESSIONINFO pSessionInfo /* in */
);

ACK_PRIVATE ACK_RESULT getChunkIdAndOffset(
    UINT64 chunkSize,        /* in */
    UINT64 offset,           /* in */
    ACK_LPBLOBINFO pBlobInfo /* out */
);

ACK_PRIVATE ACK_RESULT getChunkName(
    LPCSTR fileName,  /* in */
    UINT64 chunkId,   /* in */
    LPSTR *pChunkName /* in, out */
);

ACK_PRIVATE ACK_RESULT getKeyInfo(
    sqlite3 *db,                /* in */
    LPCSTR keyId,               /* in */
    ACK_LPINTKEYINFO *ppKeyInfo /* in, out */
);

ACK_PRIVATE VOID freeKeyInfo(
    ACK_LPINTKEYINFO pKeyInfo /* in */
);

ACK_PRIVATE ACK_RESULT buildChunkFilePath(
    ACK_LPSESSIONINFO pSessionInfo, /* in */
    LPCSTR baseFileName,            /* in */
    UINT64 chunkId,                 /* in */
    LPSTR *pFilePath                /* in, out */
);

ACK_PRIVATE ACK_RESULT planChunkRange(
    ACK_LPSESSIONINFO pSessionInfo, /* in */
    ACK_LPINTKEYINFO pKeyInfo,      /* in */
    UINT64 chunkId,                 /* in */
    UINT64 chunkOffset,             /* in */
    SIZE_T size,                    /* in */
    ACK_LPCHUNKPLAN pPlan           /* in, out */
);

ACK_PRIVATE VOID freeChunkPlan(
    ACK_LPCHUNKPLAN pPlan /* in, out */
);

ACK_PRIVATE ACK_RESULT attachChunkPlan( /* NOTE: Must be called outside transaction. */
    sqlite3 *db,          /* in */
    ACK_LPCHUNKPLAN pPlan /* in, out */
);

ACK_PRIVATE ACK_RESULT detachChunkPlan( /* NOTE: Must be called outside transaction. */
    sqlite3 *db,          /* in */
    ACK_LPCHUNKPLAN pPlan /* in, out */
);

ACK_PRIVATE ACK_RESULT updateChunkOffset(
    sqlite3 *db,    /* in */
    LPCSTR keyId,   /* in */
    UINT64 chunkId, /* in */
    UINT64 offset,  /* in */
    SIZE_T size     /* in */
);

ACK_PRIVATE ACK_RESULT getKeyBytes( /* NOTE: Also see getCachedKeyBytes. */
    ACK_LPSESSIONINFO pSessionInfo, /* in */
    LPCSTR keyId,                   /* in */
    SIZE_T size,                    /* in */
    BOOL encrypt,                   /* in */
    LPUINT64 pOffset,               /* in, out */
    LPBYTE *ppKey,                  /* in, out */
    LPSIZE_T pKeySize               /* in, out */
);

/*****************************************************************************/

#endif /* _NAVAJO_INT_H_ */

/* end of file */
