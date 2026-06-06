/*
 * tinCan.cpp --
 *
 * Copyright (c) 2008-2026 by Joe Mistachkin.  All rights reserved.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * written by: Joe Mistachkin and Dawson Cowals
 *
 * RCS: @(#) $Id: $
 */

#include <assert.h>
#include <stddef.h>
#include <windows.h>

#include "navajoPort.h"
#include "navajo.h"

#include "hresult.h"
#include "tinCan.h"

#ifndef MM_WOM_WRITE
#define MM_WOM_WRITE               (WM_APP + 1L)
#endif

#ifndef WAIT_OBJECT_1
#define WAIT_OBJECT_1       (WAIT_OBJECT_0 + 1L)
#endif

#define SPINLOCK_TRIES                      (5L)
#define SPINLOCK_WAIT                     (500L)

#define MILLISECONDS_PER_SECOND          (1000L)

#define BITS_PER_BYTE                       (8L)
#define BITS_PER_SAMPLE                     (8L)

/* 8kHz */
#define SAMPLES_PER_SECOND               (8000L)

#define BYTES_PER_SECOND                 (8192L)

#define BUFFER_MILLISECONDS                (25L)

#define BUFFER_DATA_BYTES \
    (((BITS_PER_SAMPLE * SAMPLES_PER_SECOND) / BITS_PER_BYTE) \
        / MILLISECONDS_PER_SECOND ) * BUFFER_MILLISECONDS

/*
 * NOTE: The BUFFER_BYTES value MUST be divisible by the platform word size
 *       (Windows CE) due to data alignment constraints.
 */

#define BUFFER_BYTES (sizeof(DWORD) + sizeof(WAVEHDR) + BUFFER_DATA_BYTES)

#define BUFFER_HEADER_OFFSET (sizeof(DWORD))
#define BUFFER_DATA_OFFSET   (BUFFER_HEADER_OFFSET + sizeof(WAVEHDR))

#define INPUT_BUFFER_SLOTS                (100L)
#define OUTPUT_BUFFER_SLOTS               (100L)

#define INPUT_BUFFER_SIZE      (BUFFER_BYTES * INPUT_BUFFER_SLOTS)
#define OUTPUT_BUFFER_SIZE     (BUFFER_BYTES * OUTPUT_BUFFER_SLOTS)

#define INPUT_BUFFER_OFFSET   (0)
#define OUTPUT_BUFFER_OFFSET  (INPUT_BUFFER_SIZE)
#define TOTAL_BUFFER_SIZE     (INPUT_BUFFER_SIZE + OUTPUT_BUFFER_SIZE)

#define MAX_OUTPUT_WRITE_SIZE   (BUFFER_DATA_BYTES * 10)

static DWORD WINAPI WaveInputThreadProc(LPVOID pParameter);
static DWORD WINAPI WaveOutputThreadProc(LPVOID pParameter);

static LPBYTE GetWaveBuffer();
static LPBYTE GetWaveSlotBuffer(BOOL output, LONG slot);
static LPBYTE GetNextWaveSlotBuffer(BOOL output);
static VOID MarkWaveBufferAvailable(LPWAVEHDR pWaveHeader);

static HRESULT LockWaveModule();
static HRESULT UnlockWaveModule();

static HANDLE hInputThread = NULL;
static HANDLE hOutputThread = NULL;

static DWORD inputThreadId = 0;
static DWORD outputThreadId = 0;

static HANDLE hInputShutdown = NULL;
static HANDLE hOutputShutdown = NULL;

static HWAVEIN hWaveIn = NULL;
static HWAVEOUT hWaveOut = NULL;

static LONG nextInputSlot = -1;
static LONG nextOutputSlot = -1;

static LONG waveLock = 0;
static LPBYTE waveBuffer = NULL;

