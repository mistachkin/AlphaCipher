/*
 * test.c --
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

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_MSC_VER) && !defined(_WIN32_WCE)
#include <crtdbg.h>
#endif

#define SQLITE_STATIC_LIB

#include "navajoPort.h"
#include "navajo.h"
#include "navajoProtocol.h"
#include "puke.h"

#if !defined(__SYMBIAN32__) && defined(ACK_STATIC_LIB)
#include "navajoSqlite.h"
#include "navajoIntTypes.h"
#include "navajoInt.h"
#include "navajoUtil.h"
#endif

#include "hresult.h"

#if (defined(_POSIX_VERSION) && _POSIX_VERSION >= 200112L) || \
        defined(HAVE_GETTIMEOFDAY)
#include <sys/time.h>
#endif

#include "test.h"

VOID testPause(
    BOOL force
    )
{
#if !defined(__SYMBIAN32__)
    if (force)
#endif
        getchar(); /* NOTE: Pause. */
}

VOID printBuffer(
    LPSTR buffer
    )
{
    #if defined(_WIN32_WCE)
    WCHAR wBuffer[MAX_PATH + 1];
    #endif

    fprintf(stdout, buffer);

    #if defined(_WIN32_WCE)
    MultiByteToWideChar(CP_ACP, 0, buffer,
        MAX_PATH, wBuffer, MAX_PATH);

    MessageBox(NULL, wBuffer, NULL, 0);
    #endif
}

VOID printElapsedTime(
    INT64 startSeconds,
    INT64 stopSeconds,
    INT64 startMicroseconds,
    INT64 stopMicroseconds,
    INT64 iterations,
    LPSTR suffix
    )
{
    CHAR buffer[MAX_PATH + 1];

    if (iterations == 1) {
        snprintf(buffer, MAX_PATH,
            INT64_FORMAT " seconds OR " INT64_FORMAT " microseconds %s\n",
            (stopSeconds - startSeconds),
            (stopMicroseconds - startMicroseconds),
            suffix);
    } else {
        snprintf(buffer, MAX_PATH,
            "%.04f seconds OR %.04f microseconds %s\n",
            (stopSeconds - startSeconds) / (double)iterations,
            (stopMicroseconds - startMicroseconds) / (double)iterations,
            suffix);
    }

    TEST_PRINT(buffer);
}

BOOL getSecondsAndMicroseconds(
    LPINT64 pSeconds,
    LPINT64 pMicroseconds
    )
{
#if defined(WIN32) || defined(_WIN32_WCE)
    static UINT64 frequency = 0;
    UINT64 counter = 0;
    UINT64 microseconds = 0;
    UINT64 seconds = 0;

    if ((pSeconds == NULL) || (pMicroseconds == NULL))
        return FALSE;

    if (frequency == 0) {
        if (QueryPerformanceFrequency((PLARGE_INTEGER)&frequency))
            frequency /= MICROSECONDS_PER_SECOND;
        else
            return FALSE;
    }

    if (!QueryPerformanceCounter((PLARGE_INTEGER)&counter))
        return FALSE;

    microseconds = counter / frequency;
    seconds = microseconds / MICROSECONDS_PER_SECOND;
    /* microseconds -= (seconds * MICROSECONDS_PER_SECOND); */

    *pSeconds = seconds;
    *pMicroseconds = microseconds;

    return TRUE;
#elif (defined(_POSIX_VERSION) && _POSIX_VERSION >= 200112L) || \
        defined(HAVE_GETTIMEOFDAY)
    struct timeval t;

    if (gettimeofday(&t, NULL) != 0)
        return FALSE;

    *pSeconds = (INT64)t.tv_sec;
    *pMicroseconds = (INT64)t.tv_usec;

    return TRUE;
#else
    return FALSE;
#endif
}

