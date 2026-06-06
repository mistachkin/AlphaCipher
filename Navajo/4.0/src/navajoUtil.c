/*
 * navajoUtil.c -- Private Utility Helper API
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
#include <string.h>

#if defined(__SYMBIAN32__)
#include <ctype.h>
#include <unistd.h>

#include "hresult.h"
#endif

#include "navajoPort.h"
#include "navajo.h"
#include "navajoSqlite.h"
#include "navajoIntTypes.h"
#include "navajoCache.h"
#include "navajoInt.h"
#include "navajoUtil.h"
#include "os.h"

VOID ACK_initializeMutex( /* PRIVATE */
    ACK_LPMUTEX pMutex
    )
{
    osInitializeMutex(pCurrentOs, pMutex);
}

VOID ACK_finalizeMutex( /* PRIVATE */
    ACK_LPMUTEX pMutex
    )
{
    osFinalizeMutex(pCurrentOs, pMutex);
}

VOID ACK_lockMutex( /* PRIVATE */
    ACK_LPMUTEX pMutex
    )
{
    osLockMutex(pCurrentOs, pMutex);
}

VOID ACK_unlockMutex( /* PRIVATE */
    ACK_LPMUTEX pMutex
    )
{
    osUnlockMutex(pCurrentOs, pMutex);
}

INT ACK_errno( /* PRIVATE */
    )
{
    return osErrno(pCurrentOs);
}

LPSTR ACK_getcwd( /* PRIVATE */
    LPSTR buf,
    SIZE_T size
    )
{
    return osGetcwd(pCurrentOs, buf, size);
}

LPDIR ACK_opendir( /* PRIVATE */
    LPCSTR dirname
    )
{
    return osOpendir(pCurrentOs, dirname);
}

LPDIRENT ACK_readdir( /* PRIVATE */
    LPDIR dirp
    )
{
    return osReaddir(pCurrentOs, dirp);
}

INT ACK_closedir( /* PRIVATE */
    LPDIR dirp
    )
{
    return osClosedir(pCurrentOs, dirp);
}

BOOL ACK_isdir( /* PRIVATE */
    LPCSTR dirname,
    LPDIRENT direntp
    )
{
    return osIsdir(pCurrentOs, dirname, direntp);
}

UINT64 ACK_atoull( /* PRIVATE */
    LPCSTR str
    )
{
    return osAtoull(pCurrentOs, str);
}

LPSTR ACK_itoa( /* PRIVATE */
    INT val
    )
{
    return osItoa(pCurrentOs, val);
}

LPSTR ACK_i64toa( /* PRIVATE */
    INT64 val
    )
{
    return osI64toa(pCurrentOs, val);
}

LPSTR ACK_ui64toa( /* PRIVATE */
    UINT64 val
    )
{
    return osUi64toa(pCurrentOs, val);
}

LPSTR ACK_strcat( /* PRIVATE */
    LPSTR dest,
    LPCSTR src
    )
{
    ACK_ASSERT(dest != NULL && "ACK_strcat");
    ACK_ASSERT(src != NULL && "ACK_strcat");

    return osStrcat(pCurrentOs, dest, src);
}

LPSTR ACK_strdup( /* PRIVATE */
    LPCSTR str
    )
{
    ACK_ASSERT(str != NULL && "ACK_strdup");

    return osStrdup(pCurrentOs, str);
}

LPSTR ACK_strdup2( /* PRIVATE */
    LPCSTR str,
    SIZE_T extra
    )
{
    SIZE_T length = 0;
    SIZE_T newLength = 0;
    LPSTR str2 = NULL;

    ACK_ASSERT(str != NULL && "ACK_strdup2");

    if (str == NULL)
        return NULL;

    length = strlen(str); newLength = length + extra + 1;
    str2 = (LPSTR)osZalloc(pCurrentOs, sizeof(CHAR) * newLength);

    if (str2 == NULL)
        return NULL;

    memcpy(str2, str, length);

    return str2;
}

