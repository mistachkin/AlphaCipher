/*
 * sdkTest.c --
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

#if defined(_MSC_VER)
#include <crtdbg.h>
#endif

#include "navajoPort.h"
#include "navajo.h"
#include "hresult.h"

#if (defined(_POSIX_VERSION) && _POSIX_VERSION >= 200112L) || \
        defined(HAVE_GETTIMEOFDAY)
#include <sys/time.h>
#endif

#include "sdkTest.h"

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

    if (iterations == 1)
        snprintf(buffer, MAX_PATH,
            INT64_FORMAT " seconds OR " INT64_FORMAT " microseconds %s\n",
            (stopSeconds - startSeconds),
            (stopMicroseconds - startMicroseconds),
            suffix);
    else
        snprintf(buffer, MAX_PATH,
            "%.04f seconds OR %.04f microseconds %s\n",
            (stopSeconds - startSeconds) / (double)iterations,
            (stopMicroseconds - startMicroseconds) / (double)iterations,
            suffix);

    TEST_PRINT(buffer);
}

BOOL getSecondsAndMicroseconds(
    LPINT64 pSeconds,
    LPINT64 pMicroseconds
    )
{
#if defined(WIN32)
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

    printf("Key #%d Total " UINT64_FORMAT "\n",
        keyIndex, pKeyInfo->bytesTotal);

    printf("Key #%d Used " UINT64_FORMAT "\n",
        keyIndex, pKeyInfo->bytesUsed);
}

static LPSTR getTestDatabase(VOID) /* PRIVATE */
{
    /*
     * NOTE: Honor ACK_TEST_DATABASE for a runtime override so that
     *       'make test' can stage the fixture under obj/ without
     *       requiring write access to the legacy hard-coded path
     *       (X:\layout.pef on Windows, /keys/layout.pef on POSIX).
     */

    LPSTR override = getenv("ACK_TEST_DATABASE");

    if ((override != NULL) && (*override != '\0'))
        return override;

    return (LPSTR)TEST_DATABASE;
}

VOID testGetAllKeys(VOID)
{
    ACK_RESULT kResult = ACK_S_OK;
    ACK_SESSION session = NULL;
    INT64 count = 0;
    ACK_LPKEYINFO pKeyInfo = NULL;
    INT index = 0;

    kResult = ACK_Initialize();

    if (FAILED(kResult))
        TEST_FAIL(kResult, "ACK_Initialize()");

    kResult = ACK_CreateSession(&session, ACKST_ListKeys, 0);

    if (FAILED(kResult))
        TEST_FAIL(kResult, "ACK_CreateSession(session)");

    /*
     * NOTE: The master database will always have a file
     *       name similar to "layout.pef".
     */

    kResult = ACK_MountKeySet(session, getTestDatabase());

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

    kResult = ACK_MountKeySet(hEncrypt, getTestDatabase());

    if (FAILED(kResult))
        TEST_FAIL(kResult, "ACK_MountKeySet(hEncrypt)");

    kResult = ACK_SetSessionKey(hEncrypt, TEST_KEYID);

    if (FAILED(kResult))
        TEST_FAIL(kResult, "ACK_SetSessionKey(hEncrypt)");

    kResult = ACK_CreateSession(&hDecrypt,
        (ACK_SESSIONTYPE)(ACKST_Stream | ACKST_Decrypt), 0);

    if (FAILED(kResult))
        TEST_FAIL(kResult, "ACK_CreateSession(hDecrypt)");

    kResult = ACK_MountKeySet(hDecrypt, getTestDatabase());

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

int main(
    int argc,
    char *argv[]
    )
{
#if defined(_MSC_VER)
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
    TEST_PRINT("Done.\n");
    TEST_PAUSE();

    return 0;
}

/* end of file*/
