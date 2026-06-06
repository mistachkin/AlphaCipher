/*
 * tinCan.h --
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

#if defined(_MSC_VER) && defined(WIN32) && !defined(_WIN32_WCE) && !defined(_INC_STDLIB)
#error "The header file <stdlib.h> must be included prior to this file."
#endif

#ifndef _TINCAN_H_
#define _TINCAN_H_

#define THREAD_TIMEOUT                    (2000)

#define EXITCODE_SUCCESS            EXIT_SUCCESS
#define EXITCODE_FAILURE            EXIT_FAILURE
#define EXITCODE_TERMINATE                   (2)

#define END_THIS_THREAD_PLEASE(exitCode) { \
    ExitThread(exitCode); return exitCode; \
}



#ifndef FACILITY_MMSYSTEM
#define FACILITY_MMSYSTEM                     43
#endif


#ifndef HRESULT_FROM_MMSYSTEM
#define HRESULT_FROM_MMSYSTEM(x) \
    ((HRESULT)(x) <= 0 ? ((HRESULT)(x)) : \
    ((HRESULT) (((x) & 0x0000FFFF) | \
        (FACILITY_MMSYSTEM << 16) | 0x80000000)))
#endif

typedef HRESULT (*ACK_LPDATAPROC)(
    LPBYTE pData, /* in */
    DWORD size    /* in */
);

ACK_API HRESULT InitializeWaveModule(ACK_LPDATAPROC proc);
ACK_API HRESULT FinalizeWaveModule();

ACK_API HRESULT PostToWaveOut(
    LPBYTE pData, /* in */
    DWORD size   /* in */
);

#endif /* _TINCAN_H_ */