#if defined(FEATURE_IMPORT)
INT ACK_fseek( /* PRIVATE */
    FILE *stream,
    INT64 offset,
    INT whence
    )
{
    ACK_ASSERT(stream != NULL && "ACK_fseek");

    return osFseek(pCurrentOs, stream, offset, whence);
}

INT64 ACK_ftell( /* PRIVATE */
    FILE *stream
    )
{
    ACK_ASSERT(stream != NULL && "ACK_ftell");

    return osFtell(pCurrentOs, stream);
}

ERRNO_T ACK_ftruncate( /* PRIVATE */
    FILE *stream,
    INT64 size
    )
{
    ACK_ASSERT(stream != NULL && "ACK_ftruncate");

    return osFtruncate(pCurrentOs, stream, size);
}
#endif /* defined(FEATURE_IMPORT) */

LPCSTR formatGuid( /* PRIVATE */
    LPCGUID pGuid,
    BOOL allocate,
    BOOL dashes,
    BOOL braces
    )
{
    LPCSTR formats[] = {
        "{%08lx-%04hx-%04hx-%02x%02x-%02x%02x%02x%02x%02x%02x}",
        "%08lx-%04hx-%04hx-%02x%02x-%02x%02x%02x%02x%02x%02x",
        "{%08lx%04hx%04hx%02x%02x%02x%02x%02x%02x%02x%02x}",
        "%08lx%04hx%04hx%02x%02x%02x%02x%02x%02x%02x%02x",
        NULL
    };
    LPCSTR format = NULL;

    /*
     * BUGBUG: This is not thread-safe (static buffer).
     */

    static CHAR buffer[MAX_GUID_SPACE + 1];

    if (pGuid == NULL)
        return NULL;

    if (dashes && braces)
        format = formats[0];
    else if (dashes)
        format = formats[1];
    else if (braces)
        format = formats[2];
    else
        format = formats[3];

    /*
     * NOTE: Technically, the "%x" and "%X" format specifiers
     *       to printf require an unsigned int (C89).
     */
    snprintf(buffer, MAX_GUID_SPACE, format, pGuid->Data1,
        pGuid->Data2, pGuid->Data3, (unsigned int)pGuid->Data4[0],
        (unsigned int)pGuid->Data4[1], (unsigned int)pGuid->Data4[2],
        (unsigned int)pGuid->Data4[3], (unsigned int)pGuid->Data4[4],
        (unsigned int)pGuid->Data4[5], (unsigned int)pGuid->Data4[6],
        (unsigned int)pGuid->Data4[7]);

    buffer[MAX_GUID_SPACE] = '\0';

    return allocate ? ACK_strdup(buffer) : buffer;
}

BOOL isDirectorySeparator( /* PRIVATE */
    CHAR character
    )
{
    return ((character == '/') || (character == '\\'));
}

BOOL hasDirectoryName( /* PRIVATE */
    LPCSTR fileName
    )
{
    if (fileName == NULL)
        return FALSE;

    while (*fileName) {
        CHAR character = *fileName++;

        if (isDirectorySeparator(character))
            return TRUE;
    }

    return FALSE;
}

