/*
 * transport.h -- Public transport functions
 *
 * Copyright (c) 2008-2026 by Joe Mistachkin.  All rights reserved.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) $Id: $
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef FACILITY_ACT
#define FACILITY_ACT                         98
#endif

#ifndef ACT_E_IN_USE
#define ACT_E_IN_USE \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ACT, 0x1)
#endif

ACK_RESULT InitializeTransport();
ACK_RESULT TerminateTransport();
ACK_RESULT DialRemote(LPCSTR host, LPCSTR port);
ACK_RESULT AnswerRemote(LPCSTR localHost, LPCSTR remoteHost, LPCSTR port);
ACK_RESULT TransmitBuffer(LPBYTE pBuffer, SIZE_T length);

#ifdef __cplusplus
}  //End of the 'extern "C"' block.
#endif
