/*
 * navajoIntTypes.h -- Private Types
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

#if !defined(_SQLITE3_H_) && !defined(SQLITE3_H)
#error "The header file \"sqlite3.h\" must be included prior to this file."
#endif

#if !defined(_NAVAJO_H_)
#error "The header file \"navajo.h\" must be included prior to this file."
#endif

#ifndef _NAVAJO_INT_TYPES_H_
#define _NAVAJO_INT_TYPES_H_

/*****************************************************************************/

#ifndef _LPDIRENT_DEFINED
#define _LPDIRENT_DEFINED
typedef DIRENT *LPDIRENT;
#endif

#ifndef _LPDIR_DEFINED
#define _LPDIR_DEFINED
typedef DIR *LPDIR;
#endif

/*****************************************************************************/

#ifndef _ACK_OS_DEFINED
#define _ACK_OS_DEFINED
typedef struct ACK_OS_tag {
    LPCSTR zName;       /* Name of this virtual file system */
    LPVOID pAppData;    /* Pointer to application-specific data */
    ACK_RESULT (*xInitialize)(LPVOID pOs, LPVOID pAppData);
    ACK_RESULT (*xFinalize)(LPVOID pOs, LPVOID pAppData);
    VOID (*xInitializeMutex)(ACK_LPMUTEX pMutex);
    VOID (*xFinalizeMutex)(ACK_LPMUTEX pMutex);
    VOID (*xLockMutex)(ACK_LPMUTEX pMutex);
    VOID (*xUnlockMutex)(ACK_LPMUTEX pMutex);
    INT (*xErrno)(VOID);
    LPVOID (*xMalloc)(SIZE_T size);
    LPVOID (*xZalloc)(SIZE_T size);
    LPVOID (*xRealloc)(LPVOID pBlock, SIZE_T size, BOOL zero);
    SIZE_T (*xMsize)(LPVOID pBlock);
    VOID (*xFree)(LPVOID pBlock);
    LPSTR (*xGetcwd)(LPSTR buf, SIZE_T size);
    LPDIR (*xOpendir)(LPCSTR dirname);
    LPDIRENT (*xReaddir)(LPDIR dirp);
    INT (*xClosedir)(LPDIR dirp);
    BOOL (*xIsdir)(LPCSTR dirname, LPDIRENT direntp);
    UINT64 (*xAtoull)(LPCSTR str);
    LPSTR (*xItoa)(INT val);
    LPSTR (*xI64toa)(INT64 val);
    LPSTR (*xUi64toa)(UINT64 val);
    LPSTR (*xStrcat)(LPSTR dest, LPCSTR src);
    LPSTR (*xStrdup)(LPCSTR str);
    INT (*xFseek)(FILE *stream, INT64 offset, INT whence);
    INT64 (*xFtell)(FILE *stream);
    ERRNO_T (*xFtruncate)(FILE *stream, INT64 size);
} ACK_OS, *ACK_LPOS;
#endif

/*****************************************************************************/

#ifndef _ACK_BLOBINFO_DEFINED
#define _ACK_BLOBINFO_DEFINED
typedef struct ACK_BLOBINFO_tag {
    UINT64 chunkId; /* current chunk Id for this key. */
    UINT64 offset;  /* offset within the current key chunk. */
} ACK_BLOBINFO, *ACK_LPBLOBINFO;
#endif

/*****************************************************************************/

#ifndef _ACK_CACHEDKEY_DEFINED
#define _ACK_CACHEDKEY_DEFINED
typedef struct ACK_CACHEDKEY_tag {
    CHAR aKeyId[ACK_KEYINFO_MAXID]; /* internal storage for key Id. */
    LPSTR keyId;                    /* pointer to actual storage for key Id. */
    ACK_BLOBINFO blobInfo;          /* current chunk Id and offset. */
    BOOL inUse;                     /* is this cache entry in use? */
    BOOL isEncrypt;                 /* non-zero indicates an encrypt request. */
    SIZE_T size;                    /* original read size from database. */
    SIZE_T workSize;                /* remaining bytes of this key entry. */
    UINT64 workOffset;              /* current offset into chunk for key. */
    LPBYTE pKey;                    /* storage for the cached key bytes. */
} ACK_CACHEDKEY, *ACK_LPCACHEDKEY;
#endif