BOOL truncateFileName( /* PRIVATE */
    LPSTR fileName
    )
{
    if (fileName != NULL) {
        SIZE_T length = strlen(fileName);

        if (length > 0) {
            while (--length) {
                if (isDirectorySeparator(fileName[length])) {
                    /*
                     * NOTE: Replace this directory separator with
                     *       a null character (effectively getting
                     *       rid of the trailing file name).
                     */
                    fileName[length] = '\0';

                    /*
                     * NOTE: We successfully found the final
                     *       directory separator and truncated the
                     *       string at that point, return the copied
                     *       and modified string.
                     */
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

LPCSTR getFileNameOnly( /* PRIVATE */
    LPCSTR fileName
    )
{
    if (fileName != NULL) {
        SIZE_T length = strlen(fileName);

        if (length > 0) {
            while (--length) {
                if (isDirectorySeparator(fileName[length])) {
                    while (isDirectorySeparator(fileName[length]))
                        length++;

                    return (fileName + length);
                }
            }
        }
    }

    return fileName;
}

LPSTR makeQualifiedFileName( /* PRIVATE */
    LPCSTR baseFileName,
    LPCSTR fileNameOnly,
    LPCSTR suffixOnly
    )
{
    SIZE_T newLength = 0;
    LPSTR newFileName = NULL;

    if (baseFileName == NULL)
        return NULL;

    if (fileNameOnly == NULL)
        return NULL;

    if (!hasDirectoryName(baseFileName))
        return NULL;

    if (hasDirectoryName(fileNameOnly))
        return NULL;

    if (suffixOnly != NULL)
        newLength += strlen(suffixOnly) + 1; /* '_' + "$suffix" */

    newLength += strlen(baseFileName) + strlen(fileNameOnly) + 2;
    newFileName = (LPSTR)ACK_zalloc(sizeof(CHAR) * newLength);

    if (newFileName == NULL)
        return NULL;

    /*
     * NOTE: The buffer was zalloc'd above to a length strictly greater
     *       than strlen(baseFileName) + 1, so memcpy of the trailing NUL
     *       is safe and avoids the unbounded strcpy.
     */

    memcpy(newFileName, baseFileName, strlen(baseFileName) + 1);
    truncateFileName(newFileName);

#if defined(WIN32)
    ACK_strcat(newFileName, "\\");
#else
    ACK_strcat(newFileName, "/");
#endif

    ACK_strcat(newFileName, fileNameOnly);

    if (suffixOnly != NULL) {
        ACK_strcat(newFileName, "_");
        ACK_strcat(newFileName, suffixOnly);
    }

    return newFileName;
}

#if defined(__SYMBIAN32__)
ACK_RESULT symbianDbInit( /* PRIVATE */
    ACK_LPSESSIONINFO pSessionInfo,
    LPSTR database,
    LPSTR zVfs
    )
{
    /*
     * HACK: Since the SQLite compiled for Symbian uses the Unix VFS and the OS
     *       itself seems to use Windows style file names, replace the Unix
     *       VFS path resolution function with our own.
     */

    sqlite3_vfs *pVfs = sqlite3_vfs_find(zVfs);

    UNUSED_ARGUMENT(pSessionInfo);
    UNUSED_ARGUMENT(database);

    if (pVfs == NULL)
        return E_NOINTERFACE;

    if (pVfs->xFullPathname != symbianFullPathname)
        pVfs->xFullPathname = symbianFullPathname;

    /*
     * HACK: Make sure the temporary directory used by SQLite uses the proper
     *       file name syntax.
     */

    if (sqlite3_temp_directory == NULL)
        sqlite3_temp_directory = sqlite3_mprintf("%s", TMP_DIR);

    return S_OK;
}

int symbianFullPathname( /* PRIVATE */
    sqlite3_vfs *pVfs, /* Pointer to vfs object */
    const char *zPath, /* Possibly relative input path */
    int nOut,          /* Size of output buffer in bytes */
    char *zOut         /* Output buffer */
  )
{
  SIZE_T nPath = 0;

  UNUSED_ARGUMENT(pVfs);

  zOut[nOut - 1] = '\0';
  nPath = strlen(zPath);

  if ((nPath >= 3) && isalpha(zPath[0]) && (zPath[1] == ':') &&
          isDirectorySeparator(zPath[2])) {
    snprintf(zOut, nOut, "%s", zPath);
  } else {
    SIZE_T nCwd = 0;

    if (getcwd(zOut, nOut - 1) == 0) {
      return SQLITE_CANTOPEN;
    }

    nCwd = strlen(zOut);
    snprintf(&zOut[nCwd], nOut - nCwd, "\\%s", zPath);
  }

  return SQLITE_OK;
}
#endif /* defined(__SYMBIAN32__) */

/* end of file */