HRESULT InitializeWaveModule(ACK_LPDATAPROC proc)
{
    HRESULT hResult = S_OK;
    MMRESULT status = MMSYSERR_NOERROR;
    WAVEFORMATEX waveFormat = {0};

    if (LockWaveModule() != S_OK)
        return E_LOCK;

    if ((waveInGetNumDevs() == 0) || (waveOutGetNumDevs() == 0)) {
        hResult = E_NOTIMPL;
        goto done;
    }

    if (waveBuffer == NULL) {
        waveBuffer = (LPBYTE)ACK_zalloc(
            sizeof(BYTE) * TOTAL_BUFFER_SIZE);

        if (waveBuffer == NULL) {
            hResult = E_OUTOFMEMORY;
            goto done;
        }
    }

    if (hInputShutdown == NULL) {
        hInputShutdown = CreateEvent(NULL, TRUE, FALSE, NULL);

        if (hInputShutdown == NULL) {
            hResult = HRESULT_FROM_WIN32(GetLastError());
            goto done;
        }
    }

    if (hOutputShutdown == NULL) {
        hOutputShutdown = CreateEvent(NULL, TRUE, FALSE, NULL);

        if (hOutputShutdown == NULL) {
            hResult = HRESULT_FROM_WIN32(GetLastError());
            goto done;
        }
    }

    if (hInputThread == NULL) {
        hInputThread = CreateThread(NULL, 0, WaveInputThreadProc, proc, 0,
            &inputThreadId);

       if (hInputThread == NULL) {
            hResult = HRESULT_FROM_WIN32(GetLastError());
            goto done;
       }
       CeSetThreadPriority(hInputThread, CeGetThreadPriority(hInputThread) - 1);
    }

    if (hOutputThread == NULL) {
        hOutputThread = CreateThread(NULL, 0, WaveOutputThreadProc, NULL, 0,
            &outputThreadId);

       if (hOutputThread == NULL) {
            hResult = HRESULT_FROM_WIN32(GetLastError());
            goto done;
       }
    }

    waveFormat.wFormatTag           = WAVE_FORMAT_PCM;
    waveFormat.nChannels            = 1;
    waveFormat.nSamplesPerSec       = SAMPLES_PER_SECOND;
    waveFormat.nBlockAlign          = waveFormat.nChannels * (BITS_PER_SAMPLE / BITS_PER_BYTE);
    waveFormat.nAvgBytesPerSec      = BYTES_PER_SECOND;
    waveFormat.wBitsPerSample       = BITS_PER_SAMPLE;
    waveFormat.cbSize               = 0;

    status = waveInOpen(&hWaveIn, WAVE_MAPPER, &waveFormat, inputThreadId,
        NULL, CALLBACK_THREAD);

    if (status != MMSYSERR_NOERROR) {
        hResult = HRESULT_FROM_MMSYSTEM(status);
        goto done;
    }

    status = waveOutOpen(&hWaveOut, WAVE_MAPPER, &waveFormat, outputThreadId,
        NULL, CALLBACK_THREAD);

    if (status != MMSYSERR_NOERROR) {
        hResult = HRESULT_FROM_MMSYSTEM(status);
        goto done;
    }

done:
    UnlockWaveModule();

    return hResult;
}

