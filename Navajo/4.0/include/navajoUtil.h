/*
 * navajoUtil.h -- Private Utility Helper API
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

#if !defined(_NAVAJO_PORT_H_)
#error "The header file \"navajoPort.h\" must be included prior to this file."
#endif

#if !defined(_NAVAJO_H_)
#error "The header file \"navajo.h\" must be included prior to this file."
#endif

#if !defined(_NAVAJO_INT_H_)
#error "The header file \"navajoInt.h\" must be included prior to this file."
#endif

#ifndef _NAVAJO_UTIL_H_
#define _NAVAJO_UTIL_H_

ACK_PRIVATE VOID ACK_initializeMutex(
    ACK_LPMUTEX pMutex /* in */
);

ACK_PRIVATE VOID ACK_finalizeMutex(
    ACK_LPMUTEX pMutex /* in */
);

ACK_PRIVATE VOID ACK_lockMutex(
    ACK_LPMUTEX pMutex /* in */
);

ACK_PRIVATE VOID ACK_unlockMutex(
    ACK_LPMUTEX pMutex /* in */
);

ACK_PRIVATE INT ACK_errno(VOID);

ACK_PRIVATE LPSTR ACK_getcwd(
    LPSTR buf,  /* in */
    SIZE_T size /* in */
);

ACK_PRIVATE LPDIR ACK_opendir(
    LPCSTR dirname /* in */
);

ACK_PRIVATE LPDIRENT ACK_readdir(
    LPDIR dirp /* in */
);

ACK_PRIVATE INT ACK_closedir(
    LPDIR dirp /* in */
);

ACK_PRIVATE BOOL ACK_isdir(
    LPCSTR dirname,  /* in */
    LPDIRENT direntp /* in */
);

ACK_PRIVATE UINT64 ACK_atoull(
    LPCSTR str /* in */
);

ACK_PRIVATE LPSTR ACK_itoa(
    INT val /* in */
);

ACK_PRIVATE LPSTR ACK_i64toa(
    INT64 val /* in */
);

ACK_PRIVATE LPSTR ACK_ui64toa(
    UINT64 val /* in */
);

ACK_PRIVATE LPSTR ACK_strcat(
    LPSTR dest, /* in */
    LPCSTR src  /* in */
);

ACK_PRIVATE LPSTR ACK_strdup(
    LPCSTR str /* in */
);

ACK_PRIVATE LPSTR ACK_strdup2(
    LPCSTR str,  /* in */
    SIZE_T extra /* in */
);

#if defined(FEATURE_IMPORT)
ACK_PRIVATE INT ACK_fseek(
    FILE *stream, /* in */
    INT64 offset, /* in */
    INT whence    /* in */
);

ACK_PRIVATE INT64 ACK_ftell(
    FILE *stream /* in */
);

ACK_PRIVATE ERRNO_T ACK_ftruncate(
    FILE *stream, /* in */
    INT64 size    /* in */
);
#endif /* defined(FEATURE_IMPORT) */

ACK_PRIVATE LPCSTR formatGuid(
    LPCGUID pGuid, /* in */
    BOOL allocate, /* in */
    BOOL dashes,   /* in */
    BOOL braces    /* in */
);

ACK_PRIVATE BOOL isDirectorySeparator(
    CHAR character /* in */
);

ACK_PRIVATE BOOL hasDirectoryName(
    LPCSTR fileName /* in */
);

ACK_PRIVATE BOOL truncateFileName(
    LPSTR fileName /* in */
);

ACK_PRIVATE LPCSTR getFileNameOnly(
    LPCSTR fileName /* in */
);

ACK_PRIVATE LPSTR makeQualifiedFileName(
    LPCSTR baseFileName, /* in */
    LPCSTR fileNameOnly, /* in */
    LPCSTR suffixOnly    /* in */
);

#if defined(__SYMBIAN32__)
ACK_PRIVATE ACK_RESULT symbianDbInit(
    ACK_LPSESSIONINFO pSessionInfo, /* in */
    LPSTR database,                 /* in */
    LPSTR zVfs                      /* in */
);

ACK_PRIVATE int symbianFullPathname(
    sqlite3_vfs *pVfs, /* in */
    const char *zPath, /* in */
    int nOut,          /* in */
    char *zOut         /* in */
);
#endif /* defined(__SYMBIAN32__) */
#endif /* _NAVAJO_UTIL_H_ */

/* end of file */