/*****************************************************************************/

#ifndef _ACK_SESSIONINFO_DEFINED
#define _ACK_SESSIONINFO_DEFINED
typedef struct ACK_SESSIONINFO_tag {
    ACK_LPSESSION pSession; /* original pointer passed to CreateSession. */
    ACK_MUTEX mutex;        /* mutex that locks access to this session. */
    ACK_SESSIONTYPE type;   /* session type specified upon creation. */
    LPSTR fileName;         /* original file name passed to MountKeySet. */
    LPSTR directory;        /* the default directory for keysets, if any. */
    sqlite3 *db;            /* database handle for mounted keyset, if any. */
    BOOL transaction;       /* is there a transaction pending? */
    LPSTR keyId;            /* current key Id (as set via SetSessionKey). */
    LPSTR keyFileName;      /* database file name for the current key Id. */
    UINT64 chunkId;         /* last used chunk Id for key. */
    LPBYTE pKey;            /* temporary storage for current key. */
    SIZE_T keySize;         /* size of the temporary key buffer. */
    SIZE_T cacheKeySize;    /* size of a key in the cache. */
    INT nCache;             /* size of the key cache (zero for none). */
    ACK_LPCACHEDKEY pCache; /* key cache itself. */
} ACK_SESSIONINFO, *ACK_LPSESSIONINFO;
#endif

/*****************************************************************************/

#ifndef _ACK_INTKEYINFO_DEFINED
#define _ACK_INTKEYINFO_DEFINED
typedef struct ACK_INTKEYINFO_tag {
    ACK_KEYINFO baseInfo;  /* standard public key info structure. */
    ACK_BLOBINFO blobInfo; /* current chunk Id and offset. */
    UINT64 chunkSize;      /* key chunk size (0 means "dynamic"). */
    LPSTR fileName;        /* relative file name for key database. */
    LPSTR dbName;          /* name of attached key chunk database. */
} ACK_INTKEYINFO, *ACK_LPINTKEYINFO;
#endif

/*****************************************************************************/

#ifndef _ACK_CHUNKPLAN_ENTRY_DEFINED
#define _ACK_CHUNKPLAN_ENTRY_DEFINED
typedef struct ACK_CHUNKPLAN_ENTRY_tag {
    UINT64 chunkId;     /* zero-based chunk index covered by this entry. */
    UINT64 chunkOffset; /* byte offset within the chunk to read from. */
    SIZE_T length;      /* number of bytes to read from this chunk. */
    LPSTR dbName;       /* SQLite attach alias for this chunk. */
    LPSTR fileName;     /* fully qualified file path to the chunk database. */
    BOOL attached;      /* TRUE if this entry was ATTACHed by the engine. */
} ACK_CHUNKPLAN_ENTRY, *ACK_LPCHUNKPLAN_ENTRY;
#endif

/*****************************************************************************/

#ifndef _ACK_CHUNKPLAN_DEFINED
#define _ACK_CHUNKPLAN_DEFINED
typedef struct ACK_CHUNKPLAN_tag {
    INT count;                     /* number of populated entries. */
    INT capacity;                  /* allocated capacity of entries. */
    ACK_LPCHUNKPLAN_ENTRY entries; /* array of plan entries. */
    UINT64 finalChunkId;           /* chunk Id after the request completes. */
    UINT64 finalChunkOffset;       /* offset within the final chunk. */
} ACK_CHUNKPLAN, *ACK_LPCHUNKPLAN;
#endif

/*****************************************************************************/

#endif /* _NAVAJO_INT_TYPES_H_ */

/* end of file */