HRESULT FinalizeWaveModule()
{
    HRESULT hResult = S_OK;
    MMRESULT status = MMSYSERR_ERROR;

    if (LockWaveModule() != S_OK)
        return E_LOCK;

    if (hWaveOut != NULL) {
        status = waveOutClose(hWaveOut);

        if (status != MMSYSERR_NOERROR)
            hResult = HRESULT_FROM_MMSYSTEM(status);

        hWaveOut = NULL;
    }

    if (hWaveIn != NULL) {
        status = waveInClose(hWaveIn);

        if (status != MMSYSERR_NOERROR)
            hResult = HRESULT_FROM_MMSYSTEM(status);

        hWaveIn = NULL;
    }

    if (hOutputShutdown != NULL) {
        SetEvent(hOutputShutdown);

        if (hOutputThread != NULL) {
            if (WaitForSingleObject(hOutputThread, THREAD_TIMEOUT) != WAIT_OBJECT_0)
                TerminateThread(hOutputThread, EXITCODE_TERMINATE);

            CloseHandle(hOutputThread);
            hOutputThread = NULL;
        }

        CloseHandle(hOutputShutdown);
        hOutputShutdown = NULL;
    }

    if (hInputShutdown != NULL) {
        SetEvent(hInputShutdown);

        if (hInputThread != NULL) {
            if (WaitForSingleObject(hInputThread, THREAD_TIMEOUT) != WAIT_OBJECT_0)
                TerminateThread(hInputThread, EXITCODE_TERMINATE);

            CloseHandle(hInputThread);
            hInputThread = NULL;
        }

        CloseHandle(hInputShutdown);
        hInputShutdown = NULL;
    }

    if (waveBuffer != NULL) {
        ACK_free(waveBuffer);
        waveBuffer = NULL;
    }

    UnlockWaveModule();

    return hResult;
}

static DWORD WINAPI WaveInputThreadProc(
    LPVOID pParameter
    )
{
    DWORD dwExitCode = EXITCODE_SUCCESS;
    LONG slot = 0;
    MMRESULT status = MMSYSERR_ERROR;
    MSG msg = {0};
    DWORD dwResult = 0;
    ACK_LPDATAPROC proc = (ACK_LPDATAPROC)pParameter;

    //Do not let this thread go to far before the thread starter has
    //finished releasing the lock on the wave module.  There is a potential
    //for this thread to exit early because the waveLock might be locked
    //by the thread starter function.
    while (LockWaveModule() != S_OK)
        Sleep(100);
    //Now that we got the lock, release it and continue this thread.
    UnlockWaveModule();

    /*
     * NOTE: Prepare all the WAVEHDR structures for use by the system.
     */

    for (slot = 0; slot < INPUT_BUFFER_SLOTS; slot++) {
        LPBYTE pBuffer = GetWaveSlotBuffer(FALSE, slot);
        LPWAVEHDR pWaveHeader = (LPWAVEHDR)(pBuffer + BUFFER_HEADER_OFFSET);

        if (pBuffer == NULL) {
            dwExitCode = EXITCODE_FAILURE;
            goto done;
        }

        pWaveHeader->dwBufferLength = BUFFER_DATA_BYTES;
        pWaveHeader->lpData = (LPSTR)(pBuffer + BUFFER_DATA_OFFSET);

        status = waveInPrepareHeader(hWaveIn, pWaveHeader, sizeof(WAVEHDR));

        if (status != MMSYSERR_NOERROR) {
            dwExitCode = EXITCODE_FAILURE;
            goto done;
        }

        status = waveInAddBuffer(hWaveIn, pWaveHeader, sizeof(WAVEHDR));

        if (status != MMSYSERR_NOERROR) {
            dwExitCode = EXITCODE_FAILURE;
            goto done;
        }
    }

    status = waveInStart(hWaveIn);

    if (status != MMSYSERR_NOERROR) {
        dwExitCode = EXITCODE_FAILURE;
        goto done;
    }

    /*
     * NOTE: Force message queue to be created for this thread.
     */

    PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

    do {
        dwResult = MsgWaitForMultipleObjectsEx(1, &hInputShutdown, INFINITE,
            QS_ALLINPUT, MWMO_INPUTAVAILABLE);

        if (dwResult == WAIT_FAILED)
            END_THIS_THREAD_PLEASE(EXITCODE_FAILURE);

        if (dwResult == WAIT_OBJECT_1) {
            MSG msg = {0};

            if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                switch (msg.message) {
                    case MM_WIM_OPEN: {
                        hWaveIn = (HWAVEIN)msg.wParam;
                        break;
                    }
                    case MM_WIM_DATA: {
                        LPWAVEHDR pWaveHeader = (LPWAVEHDR)msg.lParam;
                        DWORD dwLength = pWaveHeader->dwBufferLength;
                        LPBYTE pData = (LPBYTE)pWaveHeader->lpData;

                        #if 0
                        LPBYTE pOutData = (LPBYTE)ACK_zalloc(sizeof(BYTE) * dwLength);
                        memcpy(pOutData, pData, dwLength);
                        PostToWaveOut(pOutData, dwLength);
                        #else
                        /*
                         * NOTE: Call the function pointer supplied by our caller
                         *       with the newly acquired input data.
                         */

                        if (proc != NULL) proc(pData, dwLength);
                        #endif

                        status = waveInUnprepareHeader(hWaveIn, pWaveHeader, sizeof(WAVEHDR));

                        if (status != MMSYSERR_NOERROR) {
                            dwExitCode = EXITCODE_FAILURE;
                            goto done;
                        }

                        status = waveInPrepareHeader(hWaveIn, pWaveHeader, sizeof(WAVEHDR));

                        if (status != MMSYSERR_NOERROR) {
                            dwExitCode = EXITCODE_FAILURE;
                            goto done;
                        }

                        status = waveInAddBuffer(hWaveIn, pWaveHeader, sizeof(WAVEHDR));

                        if (status != MMSYSERR_NOERROR) {
                            dwExitCode = EXITCODE_FAILURE;
                            goto done;
                        }
                        break;
                    }
                    case MM_WIM_CLOSE: {
                        hWaveIn = NULL;
                    }
                    default: {
                        /*
                         * NOTE: This could, in theory, be necessary if actual
                         *       windows are being hosted in this thread (YMMV).
                         */

                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                        break;
                    }
                }
            }
        }
    } while (dwResult != WAIT_OBJECT_0);