VOID printKey(
    ACK_LPKEYINFO pKeyInfo,
    int keyIndex
    )
{
    printf("Key #%d Id \"%s\"\n",
        keyIndex, pKeyInfo->keyId);

    printf("Key #%d Name \"%s\"\n",
        keyIndex, pKeyInfo->keyName);

    printf("Key #%d IsEncrypt \"%s\"\n",
        keyIndex, pKeyInfo->isEncrypt ? "true" : "false");

    printf("Key #%d IsRevoked \"%s\"\n",
        keyIndex, pKeyInfo->isRevoked ? "true" : "false");

    printf("Key #%d IsErasable \"%s\"\n",
        keyIndex, pKeyInfo->isErasable ? "true" : "false");

    printf("Key #%d Total " INT64_FORMAT "\n",
        keyIndex, pKeyInfo->bytesTotal);

    printf("Key #%d Used " INT64_FORMAT "\n",
        keyIndex, pKeyInfo->bytesUsed);
}

VOID testGetAllKeys(VOID)
{
#if !defined(__SYMBIAN32__) && defined(ACK_STATIC_LIB)
    LPDIR dirp = NULL;
    LPDIRENT direntp = NULL;
#endif

    ACK_RESULT kResult = ACK_S_OK;
    ACK_SESSION session = NULL;
    INT64 count = 0;
    ACK_LPKEYINFO pKeyInfo = NULL;
    INT index = 0;

    kResult = ACK_Initialize();

    if (FAILED(kResult))
        TEST_FAIL(kResult, "ACK_Initialize()");

#if !defined(__SYMBIAN32__) && defined(ACK_STATIC_LIB)
    dirp = ACK_opendir("C:\\");

    while ((direntp = ACK_readdir(dirp)) != NULL) {
        printf("#%d = \"%s\"\n",
            (unsigned int)direntp->d_ino,
            direntp->d_name);
    }

    ACK_closedir(dirp);
#endif

    kResult = ACK_CreateSession(&session, ACKST_ListKeys, 0);

    if (FAILED(kResult))
        TEST_FAIL(kResult, "ACK_CreateSession(session)");

    /*
     * NOTE: The master database will always have a file
     *       name similar to "layout.pef".
     */

    kResult = ACK_MountKeySet(session, TEST_DATABASE);

    if (FAILED(kResult))
        TEST_FAIL(kResult, "ACK_MountKeySet(session)");

    kResult = ACK_GetAllKeys(session, ACK_KEYINFO_VERSION, NULL, &count,
        &pKeyInfo);

    if (FAILED(kResult))
        TEST_FAIL(kResult, "ACK_GetAllKeys(session)");

    for (index = 0; index < count; index++)
        printKey(&pKeyInfo[index], index);

    kResult = ACK_FreeAllKeys(session, NULL, &count, &pKeyInfo);

    if (FAILED(kResult))
        TEST_FAIL(kResult, "ACK_FreeAllKeys(session)");

    kResult = ACK_UnmountKeySet(session);

    if (FAILED(kResult))
        TEST_FAIL(kResult, "ACK_UnmountKeySet(session)");

    kResult = ACK_CloseSession(&session);

    if (!SUCCEEDED(kResult))
        TEST_FAIL(kResult, "ACK_CloseSession(session)");

    kResult = ACK_Finalize();

    if (FAILED(kResult))
        TEST_FAIL(kResult, "ACK_Finalize()");
}

