/*
 * test.h --
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

#ifndef _TEST_H_
#define _TEST_H_

/*****************************************************************************/

#define MICROSECONDS_PER_SECOND                          (1000000)

#define TEST_STRINGS                                           (5)

#if defined(__SYMBIAN32__)
#define TEST_DATABASE_PATH                                  "E:\\"
#elif defined(_WIN32_WCE)
// #define TEST_DATABASE                       "\\KeySet\\layout.pef"
#define TEST_DATABASE_PATH                      "\\Storage Card\\"
#else
#define TEST_DATABASE_PATH                                  "X:\\"
#endif

#define TEST_DATABASE              TEST_DATABASE_PATH "layout.pef"

#define TEST_KEYID          "e5c3fa33-2c5f-427d-bd1e-1703d36a1cef"
#define TEST_ITERATIONS                                     (1000)
#define TEST_BUFSIZE                                        (1000)

#define TEST_PAUSE() do {                                        \
    fprintf(stdout, "Press enter to continue...\n");             \
    testPause(FALSE);                                            \
} while (0)

#define TEST_PRINT(a) do {                                       \
    printBuffer((a));                                            \
} while (0)

#define TEST_FAIL(a,b) do {                                      \
    CHAR buffer[MAX_PATH + 1];                                   \
    snprintf(buffer, MAX_PATH, "FAIL 0x%08lX : %s\n", (a), (b)); \
    TEST_PRINT(buffer);                                          \
    TEST_PAUSE();                                                \
    assert(!FAILED((a)) && (b));                                 \
    abort();                                                     \
} while (0)

/*****************************************************************************/

VOID testPause(
    BOOL force /* in */
);

VOID printBuffer(
    LPSTR buffer /* in */
);

/*****************************************************************************/

VOID printElapsedTime(
    INT64 startSeconds,      /* in */
    INT64 stopSeconds,       /* in */
    INT64 startMicroseconds, /* in */
    INT64 stopMicroseconds,  /* in */
    INT64 iterations,        /* in */
    LPSTR suffix             /* in */
);

BOOL getSecondsAndMicroseconds(
    LPINT64 pSeconds,     /* out */
    LPINT64 pMicroseconds /* out */
);

VOID printKey(
    ACK_LPKEYINFO pKeyInfo, /* in */
    int keyIndex            /* in */
);

VOID testGetAllKeys(VOID);

VOID testStreamingDirect(VOID);

/*****************************************************************************/

#if !defined(FEATURE_KERNEL_ONLY)
VOID testProvider(VOID);

VOID testStreamingProtocol(VOID);
#endif

#endif /* _TEST_H_ */

/* end of file*/