done:
    /*
     * NOTE: Free all the WAVEHDR structures we prepared previously.
     */

    for (slot = 0; slot < INPUT_BUFFER_SLOTS; slot++) {
        LPBYTE pBuffer = GetWaveSlotBuffer(FALSE, slot);

        if (pBuffer == NULL)
            continue;

        /*
         * NOTE: The WAVEHDR sits at BUFFER_HEADER_OFFSET inside the slot,
         *       matching how it was registered with waveInPrepareHeader.
         */

        /* IGNORED */
        waveInUnprepareHeader(hWaveIn,
            (LPWAVEHDR)(pBuffer + BUFFER_HEADER_OFFSET), sizeof(WAVEHDR));
    }

    /*
     * NOTE: dwExitCode is set explicitly on every error path above,
     *       so a non-success value here indicates a real failure that
     *       has already been recorded.  The previous assert(...) was a
     *       no-op in release builds and provided false confidence.
     */

    END_THIS_THREAD_PLEASE(dwExitCode);
}

static DWORD WINAPI WaveOutputThreadProc(
    LPVOID pParameter
    )
{
    DWORD dwExitCode = EXITCODE_SUCCESS;
    LONG slot = 0;
    MMRESULT status = MMSYSERR_ERROR;
    MSG msg = {0};
    DWORD dwResult = 0;

    //Do not let this thread go to far before the thread starter has
    //finished releasing the lock on the wave module.  There is a potential
    //for this thread to exit early because the waveLock might be locked
    //by the thread starter function.
    while (LockWaveModule() != S_OK)
        Sleep(100);
    //Now that we got the lock, release it and continue this thread.
    UnlockWaveModule();

    /*
     * NOTE: Force message queue to be created for this thread.
     */

    PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

    do {
        dwResult = MsgWaitForMultipleObjectsEx(1, &hOutputShutdown, INFINITE,
            QS_ALLINPUT, MWMO_INPUTAVAILABLE);

        if (dwResult == WAIT_FAILED) {
            dwExitCode = EXITCODE_FAILURE;
            goto done;
        }

        if (dwResult == WAIT_OBJECT_1) {
            MSG msg = {0};

            if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                switch (msg.message) {
                    case MM_WOM_OPEN: {
                        hWaveOut = (HWAVEOUT)msg.wParam;
                        break;
                    }
                    case MM_WOM_WRITE /* CUSTOM */: {
                        /*
                         * NOTE: The WPARAM for this message is the buffer size,
                         *       in bytes.  The LPARAM is the pointer to the
                         *       buffer.
                         */

                        ACK_BUFFER messageBuffer = {
                            msg.wParam, (LPBYTE)msg.lParam
                        };

                        /*
                         * NOTE: If there is no buffer supplied with the message,
                         *       just ignore it.
                         */

                        if (messageBuffer.pData == NULL)
                            continue;

                        /*
                         * NOTE: Keep going until the message buffer is exhausted.
                         */

                        while (messageBuffer.size > 0) {
                            LPBYTE pBuffer = GetNextWaveSlotBuffer(TRUE);
                            LPWAVEHDR pWaveHeader = (LPWAVEHDR)(pBuffer + BUFFER_HEADER_OFFSET);

                            if (pBuffer == NULL)
                                break; /* DROP_PACKET_ON_FLOOR(); */

                            pWaveHeader->dwBufferLength = min(
                                messageBuffer.size, BUFFER_DATA_BYTES);
                            pWaveHeader->lpData = (LPSTR)(pBuffer + BUFFER_DATA_OFFSET);

                            memcpy(pWaveHeader->lpData, messageBuffer.pData,
                                pWaveHeader->dwBufferLength);

                            status = waveOutPrepareHeader(hWaveOut,
                                pWaveHeader, sizeof(WAVEHDR));

                            if (status != MMSYSERR_NOERROR) {
                                dwExitCode = EXITCODE_FAILURE;
                                goto done;
                            }

                            status = waveOutWrite(hWaveOut, pWaveHeader,
                                sizeof(WAVEHDR));

                            if (status != MMSYSERR_NOERROR) {
                                dwExitCode = EXITCODE_FAILURE;
                                goto done;
                            }

                            /*
                             * NOTE: Advance to the next block of the message buffer,
                             *       if any.
                             */

                            messageBuffer.size -= pWaveHeader->dwBufferLength;
                            messageBuffer.pData += pWaveHeader->dwBufferLength;
                        }

                        ACK_free((LPBYTE)msg.lParam);
                        break;
                    }
                    case MM_WOM_DONE: {
                        LPWAVEHDR pWaveHeader = (LPWAVEHDR)msg.lParam;

                        status = waveOutUnprepareHeader(hWaveOut, pWaveHeader,
                            sizeof(WAVEHDR));

                        if (status != MMSYSERR_NOERROR) {
                            dwExitCode = EXITCODE_FAILURE;
                            goto done;
                        }

                        MarkWaveBufferAvailable(pWaveHeader);
                        break;
                    }
                    case MM_WOM_CLOSE: {
                        hWaveOut = NULL;
                    }
                    default: {
                        /*
                         * NOTE: This could, in theory, be necessary if actual
                         *       windows are being hosted in this thread (YMMV).
                         */

                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                        break;
                    }
                }
            }
        }
    } while (dwResult != WAIT_OBJECT_0);