VOID testStreamingDirect(VOID)
{
    LPCSTR testVector1 = "Hello World!";
    LPCSTR testVector2 = "It is my honor";
    LPCSTR testVector3 = "to introduce";
    LPCSTR testVector4 = "the new Navajo engine";
    LPCSTR testVector5 = "with streaming.";

    LPCSTR testVectors[TEST_STRINGS] = {
        NULL /* "Hello World!" */,
        NULL /* "It is my honor" */,
        NULL /* "to introduce" */,
        NULL /* "the new Navajo engine" */,
        NULL /* "with streaming." */
    };

    /*
     * NOTE: These counts exclude the trailing NUL terminator; the
     *      caller adds +1 at the ACK_Encrypt call site below.  This
     *      differs from testStreamingProtocol's testLengths array,
     *      which includes the NUL byte.
     */

    SIZE_T testLengths[TEST_STRINGS] = {
        /* "Hello World!" */ 12,
        /* "It is my honor" */ 14,
        /* "to introduce" */ 12,
        /* "the new Navajo engine" */ 21,
        /* "with streaming." */ 15
    };

    ACK_RESULT kResult = ACK_S_OK;
    ACK_SESSION hEncrypt = NULL;
    ACK_SESSION hDecrypt = NULL;
    LPBYTE pOutput = NULL;
    SIZE_T outSize = 0;
    LPBYTE pCompare = NULL;
    SIZE_T compareSize = 0;
    CHAR buffer[MAX_PATH + 1];
    INT index = 0;
    INT64 startSeconds = 0;
    INT64 startMicroseconds = 0;
    INT64 stopSeconds = 0;
    INT64 stopMicroseconds = 0;

    testVectors[0] = testVector1;
    testVectors[1] = testVector2;
    testVectors[2] = testVector3;
    testVectors[3] = testVector4;
    testVectors[4] = testVector5;

    kResult = ACK_Initialize();

    if (FAILED(kResult))
        TEST_FAIL(kResult, "ACK_Initialize()");

    outSize = TEST_BUFSIZE;
    pOutput = (LPBYTE)ACK_zalloc(sizeof(BYTE) * outSize);

    if (pOutput == NULL)
        TEST_FAIL(E_OUTOFMEMORY, "ACK_zalloc(pOutput)");

    compareSize = TEST_BUFSIZE;
    pCompare = (LPBYTE)ACK_zalloc(sizeof(BYTE) * compareSize);

    if (pCompare == NULL)
        TEST_FAIL(E_OUTOFMEMORY, "ACK_zalloc(pCompare)");

    kResult = ACK_CreateSession(&hEncrypt,
        (ACK_SESSIONTYPE)(ACKST_Stream | ACKST_Encrypt), 0);

    if (FAILED(kResult))
        TEST_FAIL(kResult, "ACK_CreateSession(hEncrypt)");

    kResult = ACK_MountKeySet(hEncrypt, TEST_DATABASE);

    if (FAILED(kResult))
        TEST_FAIL(kResult, "ACK_MountKeySet(hEncrypt)");

    kResult = ACK_SetSessionKey(hEncrypt, TEST_KEYID);

    if (FAILED(kResult))
        TEST_FAIL(kResult, "ACK_SetSessionKey(hEncrypt)");

    kResult = ACK_CreateSession(&hDecrypt,
        (ACK_SESSIONTYPE)(ACKST_Stream | ACKST_Decrypt), 0);

    if (FAILED(kResult))
        TEST_FAIL(kResult, "ACK_CreateSession(hDecrypt)");

    kResult = ACK_MountKeySet(hDecrypt, TEST_DATABASE);

    if (FAILED(kResult))
        TEST_FAIL(kResult, "ACK_MountKeySet(hDecrypt)");

    kResult = ACK_SetSessionKey(hDecrypt, TEST_KEYID);

    if (FAILED(kResult))
        TEST_FAIL(kResult, "ACK_SetSessionKey(hDecrypt)");

    if (!getSecondsAndMicroseconds(&startSeconds, &startMicroseconds))
        TEST_FAIL(E_FAIL, "getSecondsAndMicroseconds(start)");

    for (index = 0; index < TEST_ITERATIONS; index++) {
        UINT64 offset = 0;
        LPBYTE pInput = (LPBYTE)testVectors[index % TEST_STRINGS];
        SIZE_T inSize = testLengths[index % TEST_STRINGS] + 1;

        outSize = TEST_BUFSIZE;

        kResult = ACK_Encrypt(hEncrypt, NULL, 0, &offset, pInput, inSize,
            &pOutput, &outSize);

        if (FAILED(kResult))
            TEST_FAIL(kResult, "ACK_Encrypt(hEncrypt)");

        compareSize = TEST_BUFSIZE;

        kResult = ACK_Decrypt(hDecrypt, NULL, 0, offset, pOutput, outSize,
            &pCompare, &compareSize);

        if (FAILED(kResult))
            TEST_FAIL(kResult, "ACK_Decrypt(hDecrypt)");

        if (strcmp((LPSTR)pInput, (LPSTR)pCompare) != 0) {
            snprintf(buffer, MAX_PATH,
                "encrypt/decrypt mismatch, iteration %d (\"%s\" to \"%s\")\n",
                index, (LPSTR)pInput, (LPSTR)pCompare);

            TEST_PRINT(buffer);
            TEST_FAIL(E_FAIL, "encrypt/decrypt round-trip mismatch");
        }
    }

    if (!getSecondsAndMicroseconds(&stopSeconds, &stopMicroseconds))
        TEST_FAIL(E_FAIL, "getSecondsAndMicroseconds(stop)");

    printElapsedTime(startSeconds, stopSeconds, startMicroseconds,
        stopMicroseconds, 1, "total\n");

    printElapsedTime(startSeconds, stopSeconds, startMicroseconds,
        stopMicroseconds, TEST_ITERATIONS, "per iteration\n");

    ACK_free(pOutput);
    pOutput = NULL;

    ACK_free(pCompare);
    pCompare = NULL;

    kResult = ACK_UnmountKeySet(hDecrypt);

    if (FAILED(kResult))
        TEST_FAIL(kResult, "ACK_UnmountKeySet(hDecrypt)");

    kResult = ACK_CloseSession(&hDecrypt);

    if (FAILED(kResult))
        TEST_FAIL(kResult, "ACK_CloseSession(hDecrypt)");

    kResult = ACK_UnmountKeySet(hEncrypt);

    if (FAILED(kResult))
        TEST_FAIL(kResult, "ACK_UnmountKeySet(hEncrypt)");

    kResult = ACK_CloseSession(&hEncrypt);

    if (FAILED(kResult))
        TEST_FAIL(kResult, "ACK_CloseSession(hEncrypt)");

    kResult = ACK_Finalize();

    if (FAILED(kResult))
        TEST_FAIL(kResult, "ACK_Finalize()");
}