done:
    /*
     * NOTE: Free all the WAVEHDR structures we prepared previously.
     */

    for (slot = 0; slot < OUTPUT_BUFFER_SLOTS; slot++) {
        LPBYTE pBuffer = GetWaveSlotBuffer(TRUE, slot);

        if (pBuffer == NULL)
            continue;

        /*
         * NOTE: The WAVEHDR sits at BUFFER_HEADER_OFFSET inside the slot,
         *       matching how it was registered with waveOutPrepareHeader.
         */

        /* IGNORED */
        waveOutUnprepareHeader(hWaveOut,
            (LPWAVEHDR)(pBuffer + BUFFER_HEADER_OFFSET), sizeof(WAVEHDR));
    }

    /*
     * NOTE: dwExitCode is set explicitly on every error path above,
     *       so a non-success value here indicates a real failure that
     *       has already been recorded.  The previous assert(...) was a
     *       no-op in release builds and provided false confidence.
     */

    END_THIS_THREAD_PLEASE(dwExitCode);
}

static HRESULT LockWaveModule()
{
    INT tries = 0;

    do {
        /*
         * NOTE: In one step, try to acquire the lock and check if we
         *       actually succeeded.
         */

        if (InterlockedCompareExchange(&waveLock, 1, 0) == 0)
            return S_OK;

        /*
         * NOTE: Just give up the remainder of our timeslice.
         */

        Sleep(0);
    } while (tries++ < SPINLOCK_TRIES);

    return S_FALSE;
}