#if !defined(FEATURE_KERNEL_ONLY)
VOID testProvider(VOID)
{
    USES_RTN;
    ACKP_PROVIDER hProvider = NULL;
    INT count;
    INT index;
    ACK_LPKEYINFO* pKeyInfo = NULL;

    RTN_IF_FAILED(ACK_Initialize());

    /* Initialize the provider. */
    RTN_IF_FAILED(ACKP_CreateProvider(&hProvider));

    /* Scan the specified path for keys */
    RTN_IF_FAILED(ACKP_ScanPathForKeys(hProvider,TEST_DATABASE_PATH));
    /* Get all the keys that were found in the scaned path(s). */
    RTN_IF_FAILED(ACKP_GetAllKeys(hProvider,&count,&pKeyInfo));
    /* Print out all the keys. */
    for (index = 0; index < count; index++)
        printKey(pKeyInfo[index],index);

    /*
     * NOTE: There is no explicit free between the two ACKP_GetAllKeys
     *      calls because ACKP_GetAllKeys ACK_free()s the previously
     *      stored pointer before storing the new one.  See its body
     *      in src/protocol.c.
     */

    /* Scan the specified path again to make sure previous scan gets cleared out. */
    RTN_IF_FAILED(ACKP_ScanPathForKeys(hProvider,TEST_DATABASE_PATH));
    /* Get all the keys that were found in the scaned path(s). */
    RTN_IF_FAILED(ACKP_GetAllKeys(hProvider,&count,&pKeyInfo));
    /* Print out all the keys. */
    for (index = 0; index < count; index++)
        printKey(pKeyInfo[index],index);

cleanup:
    ACK_free(pKeyInfo);
    ACKP_CloseProvider(hProvider);
    ACK_Finalize();
}

VOID testStreamingProtocol(VOID)
{
    USES_RTN;
    ACKP_PROVIDER hProvider = NULL;
    ACP_SESSION hEncryptSession = NULL;
    ACP_SESSION hDecryptSession = NULL;

    LPCSTR testVector1 = "Hello World!";
    LPCSTR testVector2 = "It is my honor";
    LPCSTR testVector3 = "to introduce";
    LPCSTR testVector4 = "the new Navajo engine";
    LPCSTR testVector5 = "with streaming.";

    LPCSTR testVectors[TEST_STRINGS] = {
        NULL /* "Hello World!" */,
        NULL /* "It is my honor" */,
        NULL /* "to introduce" */,
        NULL /* "the new Navajo engine" */,
        NULL /* "with streaming." */
    };

    /*
     * NOTE: These counts include the trailing NUL terminator and are
     *      passed verbatim to ACP_Encrypt.  This differs from
     *      testStreamingDirect's testLengths array, which excludes
     *      the NUL and adds +1 at the call site.
     */

    SIZE_T testLengths[TEST_STRINGS] = {
        /* "Hello World!" */ 13,
        /* "It is my honor" */ 15,
        /* "to introduce" */ 13,
        /* "the new Navajo engine" */ 22,
        /* "with streaming." */ 16
    };

    LPBYTE byOut = NULL;
    SIZE_T sizeOut = 0;
    LPBYTE byCompare = NULL;
    SIZE_T sizeCompare = 0;
    int index;
    int count;
    ACK_LPKEYINFO* pKeyInfo = NULL;
    CHAR szEncKeyId[ACK_KEYINFO_MAXID];

    testVectors[0] = testVector1;
    testVectors[1] = testVector2;
    testVectors[2] = testVector3;
    testVectors[3] = testVector4;
    testVectors[4] = testVector5;

    RTN_IF_FAILED(ACK_Initialize());
    /* Initialize the provider. */
    RTN_IF_FAILED(ACKP_CreateProvider(&hProvider));
    /* Scan the specified path for keys. */
    RTN_IF_FAILED(ACKP_ScanPathForKeys(hProvider,TEST_DATABASE_PATH));

    RTN_IF_FAILED(ACKP_GetAllKeys(hProvider,&count,&pKeyInfo));
    for (index = 0; index < count; index++)
        if (pKeyInfo[index]->isEncrypt) {
            memcpy(&szEncKeyId[0],pKeyInfo[index]->keyId,ACK_KEYINFO_MAXID);
            break;
        }
    ACK_free(pKeyInfo);
    pKeyInfo = NULL;

    /* Create two streaming sessions. */
    RTN_IF_FAILED(ACP_CreateSession(&hEncryptSession,(ACK_SESSIONTYPE)(ACKST_Stream | ACKST_Encrypt),hProvider));
    RTN_IF_FAILED(ACP_CreateSession(&hDecryptSession,(ACK_SESSIONTYPE)(ACKST_Stream | ACKST_Decrypt),hProvider));
    /* Set encryption key to use. */
    RTN_IF_FAILED(ACP_SetSessionKey(hEncryptSession,szEncKeyId));

    for (index = 0; index < TEST_ITERATIONS; index++) {
/*        VERIFY_HR_IF_FAILED(OTP::AddSessionSourceBuffer(hSession,L"QuickTest",(LPBYTE) &szBuffer[index % TEST_STRINGS][0],(lstrlen(szBuffer[index % TEST_STRINGS]) + 1) * sizeof(TCHAR)));

        DWORD dwFlags = OTP::dwEF_CHECKSUM_CRC8 | OTP::dwEF_COMPRESSION_STREAM_NONE;
//        bool bFlagResync;
//        VERIFY_HR_IF_FAILED(g_NavajoOptions[CNavajoOptions::ENCRYPT_RESYNC_CIPHER_TEXT].GetValue(&bFlagResync));
        bool bFlagAlphabetic;
        VERIFY_HR_IF_FAILED(g_NavajoOptions[CNavajoOptions::ENCRYPT_ALPHABETIC_CIPHER_TEXT].GetValue(&bFlagAlphabetic));
        switch (index % TEST_STRINGS) {
            case 3:
//                if (bFlagResync)
//                    dwFlags = dwFlags | OTP::dwEF_RESYNC_CIPHER_TEXT;
            default:
                if (bFlagAlphabetic)
                    dwFlags = dwFlags | OTP::dwEF_ALPHABETIC_CIPHER_TEXT;
                break;
        }
        VERIFY_HR_IF_FAILED(OTP::Encrypt(hSession,dwFlags));
*/
        RTN_IF_FAILED(ACP_Encrypt(hEncryptSession,0,(LPBYTE) testVectors[index % TEST_STRINGS],testLengths[index % TEST_STRINGS],&byOut,&sizeOut));
/*
        DWORD dwBufferSize;
        VERIFY_HR_IF_FAILED(OTP::GetSessionDestinationBuffer(hSession,NULL,&dwBufferSize));
        DWORD dwExpected = (lstrlen(szBuffer[index % TEST_STRINGS]) + 1) * sizeof(TCHAR) + sizeof(DWORD);
        if (bFlagAlphabetic) dwExpected = dwExpected * 4;
        _tprintf(_T("\nEncrypted size: %d  %s"),dwBufferSize,(dwExpected == dwBufferSize) ? _T("Match") : _T("FAILED"));
        CAutoPtr<BYTE> spBuffer(new BYTE[dwBufferSize]);
        VERIFY_HR_IF_BADNEW(spBuffer);
        VERIFY_HR_IF_FAILED(OTP::GetSessionDestinationBuffer(hSession,spBuffer,&dwBufferSize));

        VERIFY_HR_IF_FAILED(OTP::AddSessionSourceBuffer(hSessionDecrypt,L"QuickTest",spBuffer,dwBufferSize));
        spBuffer.Free();
        VERIFY(OTP::IsCipherText(hSessionDecrypt) == S_OK);
        VERIFY_HR_IF_FAILED(OTP::Decrypt(hSessionDecrypt,0));
*/
        RTN_IF_FAILED(ACP_Decrypt(hDecryptSession,0,byOut,sizeOut,&byCompare,&sizeCompare));
/*
        dwBufferSize = (lstrlen(szBuffer[index % TEST_STRINGS]) + 1) * sizeof(TCHAR);
        spBuffer.Attach(new BYTE[dwBufferSize]);
        VERIFY_HR_IF_BADNEW(spBuffer);
        VERIFY_HR_IF_FAILED(OTP::GetSessionDestinationBuffer(hSessionDecrypt,spBuffer,&dwBufferSize));
*/
        if (strcmp(testVectors[index % TEST_STRINGS],(LPSTR) byCompare) != 0) {
            printf("encrypt/decrypt mismatch (%s to %s).\n",
                testVectors[index % TEST_STRINGS],(LPSTR) byCompare);
            RTN_IF_FAILED(E_FAIL);
        }
    }
    printf("Streaming success.\n");

cleanup:
    ACK_free(byOut);
    ACK_free(byCompare);
    ACP_CloseSession(hEncryptSession);
    ACP_CloseSession(hDecryptSession);
    ACKP_CloseProvider(hProvider);
    ACK_Finalize();
}
#endif

#if defined(_WIN32_WCE)
int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPWSTR lpCmdLine,
    int nShowCmd
    )
#else
int main(
    int argc,
    char *argv[]
    )
#endif
{
    CHAR buffer[MAX_PATH + 1];

#if defined(_MSC_VER) && !defined(_WIN32_WCE)
    /*
     * NOTE: Show memory leaks.
     */

    _CrtSetDbgFlag(
        _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) |
            _CRTDBG_LEAK_CHECK_DF);
#endif

    TEST_PRINT("Starting testGetAllKeys...\n");
    testGetAllKeys();
    TEST_PRINT("Done.\n");
    TEST_PAUSE();

    TEST_PRINT("Starting testStreamingDirect...\n");
    testStreamingDirect();

#if !defined(FEATURE_KERNEL_ONLY)
    testProvider();
    testStreamingProtocol();
#endif

    sqlite3_release_memory(0);

    snprintf(buffer, MAX_PATH,
        "SQLite bytes: " INT64_FORMAT " out of " INT64_FORMAT "\n",
        sqlite3_memory_used(), sqlite3_memory_highwater(0));

    TEST_PRINT(buffer);
    TEST_PRINT("Done.\n");
    TEST_PAUSE();

    return 0;
}

/* end of file*/