static HRESULT UnlockWaveModule()
{
    return (InterlockedCompareExchange(&waveLock, 0, 1) == 1) ?
        S_OK : S_FALSE;
}

static LPBYTE GetWaveBuffer()
{
    LPBYTE pBuffer = NULL;

    if (LockWaveModule() != S_OK)
        return NULL;

    pBuffer = waveBuffer;

    UnlockWaveModule();

    return pBuffer;
}

static LPBYTE GetWaveSlotBuffer(
    BOOL output,
    LONG slot
    )
{
    LPBYTE pBuffer = GetWaveBuffer();

    if (pBuffer == NULL)
        return NULL;

    if (slot < 0)
        return NULL;

    if (!output && (slot >= INPUT_BUFFER_SLOTS))
        return NULL;

    if (output && (slot >= OUTPUT_BUFFER_SLOTS))
        return NULL;

    return (pBuffer +
        (output ? OUTPUT_BUFFER_OFFSET : INPUT_BUFFER_OFFSET) +
        (slot * BUFFER_BYTES));
}

static VOID MarkWaveBufferAvailable(
    LPWAVEHDR pWaveHeader
    )
{
    LPDWORD pUsed = (LPDWORD)((LPBYTE)pWaveHeader - sizeof(DWORD));

    *pUsed = FALSE;
    return;
}

static LPBYTE GetNextWaveSlotBuffer(
    BOOL output
    )
{
    LPLONG pAddend = output ? &nextOutputSlot : &nextInputSlot;
    LONG slots = output ? OUTPUT_BUFFER_SLOTS : INPUT_BUFFER_SLOTS;
    LPBYTE pBuffer = NULL;

    while (1) {
        LONG slot = InterlockedIncrement(pAddend);
        LPDWORD pUsed = NULL;

        if (slot >= slots) {
            InterlockedExchange(pAddend, slot = 0);

            /*
             * NOTE: Abort the search, for now.  The caller will simply have to
             *       abandon this packet of audio data.
             */

            return NULL;
        }

        pBuffer = GetWaveSlotBuffer(output, slot);

        if (pBuffer == NULL)
            continue;

        pUsed = (LPDWORD)pBuffer;

        if (*pUsed)
            continue;

        *pUsed = TRUE;
        break;
    }

    return pBuffer;
}

HRESULT PostToWaveOut( /* PUBLIC */
    LPBYTE pData,
    DWORD size
    )
{
    if (pData == NULL)
        return E_POINTER;

    if (size == 0)
        return E_INVALIDARG;
    else if (size > INT_MAX)
        return E_OVERFLOW;
    else if (size > MAX_OUTPUT_WRITE_SIZE)
        return E_TOOLARGE;

    if (!PostThreadMessage(outputThreadId, MM_WOM_WRITE, (WPARAM)size,
            (LPARAM)pData))
        return HRESULT_FROM_WIN32(GetLastError());

    return S_OK;
}

/* end of file */
